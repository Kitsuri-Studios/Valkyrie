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

#include "utils.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

inline std::string cross_compile(const std::string& target, const std::string& runner_code) {
    std::cout << "Cross-compiling for " << target << "..." << std::endl;
    
    if (target == "windows") {
        int check = system("which x86_64-w64-mingw32-g++ > /dev/null 2>&1");
        if (check != 0) {
            print_error("mingw-w64 not found", "Install: sudo pacman -S mingw-w64-gcc (Arch) or sudo apt install mingw-w64 (Ubuntu)");
            return "";
        }
        
        std::cout << "Building for Windows with MinGW..." << std::endl;
        
        char* home = getenv("HOME");
        std::string valkyrie_dir = std::string(home) + "/valkyrie";
        std::string webview_dir = valkyrie_dir + "/_deps/webview-src";
        
        std::string quickjs_src = valkyrie_dir + "/quickjs";
        if (!fs::exists(quickjs_src)) {
            std::cout << "QuickJS source not found, downloading..." << std::endl;
            exec_cmd("cd " + valkyrie_dir + " && git clone https://github.com/bellard/quickjs.git", nullptr);
        }
        
        std::string quickjs_win = valkyrie_dir + "/quickjs/libquickjs.win.a";
        if (!fs::exists(quickjs_win)) {
            std::cout << "Building QuickJS for Windows..." << std::endl;
            
            std::string qjs_build = "cd " + quickjs_src + " && ";
            qjs_build += "make clean && ";
            qjs_build += "make CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar libquickjs.a && ";
            qjs_build += "mv libquickjs.a libquickjs.win.a";
            
            int exit_code = 0;
            std::string output = exec_cmd(qjs_build, &exit_code);
            
            if (exit_code != 0) {
                print_error("QuickJS Windows build failed", output);
                return "";
            }
        }
        
        std::string libuv_src = valkyrie_dir + "/libuv";
        std::string libuv_win = valkyrie_dir + "/libuv/libuv.win.a";
        
        if (!fs::exists(libuv_src)) {
            std::cout << "libuv source not found, downloading..." << std::endl;
            exec_cmd("cd " + valkyrie_dir + " && git clone https://github.com/libuv/libuv.git", nullptr);
        }
        
        if (!fs::exists(libuv_win)) {
            std::cout << "Building libuv for Windows..." << std::endl;
            
            std::string uv_build = "cd " + libuv_src + " && ";
            
            uv_build += "x86_64-w64-mingw32-gcc -c -O2 -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0600 ";
            uv_build += "-Iinclude -Isrc src/*.c src/win/*.c 2>/dev/null && ";
            
            uv_build += "x86_64-w64-mingw32-ar rcs libuv.win.a *.o && ";
            
            uv_build += "rm -f *.o";
            
            int exit_code = 0;
            std::string output = exec_cmd(uv_build, &exit_code);
            
            if (exit_code != 0) {
                print_error("libuv Windows build failed", output);
                return "";
            }
        }
        
        std::string webview2_dir = valkyrie_dir + "/webview2";
        if (!fs::exists(webview2_dir + "/WebView2.h")) {
            std::cout << "WebView2 SDK not found, downloading..." << std::endl;
            
            std::string dl_cmd = "cd " + valkyrie_dir + " && ";
            dl_cmd += "curl -L https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2535.41 -o webview2.zip && ";
            dl_cmd += "unzip -q webview2.zip -d webview2_tmp && ";
            dl_cmd += "mkdir -p webview2 && ";
            dl_cmd += "cp -r webview2_tmp/build/native/include/* webview2/ 2>/dev/null || ";
            dl_cmd += "find webview2_tmp -name '*.h' -exec cp {} webview2/ \\; && ";
            dl_cmd += "rm -rf webview2_tmp webview2.zip";
            
            exec_cmd(dl_cmd, nullptr);
            
            if (!fs::exists(webview2_dir + "/EventToken.h")) {
                std::string eventtoken = "#pragma once\n";
                eventtoken += "#include <windows.h>\n";
                eventtoken += "typedef struct EventRegistrationToken { __int64 value; } EventRegistrationToken;\n";
                write_file(webview2_dir + "/EventToken.h", eventtoken);
            }
        }
        
        write_file("_build.cpp", runner_code);
        
        std::string compile_cmd = "x86_64-w64-mingw32-g++ -std=c++20 -O2 -o app.exe _build.cpp ";
        compile_cmd += "-I" + valkyrie_dir + " ";
        compile_cmd += "-I" + valkyrie_dir + "/src ";
        compile_cmd += "-I" + webview_dir + " ";
        compile_cmd += "-I" + quickjs_src + " ";
        compile_cmd += "-I" + libuv_src + "/include ";
        compile_cmd += "-I" + webview2_dir + " ";
        compile_cmd += "-DWEBVIEW_EDGE ";
        compile_cmd += quickjs_win + " ";
        compile_cmd += libuv_win + " ";
        compile_cmd += "-lole32 -lcomctl32 -loleaut32 -luuid -lgdi32 -lws2_32 -liphlpapi -lpsapi -luserenv -ladvapi32 ";
        compile_cmd += "-lcomdlg32 -lshlwapi -ldbghelp -lshell32 ";
        compile_cmd += "-mwindows ";
        compile_cmd += "-static-libgcc -static-libstdc++ -static -lpthread 2>&1";
        
        int exit_code = 0;
        std::string output = exec_cmd(compile_cmd, &exit_code);
        
        fs::remove("_build.cpp");
        
        if (exit_code != 0) {
            print_error("Windows cross-compilation failed", output);
            return "";
        }
        
        exec_cmd("x86_64-w64-mingw32-strip app.exe", nullptr);
        
        std::cout << "Windows binary created: app.exe" << std::endl;
        return "app.exe";
        
    } else if (target == "macos") {
        char* osxcross = getenv("OSXCROSS_TARGET_DIR");
        if (!osxcross) {
            print_error("osxcross not found", 
                "setup osxcross first:\n"
                "git clone https://github.com/tpoechtrager/osxcross\n"
                "follow instructions to build toolchain\n"
                "export OSXCROSS_TARGET_DIR=/path/to/osxcross/target");
            return "";
        }
        
        write_file("_build.cpp", runner_code);
        
        std::string osxcross_dir = osxcross;
        std::string compile_cmd = osxcross_dir + "/bin/o64-clang++ -std=c++20 -O2 -o app _build.cpp ";
        compile_cmd += "-framework WebKit -framework Cocoa ";
        compile_cmd += "-I" + osxcross_dir + "/SDK/MacOSX.sdk/usr/include ";
        
        int exit_code = 0;
        std::string output = exec_cmd(compile_cmd, &exit_code);
        
        fs::remove("_build.cpp");
        
        if (exit_code != 0) {
            print_error("macos cross-compile failed", output);
            return "";
        }
        
        return "app";
        
    } else if (target == "linux") {
        return "";
    }
    
    return "";
}
