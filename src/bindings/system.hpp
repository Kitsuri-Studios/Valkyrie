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
    #include <shlobj.h>
    #define popen _popen
    #define pclose _pclose
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <gtk/gtk.h>
#endif

namespace valkyrie {

class clipboard {
public:
    static std::string read_text() {
#ifdef _WIN32
        if (!OpenClipboard(nullptr)) return "";
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (!hData) {
            CloseClipboard();
            return "";
        }
        char* text = static_cast<char*>(GlobalLock(hData));
        if (!text) {
            CloseClipboard();
            return "";
        }
        std::string result(text);
        GlobalUnlock(hData);
        CloseClipboard();
        return result;
#else
        if (!gtk_init_check(nullptr, nullptr)) return "";
        GtkClipboard* cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        if (!cb) return "";
        gchar* text = gtk_clipboard_wait_for_text(cb);
        if (!text) return "";
        std::string result(text);
        g_free(text);
        return result;
#endif
    }
    
    static void write_text(const std::string& text) {
#ifdef _WIN32
        if (!OpenClipboard(nullptr)) return;
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
        if (!hMem) {
            CloseClipboard();
            return;
        }
        memcpy(GlobalLock(hMem), text.c_str(), text.length() + 1);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
#else
        if (!gtk_init_check(nullptr, nullptr)) return;
        GtkClipboard* cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        if (!cb) return;
        gtk_clipboard_set_text(cb, text.c_str(), -1);
        gtk_clipboard_store(cb);
#endif
    }
};

class dialogs {
public:
    static void show_message(const std::string& title, const std::string& message) {
#ifdef _WIN32
        MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
#else
        std::string cmd = "zenity --info --title=\"" + title + "\" --text=\"" + message + "\" 2>/dev/null &";
        system(cmd.c_str());
#endif
    }
    
    static std::string show_open_dialog(const std::string& title) {
#ifdef _WIN32
        char filename[MAX_PATH] = "";
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = title.c_str();
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameA(&ofn)) {
            return std::string(filename);
        }
        return "";
#else
        std::string cmd = "zenity --file-selection --title=\"" + title + "\" 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        
        char buffer[1024];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            result = buffer;
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }
        pclose(pipe);
        return result;
#endif
    }
    
    static std::string show_folder_dialog(const std::string& title) {
#ifdef _WIN32
        BROWSEINFOA bi = {};
        bi.lpszTitle = title.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if (pidl) {
            char path[MAX_PATH];
            if (SHGetPathFromIDListA(pidl, path)) {
                CoTaskMemFree(pidl);
                return std::string(path);
            }
            CoTaskMemFree(pidl);
        }
        return "";
#else
        std::string cmd = "zenity --file-selection --directory --title=\"" + title + "\" 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        
        char buffer[1024];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            result = buffer;
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }
        pclose(pipe);
        return result;
#endif
    }
    
    static std::string show_save_dialog(const std::string& title) {
#ifdef _WIN32
        char filename[MAX_PATH] = "";
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = title.c_str();
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameA(&ofn)) {
            return std::string(filename);
        }
        return "";
#else
        std::string cmd = "zenity --file-selection --save --title=\"" + title + "\" 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        
        char buffer[1024];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            result = buffer;
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }
        pclose(pipe);
        return result;
#endif
    }
};

class notifications {
public:
    static void show(const std::string& title, const std::string& body) {
#ifdef _WIN32
        std::string msg = title + "\n\n" + body;
        MessageBoxA(nullptr, msg.c_str(), "Notification", MB_OK | MB_ICONINFORMATION);
#else
        std::string cmd = "notify-send \"" + title + "\" \"" + body + "\" 2>/dev/null &";
        system(cmd.c_str());
#endif
    }
};

} // namespace valkyrie
