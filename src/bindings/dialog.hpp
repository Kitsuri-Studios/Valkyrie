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

#include "system.hpp"
#include <quickjs/quickjs.h>
#include <string>

namespace valkyrie {

static JSValue js_clipboard_read(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto text = clipboard::read_text();
    return JS_NewString(ctx, text.c_str());
}

static JSValue js_clipboard_write(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* text = JS_ToCString(ctx, argv[0]);
    if (!text) return JS_EXCEPTION;
    clipboard::write_text(text);
    JS_FreeCString(ctx, text);
    return JS_UNDEFINED;
}

static JSValue js_show_message_box(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* title = JS_ToCString(ctx, argv[0]);
    const char* message = JS_ToCString(ctx, argv[1]);
    if (!title || !message) return JS_EXCEPTION;
    
    dialogs::show_message(title, message);
    
    JS_FreeCString(ctx, title);
    JS_FreeCString(ctx, message);
    return JS_UNDEFINED;
}

static JSValue js_show_open_dialog(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* title = argc > 0 ? JS_ToCString(ctx, argv[0]) : "Open File";
    std::string result = dialogs::show_open_dialog(title);
    if (argc > 0) JS_FreeCString(ctx, title);
    return result.empty() ? JS_NULL : JS_NewString(ctx, result.c_str());
}

static JSValue js_show_folder_dialog(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* title = argc > 0 ? JS_ToCString(ctx, argv[0]) : "Select Folder";
    std::string result = dialogs::show_folder_dialog(title);
    if (argc > 0) JS_FreeCString(ctx, title);
    return result.empty() ? JS_NULL : JS_NewString(ctx, result.c_str());
}

static JSValue js_show_save_dialog(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* title = argc > 0 ? JS_ToCString(ctx, argv[0]) : "Save File";
    std::string result = dialogs::show_save_dialog(title);
    if (argc > 0) JS_FreeCString(ctx, title);
    return result.empty() ? JS_NULL : JS_NewString(ctx, result.c_str());
}

static JSValue js_show_notification(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* title = JS_ToCString(ctx, argv[0]);
    const char* body = argc > 1 ? JS_ToCString(ctx, argv[1]) : "";
    if (!title) return JS_EXCEPTION;
    
    notifications::show(title, body);
    
    JS_FreeCString(ctx, title);
    if (argc > 1) JS_FreeCString(ctx, body);
    return JS_UNDEFINED;
}

static JSValue js_open_external(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc == 0) return JS_FALSE;
    const char* url = JS_ToCString(ctx, argv[0]);
    if (!url) return JS_EXCEPTION;
    
    std::string cmd = "xdg-open \"" + std::string(url) + "\" 2>/dev/null &";
    int result = system(cmd.c_str());
    
    JS_FreeCString(ctx, url);
    return JS_NewBool(ctx, result == 0);
}

} // namespace valkyrie
