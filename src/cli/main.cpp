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

#include "src/cli/utils.hpp"
#include "src/cli/templates.hpp"
#include "src/cli/commands.hpp"
#include "src/cli/package.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

const char* VERSION = "1.0.0";

void print_help() {
    std::cout << R"(
Valkyrie - Native Desktop Application Framework

Usage:
    valkyrie <command> [options]

Commands:
    init [name]         Create a new project
    dev                 Start development server
    build [--target]    Build production binary
    package [--target]  Create distribution package
    run                 Execute built application
    version             Display version
    help                Display this help message

Flags:
    --target=linux      Target Linux (.deb/.rpm/.pkg)
    --target=windows    Target Windows (.exe)
    --target=macos      Target macOS (.app/.dmg)

Examples:
    valkyrie init my-app
    cd my-app && npm install && valkyrie dev

)" << std::endl;
}

void print_version() {
    std::cout << "valkyrie v" << VERSION << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }
    
    std::string cmd = argv[1];
    
    if (cmd == "init") {
        std::string name = "my-app";
        if (argc > 2) {
            name = argv[2];
        }
        create_project(name);
    } else if (cmd == "dev") {
        run_dev();
    } else if (cmd == "build") {
        std::string target = get_platform();
        if (argc > 2) {
            std::string arg = argv[2];
            if (arg.find("--target=") == 0) {
                target = arg.substr(9);
            }
        }
        
        build_app(target);
    } else if (cmd == "package") {
        std::string target = "";
        if (argc > 2) {
            std::string arg = argv[2];
            if (arg.find("--target=") == 0) {
                target = arg.substr(9);
                std::cout << "packaging for target: " << target << "\n" << std::endl;
            }
        }
        package_app(target, build_app);
    } else if (cmd == "run") {
        if (fs::exists("app")) {
            system("./app");
        } else {
            print_error("no built app found", "run 'valkyrie build' first");
        }
    } else if (cmd == "version" || cmd == "-v" || cmd == "--version") {
        print_version();
    } else if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        print_help();
    } else {
        std::cerr << "unknown command: " << cmd << std::endl;
        std::cerr << "run 'valkyrie help' for usage" << std::endl;
        return 1;
    }
    
    return 0;
}
