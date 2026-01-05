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

#include "../bindings/system.hpp"

#ifndef VALKYRIE_NO_WEBVIEW
    #include <core/include/webview.h>
#endif
#include <string>
#include <deque>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
#endif

namespace valkyrie {

struct ipc_message {
    std::string data;
    uint64_t timestamp;
};

extern std::deque<ipc_message> g_ipc_queue;
extern std::mutex g_ipc_mutex;
extern webview::webview* g_webview_ptr;

inline void handle_builtin_command(const std::string& json_str) {
    auto get_value = [&](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":";
        size_t pos = json_str.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        
        while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '"')) pos++;
        if (json_str[pos-1] != '"') return "";
        
        size_t end = json_str.find('"', pos);
        if (end == std::string::npos) return "";
        return json_str.substr(pos, end - pos);
    };
    
    std::string command = get_value("command");
    
    if (command == "dialog") {
        std::string message = get_value("message");
        dialogs::show_message("Message", message);
    }
    else if (command == "open_file") {
        std::string result = dialogs::show_open_dialog("Open File");
        if (!result.empty() && g_webview_ptr) {
            std::string js = "if(window.onFileOpen){window.onFileOpen('" + result + "');}";
            g_webview_ptr->dispatch([js]() {
                if (g_webview_ptr) g_webview_ptr->eval(js);
            });
        }
    }
    else if (command == "open_folder") {
        std::string result = dialogs::show_folder_dialog("Open Folder");
        if (!result.empty() && g_webview_ptr) {
            std::string js = "if(window.onFolderOpen){window.onFolderOpen('" + result + "');}";
            g_webview_ptr->dispatch([js]() {
                if (g_webview_ptr) g_webview_ptr->eval(js);
            });
        }
    }
    else if (command == "save_file") {
        std::string result = dialogs::show_save_dialog("Save File");
        if (!result.empty() && g_webview_ptr) {
            std::string js = "if(window.onFileSave){window.onFileSave('" + result + "');}";
            g_webview_ptr->dispatch([js]() {
                if (g_webview_ptr) g_webview_ptr->eval(js);
            });
        }
    }
    else if (command == "notify") {
        std::string message = get_value("message");
        size_t colon = message.find(':');
        if (colon != std::string::npos) {
            notifications::show(message.substr(0, colon), message.substr(colon + 2));
        } else {
            notifications::show("Notification", message);
        }
    }
    else if (command == "clipboard_write") {
        std::string text = get_value("text");
        clipboard::write_text(text);
    }
    else if (command == "clipboard_read") {
        std::string text = clipboard::read_text();
        if (g_webview_ptr) {
            std::string escaped;
            for (char c : text) {
                if (c == '\\') escaped += "\\\\";
                else if (c == '\'') escaped += "\\'";
                else if (c == '\n') escaped += "\\n";
                else escaped += c;
            }
            std::string js = "if(window.onClipboardRead){window.onClipboardRead('" + escaped + "');}";
            g_webview_ptr->dispatch([js]() {
                if (g_webview_ptr) g_webview_ptr->eval(js);
            });
        }
    }
    else if (command == "open_url") {
        std::string url = get_value("url");
        if (!url.empty()) {
#ifdef _WIN32
            ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif __APPLE__
            std::string cmd = "open \"" + url + "\"";
            system(cmd.c_str());
#else
            std::string cmd = "xdg-open \"" + url + "\" &";
            system(cmd.c_str());
#endif
        }
    }
}

} // namespace valkyrie
