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

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdio>

#ifndef _WIN32
    #include <sys/wait.h>
#endif

namespace fs = std::filesystem;

inline std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

inline void write_file(const std::string& path, const std::string& content) {
    auto parent = fs::path(path).parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent);
    }
    std::ofstream f(path);
    f << content;
}

inline std::map<std::string, std::string> load_env(const std::string& path = ".env") {
    std::map<std::string, std::string> env;
    if (!fs::exists(path)) return env;
    
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        auto eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            env[key] = val;
        }
    }
    return env;
}

inline void print_error(const std::string& msg, const std::string& detail = "") {
    std::cerr << "\n  error: " << msg << std::endl;
    if (!detail.empty()) {
        std::cerr << "\ndetails:\n" << detail << std::endl;
    }
    std::cerr << std::endl;
}

inline std::string exec_cmd(const std::string& cmd, int* exit_code = nullptr) {
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) {
        if (exit_code) *exit_code = -1;
        return "failed to run command";
    }
    
    std::string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    
    int status = pclose(pipe);
#ifdef _WIN32
    if (exit_code) *exit_code = status;
#else
    if (exit_code) *exit_code = WEXITSTATUS(status);
#endif
    return result;
}

inline std::string get_platform() {
#ifdef __linux__
    return "linux";
#elif __APPLE__
    return "macos";
#elif _WIN32
    return "windows";
#else
    return "unknown";
#endif
}
