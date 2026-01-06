/*
 * Copyright 2026 Kitsuri Studios
 * Developed by Mostafizur Rahman (aeticusdev)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * CLI-only version without WebView support
 */

#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace fs = std::filesystem;

const char* VERSION = "1.0.0-cli";

void print_help() {
    std::cout << R"(
Valkyrie CLI-only - Project Generator

Usage:
    valkyrie <command> [options]

Commands:
    init [name]         Create a new project
    version             Display version
    help                Display this help message

Note: This is a CLI-only build without WebView support.
For full functionality (dev, build, run), use the amd64 build.

)" << std::endl;
}

void print_version() {
    std::cout << "valkyrie v" << VERSION << " (CLI-only)" << std::endl;
}

void create_project(const std::string& name) {
    if (fs::exists(name)) {
        std::cerr << "error: directory '" << name << "' already exists" << std::endl;
        return;
    }
    
    std::cout << "creating project: " << name << std::endl;
    
    fs::create_directories(name + "/src");
    
    // package.json
    std::ofstream pkg(name + "/package.json");
    pkg << R"({
  "name": ")" << name << R"(",
  "version": "1.0.0",
  "private": true,
  "type": "module",
  "scripts": {
    "dev": "valkyrie dev",
    "build": "valkyrie build"
  },
  "dependencies": {
    "react": "^18.2.0",
    "react-dom": "^18.2.0"
  },
  "devDependencies": {
    "@types/react": "^18.2.0",
    "@types/react-dom": "^18.2.0",
    "esbuild": "^0.19.0"
  }
})";
    pkg.close();
    
    // src/index.html
    std::ofstream html(name + "/src/index.html");
    html << R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" << name << R"(</title>
</head>
<body>
    <div id="root"></div>
    <script src="./main.jsx" type="module"></script>
</body>
</html>)";
    html.close();
    
    // src/main.jsx
    std::ofstream jsx(name + "/src/main.jsx");
    jsx << R"(import React from 'react';
import ReactDOM from 'react-dom/client';

function App() {
    return (
        <div style={{ padding: '20px', fontFamily: 'system-ui' }}>
            <h1>)" << name << R"(</h1>
            <p>Edit src/main.jsx to get started.</p>
        </div>
    );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);
)";
    jsx.close();
    
    // valkyrie.json
    std::ofstream config(name + "/valkyrie.json");
    config << R"({
  "name": ")" << name << R"(",
  "version": "1.0.0",
  "entry": "src/index.html",
  "window": {
    "width": 800,
    "height": 600,
    "title": ")" << name << R"("
  }
})";
    config.close();
    
    // README.md
    std::ofstream readme(name + "/README.md");
    readme << "# " << name << "\n\n";
    readme << "Created with Valkyrie\n\n";
    readme << "## Getting Started\n\n";
    readme << "```bash\n";
    readme << "cd " << name << "\n";
    readme << "npm install\n";
    readme << "valkyrie dev\n";
    readme << "```\n";
    readme.close();
    
    std::cout << "\nproject created successfully!\n" << std::endl;
    std::cout << "next steps:" << std::endl;
    std::cout << "  cd " << name << std::endl;
    std::cout << "  npm install" << std::endl;
    std::cout << "\nnote: 'valkyrie dev' requires full build (amd64)" << std::endl;
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
    } else if (cmd == "version" || cmd == "-v" || cmd == "--version") {
        print_version();
    } else if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        print_help();
    } else if (cmd == "dev" || cmd == "build" || cmd == "run" || cmd == "package") {
        std::cerr << "error: '" << cmd << "' requires full build with WebView support" << std::endl;
        std::cerr << "this is a CLI-only build (arm64/i386)" << std::endl;
        std::cerr << "use the amd64 build for full functionality" << std::endl;
        return 1;
    } else {
        std::cerr << "unknown command: " << cmd << std::endl;
        std::cerr << "run 'valkyrie help' for usage" << std::endl;
        return 1;
    }
    
    return 0;
}
