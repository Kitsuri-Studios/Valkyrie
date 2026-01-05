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
#include "cross_compile.hpp"
#include "../core/app.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

inline void run_dev() {
    if (!fs::exists("index.html")) {
        print_error("index.html not found", "Ensure you are in a Valkyrie project directory.");
        return;
    }
    
    std::cout << "Starting development server...\n" << std::endl;
    
    auto env = load_env();
    if (!env.empty()) {
        std::cout << "Loaded " << env.size() << " environment variables from .env" << std::endl;
        for (const auto& [k, v] : env) {
#ifdef _WIN32
            _putenv_s(k.c_str(), v.c_str());
#else
            setenv(k.c_str(), v.c_str(), 1);
#endif
        }
    }
    
    bool has_npm = fs::exists("package.json");
    
    if (has_npm) {
        if (!fs::exists("node_modules")) {
            std::cout << "Installing dependencies..." << std::endl;
            int exit_code = 0;
            std::string output = exec_cmd("npm install", &exit_code);
            if (exit_code != 0) {
                print_error("npm install failed", output);
                return;
            }
        }
        
        std::cout << "Bundling..." << std::endl;
        fs::create_directories("dist");
        
        std::string entry = "app.js";
        if (fs::exists("app.ts")) entry = "app.ts";
        else if (fs::exists("app.tsx")) entry = "app.tsx";
        else if (fs::exists("app.jsx")) entry = "app.jsx";
        
        std::string bundle_cmd = "npx esbuild " + entry + 
            " --bundle --outfile=dist/bundle.js" +
            " --platform=browser --format=iife" +
            " --loader:.ts=ts --loader:.tsx=tsx --loader:.jsx=jsx" +
            " --loader:.css=css --loader:.scss=css --loader:.sass=css" +
            " --sourcemap";
        
        int exit_code = 0;
        std::string output = exec_cmd(bundle_cmd, &exit_code);
        
        if (exit_code != 0) {
            print_error("Bundling failed", output);
            return;
        }
        
        if (fs::exists("dist/bundle.css")) {
            std::cout << "CSS output found, will be inlined." << std::endl;
        }
    }
    
    std::string html = read_file("index.html");
    std::string js;
    
    if (has_npm) {
        js = read_file("dist/bundle.js");
    } else {
        js = read_file("app.js");
    }
    
    if (html.empty()) {
        print_error("Failed to read index.html");
        return;
    }
    
    std::string css;
    if (has_npm && fs::exists("dist/bundle.css")) {
        css = read_file("dist/bundle.css");
    }
    
    if (!css.empty()) {
        size_t pos = html.find("</head>");
        if (pos != std::string::npos) {
            html.insert(pos, "<style>" + css + "</style>");
        }
    }
    
    if (!js.empty()) {
        size_t pos = html.find("</body>");
        if (pos != std::string::npos) {
            html.insert(pos, "<script>" + js + "</script>");
        }
    }
    
    std::cout << "Launching application...\n" << std::endl;
    
    try {
        valkyrie::app application;
        application.init();
        application.set_html(html);
        application.run("valkyrie dev", 1024, 768);
    } catch (const std::exception& e) {
        print_error("Runtime error", e.what());
    }
}

