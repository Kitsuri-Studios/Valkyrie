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

#include <quickjs/quickjs.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    #define popen _popen
    #define pclose _pclose
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

namespace valkyrie {

class fs_ops {
public:
    static std::string read_text_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    
    static std::vector<uint8_t> read_binary_file(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return {};
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    
    static bool write_text_file(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << content;
        return true;
    }
    
    static bool write_binary_file(const std::string& path, const uint8_t* data, size_t len) {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        file.write((const char*)data, len);
        return true;
    }
    
    static bool file_exists(const std::string& path) {
        return std::filesystem::exists(path);
    }
    
    static std::vector<std::string> list_directory(const std::string& path) {
        std::vector<std::string> result;
        if (!std::filesystem::exists(path)) return result;
        
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            result.push_back(entry.path().filename().string());
        }
        return result;
    }
    
    static bool is_directory(const std::string& path) {
        return std::filesystem::is_directory(path);
    }
    
    static bool mkdir(const std::string& path) {
        return std::filesystem::create_directories(path);
    }
    
    static bool remove_file(const std::string& path) {
        return std::filesystem::remove(path);
    }
    
    static bool remove_dir(const std::string& path) {
        return std::filesystem::remove_all(path) > 0;
    }
    
    static std::string get_cwd() {
        return std::filesystem::current_path().string();
    }
    
    static bool set_cwd(const std::string& path) {
        try {
            std::filesystem::current_path(path);
            return true;
        } catch (...) {
            return false;
        }
    }
};

static JSValue js_fs_read_file(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    
    auto content = fs_ops::read_text_file(path);
    JS_FreeCString(ctx, path);
    
    if (content.empty()) {
        return JS_NULL;
    }
    
    return JS_NewString(ctx, content.c_str());
}

static JSValue js_fs_write_file(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    const char* content = JS_ToCString(ctx, argv[1]);
    
    if (!path || !content) {
        if (path) JS_FreeCString(ctx, path);
        if (content) JS_FreeCString(ctx, content);
        return JS_EXCEPTION;
    }
    
    bool success = fs_ops::write_text_file(path, content);
    
    JS_FreeCString(ctx, path);
    JS_FreeCString(ctx, content);
    
    return JS_NewBool(ctx, success);
}

static JSValue js_fs_exists(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    
    bool exists = fs_ops::file_exists(path);
    JS_FreeCString(ctx, path);
    
    return JS_NewBool(ctx, exists);
}

static JSValue js_fs_list_dir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    
    auto files = fs_ops::list_directory(path);
    JS_FreeCString(ctx, path);
    
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < files.size(); i++) {
        JS_SetPropertyUint32(ctx, arr, i, JS_NewString(ctx, files[i].c_str()));
    }
    
    return arr;
}

static JSValue js_fs_is_dir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    bool isDir = fs_ops::is_directory(path);
    JS_FreeCString(ctx, path);
    return JS_NewBool(ctx, isDir);
}

static JSValue js_fs_mkdir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    bool success = fs_ops::mkdir(path);
    JS_FreeCString(ctx, path);
    return JS_NewBool(ctx, success);
}

static JSValue js_fs_unlink(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    bool success = fs_ops::remove_file(path);
    JS_FreeCString(ctx, path);
    return JS_NewBool(ctx, success);
}

static JSValue js_fs_rmdir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    bool success = fs_ops::remove_dir(path);
    JS_FreeCString(ctx, path);
    return JS_NewBool(ctx, success);
}

static JSValue js_fs_cwd(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    return JS_NewString(ctx, fs_ops::get_cwd().c_str());
}

static JSValue js_fs_chdir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    bool success = fs_ops::set_cwd(path);
    JS_FreeCString(ctx, path);
    return JS_NewBool(ctx, success);
}

static JSValue js_path_join(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    std::filesystem::path result;
    for (int i = 0; i < argc; i++) {
        const char* part = JS_ToCString(ctx, argv[i]);
        if (part) {
            if (result.empty()) {
                result = part;
            } else {
                result /= part;
            }
            JS_FreeCString(ctx, part);
        }
    }
    return JS_NewString(ctx, result.string().c_str());
}

static JSValue js_path_dirname(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    std::filesystem::path p(path);
    JS_FreeCString(ctx, path);
    return JS_NewString(ctx, p.parent_path().string().c_str());
}

static JSValue js_path_basename(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    std::filesystem::path p(path);
    JS_FreeCString(ctx, path);
    return JS_NewString(ctx, p.filename().string().c_str());
}

static JSValue js_path_extname(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_EXCEPTION;
    std::filesystem::path p(path);
    JS_FreeCString(ctx, path);
    return JS_NewString(ctx, p.extension().string().c_str());
}

} // namespace valkyrie
