/*
 * Copyright 2026 Kitsuri Studios
 * Developed by Mostafizur Rahman (aeticusdev)
 *
 * SUMMARY (BSD 3-Clause License):
 *  You may use, copy, modify, and distribute this software
 *  You may use it for commercial and private purposes
 *  You must include this copyright notice and license text
 *  You may NOT use the project name or contributors to endorse derived products
 *  No warranty or liability is provided by the authors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "vfs.hpp"
#include "runtime.hpp"
#include "ipc.hpp"
#include "../bindings/system.hpp"
#include "../bindings/fs.hpp"
#include "../bindings/os.hpp"
#include "../bindings/dialog.hpp"
#include "../bindings/net.hpp"
#include "../bindings/socket.hpp"

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
#endif

#include <core/include/webview.h>
#include <quickjs/quickjs.h>
#include <uv.h>

#include <thread>
#include <mutex>
#include <deque>
#include <string>
#include <functional>
#include <atomic>
#include <memory>
#include <cstring>
#include <map>
#include <chrono>
#include <iostream>

#ifndef _WIN32
    #include <unistd.h>
#endif

namespace valkyrie {

std::deque<ipc_message> g_ipc_queue;
std::mutex g_ipc_mutex;
static uv_async_t g_async_handle;
webview::webview* g_webview_ptr = nullptr;
static JSContext* g_ctx = nullptr;

static JSValue js_require(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* module_name = JS_ToCString(ctx, argv[0]);
    if (!module_name) return JS_EXCEPTION;
    
    std::string path = module_name;
    
    if (path == "http" || path == "https") {
        JSValue exports = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, exports, "fetch", JS_NewCFunction(ctx, js_http_fetch, "fetch", 1));
        JS_FreeCString(ctx, module_name);
        return exports;
    }
    
    auto file = vfs::instance().read_file(path);
    
    if (!file && path.find('.') == std::string::npos) {
        file = vfs::instance().read_file(path + ".js");
    }
    
    if (!file) {
        JS_FreeCString(ctx, module_name);
        return JS_ThrowReferenceError(ctx, "Module not found: %s", module_name);
    }
    
    JSValue module_obj = JS_NewObject(ctx);
    JSValue exports_obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, module_obj, "exports", exports_obj);
    
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue old_module = JS_GetPropertyStr(ctx, global, "module");
    JSValue old_exports = JS_GetPropertyStr(ctx, global, "exports");
    
    JS_SetPropertyStr(ctx, global, "module", JS_DupValue(ctx, module_obj));
    JS_SetPropertyStr(ctx, global, "exports", JS_DupValue(ctx, exports_obj));
    
    std::string code((char*)file->data.data(), file->data.size());
    JSValue result = JS_Eval(ctx, code.c_str(), code.size(), module_name, JS_EVAL_TYPE_GLOBAL);
    
    JS_SetPropertyStr(ctx, global, "module", old_module);
    JS_SetPropertyStr(ctx, global, "exports", old_exports);
    JS_FreeValue(ctx, global);
    
    if (JS_IsException(result)) {
        JS_FreeValue(ctx, module_obj);
        JS_FreeCString(ctx, module_name);
        return result;
    }
    
    JS_FreeValue(ctx, result);
    
    JSValue ret = JS_GetPropertyStr(ctx, module_obj, "exports");
    JS_FreeValue(ctx, module_obj);
    JS_FreeCString(ctx, module_name);
    
    return ret;
}

static JSValue js_native_print(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc > 0) {
        const char* str = JS_ToCString(ctx, argv[0]);
        if (str) {
            std::cout << "[JS] " << str << std::endl;
            JS_FreeCString(ctx, str);
        }
    }
    return JS_UNDEFINED;
}

static JSValue js_send_to_ui(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_FALSE;
    
    const char* event = JS_ToCString(ctx, argv[0]);
    const char* data = JS_ToCString(ctx, argv[1]);
    
    if (event && data && g_webview_ptr) {
        std::string evt(event);
        std::string json(data);
        
        std::string escaped;
        for (char c : json) {
            if (c == '\\') escaped += "\\\\";
            else if (c == '\'') escaped += "\\'";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else escaped += c;
        }
        
        std::string js = "if(window." + evt + "){window." + evt + "('" + escaped + "');}";
        
        g_webview_ptr->dispatch([js]() {
            if (g_webview_ptr) {
                g_webview_ptr->eval(js);
            }
        });
    }
    
    if (event) JS_FreeCString(ctx, event);
    if (data) JS_FreeCString(ctx, data);
    
    return JS_TRUE;
}

static void async_cb(uv_async_t* handle) {
    ipc_message msg;
    {
        std::lock_guard<std::mutex> lock(g_ipc_mutex);
        if (!g_ipc_queue.empty()) {
            msg = g_ipc_queue.front();
            g_ipc_queue.pop_front();
        } else {
            return;
        }
    }
    
    if (g_ctx) {
        JSValue global = JS_GetGlobalObject(g_ctx);
        JSValue handleCmd = JS_GetPropertyStr(g_ctx, global, "handleCommand");
        
        if (JS_IsFunction(g_ctx, handleCmd)) {
            JSValue obj = JS_ParseJSON(g_ctx, msg.data.c_str(), msg.data.length(), "<ipc>");
            if (!JS_IsException(obj)) {
                JSValue result = JS_Call(g_ctx, handleCmd, global, 1, &obj);
                if (JS_IsException(result)) {
                    JSValue exception = JS_GetException(g_ctx);
                    const char* err = JS_ToCString(g_ctx, exception);
                    if (err) {
                        std::cerr << "Backend error: " << err << std::endl;
                        JS_FreeCString(g_ctx, err);
                    }
                    JS_FreeValue(g_ctx, exception);
                }
                JS_FreeValue(g_ctx, result);
            }
            JS_FreeValue(g_ctx, obj);
        }
        
        JS_FreeValue(g_ctx, handleCmd);
        JS_FreeValue(g_ctx, global);
    }
}

class app {
public:
    app() : running_(false), rt_(nullptr), ctx_(nullptr) {}
    
    ~app() {
        stop();
    }
    
    void init() {
        stop_requested_.store(false);
        logic_thread_ = std::thread([this]() {
            this->logic_thread_main();
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void load_script(const std::string& code, const std::string& filename = "<eval>") {
        int retries = 50;
        while (!ctx_ && retries-- > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (!ctx_) return;
        
        JSValue result = JS_Eval(ctx_, code.c_str(), code.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(ctx_);
            const char* err = JS_ToCString(ctx_, exception);
            if (err) {
                if (g_webview_ptr) {
                    std::string msg = "JS Error: " + std::string(err);
                    g_webview_ptr->dispatch([msg]() {
                        if (g_webview_ptr) {
                            g_webview_ptr->eval("console.error('" + msg + "');");
                        }
                    });
                }
                JS_FreeCString(ctx_, err);
            }
            JS_FreeValue(ctx_, exception);
        }
        JS_FreeValue(ctx_, result);
    }
    
    void load_from_vfs(const std::string& path) {
        auto file = vfs::instance().read_file(path);
        if (!file) return;
        
        std::string code((char*)file->data.data(), file->data.size());
        load_script(code, path);
    }
    
    void eval(const std::string& code) {
        load_script(code, "<eval>");
    }
    
    webview::webview& webview() { return *webview_; }
    
    void run(const std::string& title = "Valkyrie App", int width = 1280, int height = 720) {
#ifndef _WIN32
        setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", 1);
        setenv("WEBKIT_DISABLE_DMABUF_RENDERER", "1", 1);
#endif
        
        webview_ = std::make_unique<webview::webview>(true, nullptr);
        g_webview_ptr = webview_.get();
        
        webview_->set_title(title);
        webview_->set_size(width, height, WEBVIEW_HINT_NONE);
        
        webview_->init(R"js(
            document.addEventListener('click', function(e) {
                let el = e.target;
                while(el && el.tagName !== 'A') el = el.parentElement;
                if(el && el.tagName === 'A' && el.href && !el.href.startsWith('javascript:')) {
                    e.preventDefault();
                    e.stopPropagation();
                    window.open(el.href);
                }
            }, true);
        )js");
        
        if (!pending_html_.empty()) {
            webview_->set_html(pending_html_);
        }
        
        webview_->bind("native_send", [this](std::string req) -> std::string {
            std::string json_str = req;
            if (req.size() > 4 && req[0] == '[' && req[1] == '"') {
                size_t start = req.find('"') + 1;
                size_t end = req.rfind('"');
                if (start < end) {
                    json_str = req.substr(start, end - start);
                    std::string unescaped;
                    for (size_t i = 0; i < json_str.length(); i++) {
                        if (json_str[i] == '\\' && i + 1 < json_str.length()) {
                            if (json_str[i+1] == '"') {
                                unescaped += '"';
                                i++;
                            } else if (json_str[i+1] == '\\') {
                                unescaped += '\\';
                                i++;
                            } else {
                                unescaped += json_str[i];
                            }
                        } else {
                            unescaped += json_str[i];
                        }
                    }
                    json_str = unescaped;
                }
            }
            
            handle_builtin_command(json_str);
            
            {
                std::lock_guard<std::mutex> lock(g_ipc_mutex);
                g_ipc_queue.push_back({json_str, 0});
            }
            uv_async_send(&g_async_handle);
            
            return "{}";
        });
        
        if (pending_html_.empty()) {
            const char* default_html = R"html(
<!DOCTYPE html>
<html>
<head>
    <style>
        body { margin: 0; padding: 20px; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace; 
               background: #1e1e1e; color: #d4d4d4; }
        h1 { color: #569cd6; }
        .status { color: #4ec9b0; }
    </style>
</head>
<body>
    <h1>Valkyrie Engine</h1>
    <p class="status">Runtime initialized</p>
    <script>
        console.log('Valkyrie Engine ready');
    </script>
</body>
</html>
            )html";
            webview_->set_html(default_html);
        }
        
        running_ = true;
        webview_->run();
        running_ = false;
    }
    
    void set_html(const std::string& html) {
        std::string injected_html = html;
        
        const char* valkyrie_api = R"api(
<script>
window.valkyrie = {
    send(data) {
        window.native_send(JSON.stringify(data));
    },
    dialog: {
        showMessageBox(options) {
            const title = options.title || 'Message';
            const message = options.message || '';
            window.native_send(JSON.stringify({
                command: 'dialog',
                message: title + '\\n\\n' + message
            }));
        },
        showOpenDialog(options) {
            window.native_send(JSON.stringify({ command: 'open_file' }));
        },
        showSaveDialog(options) {
            window.native_send(JSON.stringify({ command: 'save_file' }));
        }
    },
    notification: {
        show(title, body) {
            window.native_send(JSON.stringify({
                command: 'notify',
                message: title + ': ' + body
            }));
        }
    },
    clipboard: {
        writeText(text) {
            window.native_send(JSON.stringify({
                command: 'clipboard_write',
                text: text
            }));
        },
        readText() {
            window.native_send(JSON.stringify({ command: 'clipboard_read' }));
        }
    },
    window: {
        setTitle(title) {
            document.title = title;
        }
    },
    version: '1.0.0',
    platform: 'linux'
};
window.open = function(url) {
    window.native_send(JSON.stringify({ command: 'open_url', url: url }));
};
console.log('Valkyrie API loaded');
</script>
)api";
        
        size_t head_pos = injected_html.find("</head>");
        if (head_pos != std::string::npos) {
            injected_html.insert(head_pos, valkyrie_api);
        } else {
            size_t body_pos = injected_html.find("<body>");
            if (body_pos != std::string::npos) {
                injected_html.insert(body_pos, valkyrie_api);
            }
        }
        
        pending_html_ = injected_html;
        if (webview_) {
            webview_->set_html(injected_html);
        }
    }
    
    void stop() {
        running_ = false;
        stop_requested_.store(true);
        if (logic_thread_.joinable()) {
            logic_thread_.join();
        }
        if (ctx_) {
            JS_RunGC(rt_);
            JS_FreeContext(ctx_);
            ctx_ = nullptr;
        }
        if (rt_) {
            JS_RunGC(rt_);
            JS_FreeRuntime(rt_);
            rt_ = nullptr;
        }
    }

private:
    void logic_thread_main() {
        uv_loop_t* loop = uv_default_loop();
        
        uv_async_init(loop, &g_async_handle, async_cb);
        
        rt_ = JS_NewRuntime();
        ctx_ = JS_NewContext(rt_);
        g_ctx = ctx_;
        
        JS_NewClassID(&js_socket_class_id);
        JS_NewClass(rt_, js_socket_class_id, &js_socket_class);
        
        JSValue global = JS_GetGlobalObject(ctx_);
        
        JSValue socket_ctor = JS_NewCFunction2(ctx_, js_socket_new, "NativeSocket", 0, JS_CFUNC_constructor, 0);
        JSValue proto = JS_NewObject(ctx_);
        JS_SetPropertyStr(ctx_, proto, "connect", JS_NewCFunction(ctx_, js_socket_connect, "connect", 2));
        JS_SetPropertyStr(ctx_, proto, "write", JS_NewCFunction(ctx_, js_socket_write, "write", 1));
        JS_SetPropertyStr(ctx_, proto, "setOnConnect", JS_NewCFunction(ctx_, js_socket_set_on_connect, "setOnConnect", 1));
        JS_SetPropertyStr(ctx_, proto, "setOnData", JS_NewCFunction(ctx_, js_socket_set_on_data, "setOnData", 1));
        JS_SetPropertyStr(ctx_, proto, "setOnError", JS_NewCFunction(ctx_, js_socket_set_on_error, "setOnError", 1));
        JS_SetPropertyStr(ctx_, proto, "setOnClose", JS_NewCFunction(ctx_, js_socket_set_on_close, "setOnClose", 1));
        JS_SetConstructorBit(ctx_, socket_ctor, 1);
        JS_SetClassProto(ctx_, js_socket_class_id, proto);
        JS_SetPropertyStr(ctx_, global, "NativeSocket", socket_ctor);
        
        JS_SetPropertyStr(ctx_, global, "setTimeout", JS_NewCFunction(ctx_, js_set_timeout, "setTimeout", 2));
        JS_SetPropertyStr(ctx_, global, "require", JS_NewCFunction(ctx_, js_require, "require", 1));
        JS_SetPropertyStr(ctx_, global, "native_print", JS_NewCFunction(ctx_, js_native_print, "native_print", 1));
        JS_SetPropertyStr(ctx_, global, "sendToUI", JS_NewCFunction(ctx_, js_send_to_ui, "sendToUI", 2));
        JS_SetPropertyStr(ctx_, global, "buffer_alloc", JS_NewCFunction(ctx_, js_buffer_alloc, "buffer_alloc", 1));
        JS_SetPropertyStr(ctx_, global, "buffer_from_bytes", JS_NewCFunction(ctx_, js_buffer_from_bytes, "buffer_from_bytes", 1));
        
        JSValue clipboard_obj = JS_NewObject(ctx_);
        JS_SetPropertyStr(ctx_, clipboard_obj, "read", JS_NewCFunction(ctx_, js_clipboard_read, "read", 0));
        JS_SetPropertyStr(ctx_, clipboard_obj, "write", JS_NewCFunction(ctx_, js_clipboard_write, "write", 1));
        JS_SetPropertyStr(ctx_, global, "clipboard", clipboard_obj);
        
        JSValue dialog_obj = JS_NewObject(ctx_);
        JS_SetPropertyStr(ctx_, dialog_obj, "showMessage", JS_NewCFunction(ctx_, js_show_message_box, "showMessage", 2));
        JS_SetPropertyStr(ctx_, dialog_obj, "showOpen", JS_NewCFunction(ctx_, js_show_open_dialog, "showOpen", 1));
        JS_SetPropertyStr(ctx_, dialog_obj, "showFolder", JS_NewCFunction(ctx_, js_show_folder_dialog, "showFolder", 1));
        JS_SetPropertyStr(ctx_, dialog_obj, "showSave", JS_NewCFunction(ctx_, js_show_save_dialog, "showSave", 1));
        JS_SetPropertyStr(ctx_, global, "dialog", dialog_obj);
        
        JS_SetPropertyStr(ctx_, global, "notify", JS_NewCFunction(ctx_, js_show_notification, "notify", 2));
        JS_SetPropertyStr(ctx_, global, "systemInfo", JS_NewCFunction(ctx_, js_system_info, "systemInfo", 0));
        
        JSValue fs_obj = JS_NewObject(ctx_);
        JS_SetPropertyStr(ctx_, fs_obj, "readFile", JS_NewCFunction(ctx_, js_fs_read_file, "readFile", 1));
        JS_SetPropertyStr(ctx_, fs_obj, "writeFile", JS_NewCFunction(ctx_, js_fs_write_file, "writeFile", 2));
        JS_SetPropertyStr(ctx_, fs_obj, "exists", JS_NewCFunction(ctx_, js_fs_exists, "exists", 1));
        JS_SetPropertyStr(ctx_, fs_obj, "listDir", JS_NewCFunction(ctx_, js_fs_list_dir, "listDir", 1));
        JS_SetPropertyStr(ctx_, fs_obj, "isDir", JS_NewCFunction(ctx_, js_fs_is_dir, "isDir", 1));
        JS_SetPropertyStr(ctx_, fs_obj, "mkdir", JS_NewCFunction(ctx_, js_fs_mkdir, "mkdir", 1));
        JS_SetPropertyStr(ctx_, fs_obj, "unlink", JS_NewCFunction(ctx_, js_fs_unlink, "unlink", 1));
        JS_SetPropertyStr(ctx_, fs_obj, "rmdir", JS_NewCFunction(ctx_, js_fs_rmdir, "rmdir", 1));
        JS_SetPropertyStr(ctx_, fs_obj, "cwd", JS_NewCFunction(ctx_, js_fs_cwd, "cwd", 0));
        JS_SetPropertyStr(ctx_, fs_obj, "chdir", JS_NewCFunction(ctx_, js_fs_chdir, "chdir", 1));
        JS_SetPropertyStr(ctx_, global, "fs", fs_obj);
        
        JSValue path_obj = JS_NewObject(ctx_);
        JS_SetPropertyStr(ctx_, path_obj, "join", JS_NewCFunction(ctx_, js_path_join, "join", 2));
        JS_SetPropertyStr(ctx_, path_obj, "dirname", JS_NewCFunction(ctx_, js_path_dirname, "dirname", 1));
        JS_SetPropertyStr(ctx_, path_obj, "basename", JS_NewCFunction(ctx_, js_path_basename, "basename", 1));
        JS_SetPropertyStr(ctx_, path_obj, "extname", JS_NewCFunction(ctx_, js_path_extname, "extname", 1));
        JS_SetPropertyStr(ctx_, path_obj, "sep", JS_NewString(ctx_, "/"));
        JS_SetPropertyStr(ctx_, global, "path", path_obj);
        
        JSValue os_obj = JS_NewObject(ctx_);
        JS_SetPropertyStr(ctx_, os_obj, "homedir", JS_NewCFunction(ctx_, js_os_homedir, "homedir", 0));
        JS_SetPropertyStr(ctx_, os_obj, "tmpdir", JS_NewCFunction(ctx_, js_os_tmpdir, "tmpdir", 0));
        JS_SetPropertyStr(ctx_, os_obj, "env", JS_NewCFunction(ctx_, js_os_env, "env", 1));
        JS_SetPropertyStr(ctx_, os_obj, "platform", JS_NewString(ctx_, "linux"));
        JS_SetPropertyStr(ctx_, global, "os", os_obj);
        
        JSValue cp_obj = JS_NewObject(ctx_);
        JS_SetPropertyStr(ctx_, cp_obj, "exec", JS_NewCFunction(ctx_, js_exec, "exec", 1));
        JS_SetPropertyStr(ctx_, cp_obj, "spawn", JS_NewCFunction(ctx_, js_spawn, "spawn", 1));
        JS_SetPropertyStr(ctx_, global, "child_process", cp_obj);
        
        JSValue runtime_result = JS_Eval(ctx_, RUNTIME_JS, strlen(RUNTIME_JS), "<runtime>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(runtime_result)) {
            JSValue exception = JS_GetException(ctx_);
            const char* err = JS_ToCString(ctx_, exception);
            if (err) {
                JS_FreeCString(ctx_, err);
            }
            JS_FreeValue(ctx_, exception);
        }
        JS_FreeValue(ctx_, runtime_result);
        JS_FreeValue(ctx_, global);
        
        running_ = true;
        while (!stop_requested_.load() && running_) {
            uv_run(loop, UV_RUN_NOWAIT);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        uv_close((uv_handle_t*)&g_async_handle, nullptr);
        uv_run(loop, UV_RUN_DEFAULT);
        uv_loop_close(loop);
    }
    
    std::thread logic_thread_;
    std::unique_ptr<webview::webview> webview_;
    std::atomic<bool> running_;
    std::atomic<bool> stop_requested_;
    std::string pending_html_;
    
    JSRuntime* rt_;
    JSContext* ctx_;
};

} // namespace valkyrie