inline void build_app(const std::string& target_platform = "") {
    if (!fs::exists("index.html")) {
        print_error("index.html not found");
        return;
    }
    
    std::string target = target_platform.empty() ? get_platform() : target_platform;
    bool is_cross = (target != get_platform());
    
    std::cout << "Building production binary...\n" << std::endl;
    
    auto env = load_env();
    if (!env.empty()) {
        std::cout << "Loaded " << env.size() << " environment variables." << std::endl;
        for (const auto& [k, v] : env) {
#ifdef _WIN32
            _putenv_s(k.c_str(), v.c_str());
#else
            setenv(k.c_str(), v.c_str(), 1);
#endif
        }
    }
    
    bool has_npm = fs::exists("package.json");
    
    if (has_npm) {
        if (!fs::exists("node_modules")) {
            std::cout << "Installing dependencies..." << std::endl;
            int exit_code = 0;
            std::string output = exec_cmd("npm install", &exit_code);
            if (exit_code != 0) {
                print_error("npm install failed", output);
                return;
            }
        }
        
        std::cout << "Bundling..." << std::endl;
        fs::create_directories("dist");
        
        std::string entry = "app.js";
        if (fs::exists("app.ts")) entry = "app.ts";
        else if (fs::exists("app.tsx")) entry = "app.tsx";
        else if (fs::exists("app.jsx")) entry = "app.jsx";
        
        std::string bundle_cmd = "npx esbuild " + entry + 
            " --bundle --outfile=dist/bundle.js" +
            " --platform=browser --format=iife" +
            " --loader:.ts=ts --loader:.tsx=tsx --loader:.jsx=jsx" +
            " --loader:.css=css --loader:.scss=css --loader:.sass=css" +
            " --minify";
        
        int exit_code = 0;
        std::string output = exec_cmd(bundle_cmd, &exit_code);
        
        if (exit_code != 0) {
            print_error("Bundling failed", output);
            return;
        }
    }
    
    std::string html = read_file("index.html");
    std::string js = has_npm ? read_file("dist/bundle.js") : read_file("app.js");
    std::string css;
    
    if (has_npm && fs::exists("dist/bundle.css")) {
        css = read_file("dist/bundle.css");
    }
    
    if (!css.empty()) {
        size_t pos = html.find("</head>");
        if (pos != std::string::npos) {
            html.insert(pos, "<style>" + css + "</style>");
        }
    }
    
    if (!js.empty()) {
        size_t pos = html.find("</body>");
        if (pos != std::string::npos) {
            html.insert(pos, "<script>" + js + "</script>");
        }
    }
    
    std::string runner = R"(#include "src/core/app.hpp"
using namespace valkyrie;

const char* HTML = R"HTML()" + html + R"()HTML";

int main() {
    app application;
    application.init();
    application.set_html(HTML);
    application.run("app", 1024, 768);
    return 0;
}
)";
    
    if (is_cross) {
        std::cout << "\ncross-compiling from " << get_platform() << " to " << target << "\n" << std::endl;
        std::string result = cross_compile(target, runner);
        
        if (result.empty()) {
            print_error("Cross-compilation failed");
            return;
        }
        
        std::cout << "\nBuild complete." << std::endl;
        std::cout << "Binary: ./" << result << std::endl;
        
        auto size = fs::file_size(result);
        std::cout << "Size: " << (size / 1024 / 1024) << " MB" << std::endl;
        
        return;
    }
    
    write_file("_build.cpp", runner);
    
    std::cout << "Compiling..." << std::endl;
    
    char* home = getenv("HOME");
    std::string valkyrie_dir = std::string(home) + "/valkyrie";
    
    std::string compile_cmd = "g++ -std=c++20 -O2 -o app _build.cpp "
        "-I" + valkyrie_dir + " "
        "-I" + valkyrie_dir + "/src "
        "-I" + valkyrie_dir + "/_deps/webview-src "
        "$(pkg-config --cflags --libs webkit2gtk-4.0 gtk+-3.0) "
        "-lpthread -luv /usr/lib/quickjs/libquickjs.a";
    
    int exit_code = 0;
    std::string output = exec_cmd(compile_cmd, &exit_code);
    
    if (exit_code == 0) {
        exec_cmd("strip app", nullptr);
        fs::remove("_build.cpp");
        
        std::cout << "\nBuild complete." << std::endl;
        std::cout << "Binary: ./app" << std::endl;
        
        auto size = fs::file_size("app");
        std::cout << "Size: " << (size / 1024 / 1024) << " MB" << std::endl;
    } else {
        print_error("Compilation failed", output);
        fs::remove("_build.cpp");
    }
}
