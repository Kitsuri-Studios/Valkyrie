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

inline void create_project(const std::string& name) {
    if (fs::exists(name)) {
        std::cerr << "error: directory '" << name << "' already exists" << std::endl;
        return;
    }
    
    std::cout << "\ncreating project: " << name << "\n" << std::endl;
    
    std::cout << "template:\n";
    std::cout << "  1) vanilla js\n";
    std::cout << "  2) react\n";
    std::cout << "  3) vue\n";
    std::cout << "  4) preact\n";
    std::cout << "choice [1]: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    if (choice.empty()) choice = "1";
    
    fs::create_directories(name);
    std::string orig_dir = fs::current_path().string();
    fs::current_path(name);
    
    const std::string COMMON_CSS = R"(:root {
      --bg: #0a0a0a;
      --fg: #e8e8e8;
      --muted: #666;
      --accent: #888;
      --border: #1a1a1a;
      --border-light: #252525;
      --font: 'Inter', -apple-system, sans-serif;
      --ease: cubic-bezier(0.16, 1, 0.3, 1);
    }
    
    * { margin: 0; padding: 0; box-sizing: border-box; }
    
    body {
      font-family: var(--font);
      background: var(--bg);
      color: var(--fg);
      min-height: 100vh;
      line-height: 1.5;
      -webkit-font-smoothing: antialiased;
    }
    
    .container {
      max-width: 600px;
      margin: 0 auto;
      padding: 4rem 2rem;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
    }
    
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding-bottom: 4rem;
      border-bottom: 1px solid var(--border);
    }
    
    .logo {
      font-weight: 600;
      letter-spacing: -0.02em;
    }
    
    .time {
      font-variant-numeric: tabular-nums;
      color: var(--muted);
      font-size: 0.875rem;
    }
    
    .hero {
      padding: 4rem 0;
      flex: 1;
    }
    
    h1 {
      font-size: 2.5rem;
      font-weight: 600;
      letter-spacing: -0.03em;
      line-height: 1.1;
      margin-bottom: 1rem;
    }
    
    .subtitle {
      color: var(--muted);
      font-size: 1rem;
    }
    
    .counter-section {
      padding: 3rem 0;
      border-top: 1px solid var(--border);
      border-bottom: 1px solid var(--border);
    }
    
    .counter {
      font-size: 4rem;
      font-weight: 600;
      font-variant-numeric: tabular-nums;
      text-align: center;
      margin-bottom: 2rem;
      letter-spacing: -0.02em;
    }
    
    .buttons {
      display: flex;
      gap: 0.5rem;
      justify-content: center;
    }
    
    button {
      background: var(--border-light);
      border: 1px solid var(--border-light);
      color: var(--fg);
      padding: 0.75rem 1.5rem;
      font-size: 1rem;
      font-family: var(--font);
      cursor: pointer;
      transition: all 0.2s var(--ease);
      min-width: 3rem;
    }
    
    button:hover {
      background: var(--border);
      border-color: var(--muted);
    }
    
    button.reset {
      color: var(--muted);
      padding: 0.75rem 2rem;
    }
    
    .footer {
      padding-top: 4rem;
      display: flex;
      justify-content: space-between;
      font-size: 0.875rem;
    }
    
    .muted { color: var(--muted); })";
    
    if (choice == "2") {
        write_file("package.json", R"({
  "name": ")" + name + R"(",
  "version": "0.1.0",
  "type": "module",
  "scripts": {
    "dev": "valkyrie dev",
    "build": "valkyrie build"
  },
  "dependencies": {
    "react": "^18.2.0",
    "react-dom": "^18.2.0"
  }
})");
        
        write_file("app.jsx", R"(import React, { useState, useEffect } from 'react';
import { createRoot } from 'react-dom/client';

function App() {
  const [count, setCount] = useState(0);
  const [time, setTime] = useState(new Date().toLocaleTimeString());
  
  useEffect(() => {
    const timer = setInterval(() => {
      setTime(new Date().toLocaleTimeString());
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  return (
    <main className="container">
      <header className="header">
        <span className="logo">valkyrie</span>
        <span className="time">{time}</span>
      </header>
      
      <section className="hero">
        <h1>desktop apps.<br/>without the bloat.</h1>
        <p className="subtitle">react + native. ~2mb. no electron.</p>
      </section>
      
      <div className="counter-section">
        <div className="counter">{count}</div>
        <div className="buttons">
          <button onClick={() => setCount(c => c - 1)}>-</button>
          <button onClick={() => setCount(0)} className="reset">reset</button>
          <button onClick={() => setCount(c => c + 1)}>+</button>
        </div>
      </div>
      
      <footer className="footer">
        <span>built with valkyrie</span>
        <span className="muted">v1.0.0</span>
      </footer>
    </main>
  );
}

createRoot(document.getElementById('root')).render(<App />);
)");
        
        write_file("index.html", R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>)" + name + R"(</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
  <style>)" + COMMON_CSS + R"(</style>
</head>
<body>
  <div id="root"></div>
</body>
</html>)");
        
        std::cout << "\nreact project created" << std::endl;
        std::cout << "\nnext:\n  cd " << name << "\n  npm install\n  valkyrie dev" << std::endl;
        
    } else if (choice == "3") {
        write_file("package.json", R"({
  "name": ")" + name + R"(",
  "version": "0.1.0",
  "type": "module",
  "scripts": {
    "dev": "valkyrie dev",
    "build": "valkyrie build"
  },
  "dependencies": {
    "vue": "^3.3.0"
  }
})");
        
        write_file("app.js", R"(import { createApp, ref, onMounted, onUnmounted } from 'vue';

const App = {
  setup() {
    const count = ref(0);
    const time = ref(new Date().toLocaleTimeString());
    let timer;
    
    onMounted(() => {
      timer = setInterval(() => {
        time.value = new Date().toLocaleTimeString();
      }, 1000);
    });
    
    onUnmounted(() => clearInterval(timer));
    
    return { count, time };
  },
  template: `
    <main class="container">
      <header class="header">
        <span class="logo">valkyrie</span>
        <span class="time">{{ time }}</span>
      </header>
      
      <section class="hero">
        <h1>desktop apps.<br>without the bloat.</h1>
        <p class="subtitle">vue + native. ~2mb. no electron.</p>
      </section>
      
      <div class="counter-section">
        <div class="counter">{{ count }}</div>
        <div class="buttons">
          <button @click="count--">-</button>
          <button @click="count = 0" class="reset">reset</button>
          <button @click="count++">+</button>
        </div>
      </div>
      
      <footer class="footer">
        <span>built with valkyrie</span>
        <span class="muted">v1.0.0</span>
      </footer>
    </main>
  `
};

createApp(App).mount('#app');
)");
        
        write_file("index.html", R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>)" + name + R"(</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
  <style>)" + COMMON_CSS + R"(</style>
</head>
<body>
  <div id="app"></div>
</body>
</html>)");
        
        std::cout << "\nvue project created" << std::endl;
        std::cout << "\nnext:\n  cd " << name << "\n  npm install\n  valkyrie dev" << std::endl;
        
    } else if (choice == "4") {
        write_file("package.json", R"({
  "name": ")" + name + R"(",
  "version": "0.1.0",
  "type": "module",
  "scripts": {
    "dev": "valkyrie dev",
    "build": "valkyrie build"
  },
  "dependencies": {
    "preact": "^10.19.0"
  }
})");
        
        write_file("app.jsx", R"(import { h, render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

function App() {
  const [count, setCount] = useState(0);
  const [time, setTime] = useState(new Date().toLocaleTimeString());
  
  useEffect(() => {
    const timer = setInterval(() => {
      setTime(new Date().toLocaleTimeString());
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  return (
    <main class="container">
      <header class="header">
        <span class="logo">valkyrie</span>
        <span class="time">{time}</span>
      </header>
      
      <section class="hero">
        <h1>desktop apps.<br/>without the bloat.</h1>
        <p class="subtitle">preact + native. ~2mb. no electron.</p>
      </section>
      
      <div class="counter-section">
        <div class="counter">{count}</div>
        <div class="buttons">
          <button onClick={() => setCount(c => c - 1)}>-</button>
          <button onClick={() => setCount(0)} class="reset">reset</button>
          <button onClick={() => setCount(c => c + 1)}>+</button>
        </div>
      </div>
      
      <footer class="footer">
        <span>built with valkyrie</span>
        <span class="muted">v1.0.0</span>
      </footer>
    </main>
  );
}

render(<App />, document.getElementById('root'));
)");
        
        write_file("index.html", R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>)" + name + R"(</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
  <style>)" + COMMON_CSS + R"(</style>
</head>
<body>
  <div id="root"></div>
</body>
</html>)");
        
        std::cout << "\npreact project created" << std::endl;
        std::cout << "\nnext:\n  cd " << name << "\n  npm install\n  valkyrie dev" << std::endl;
        
    } else {
        write_file("app.js", R"(document.addEventListener('DOMContentLoaded', () => {
  const counter = document.querySelector('.counter');
  const buttons = document.querySelectorAll('button');
  const timeEl = document.querySelector('.time');
  let count = 0;
  
  const updateTime = () => {
    timeEl.textContent = new Date().toLocaleTimeString();
  };
  updateTime();
  setInterval(updateTime, 1000);
  
  buttons.forEach(btn => {
    btn.addEventListener('click', () => {
      if (btn.classList.contains('reset')) {
        count = 0;
      } else if (btn.textContent === '+') {
        count++;
      } else {
        count--;
      }
      counter.textContent = count;
    });
  });
});
)");
        
        write_file("index.html", R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>)" + name + R"(</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
  <style>)" + COMMON_CSS + R"(</style>
</head>
<body>
  <main class="container">
    <header class="header">
      <span class="logo">valkyrie</span>
      <span class="time">--:--:--</span>
    </header>
    
    <section class="hero">
      <h1>desktop apps.<br>without the bloat.</h1>
      <p class="subtitle">vanilla js + native. ~2mb. no electron.</p>
    </section>
    
    <div class="counter-section">
      <div class="counter">0</div>
      <div class="buttons">
        <button>-</button>
        <button class="reset">reset</button>
        <button>+</button>
      </div>
    </div>
    
    <footer class="footer">
      <span>built with valkyrie</span>
      <span class="muted">v1.0.0</span>
    </footer>
  </main>
  <script src="app.js"></script>
</body>
</html>)");
        
        std::cout << "\nvanilla js project created" << std::endl;
        std::cout << "\nnext:\n  cd " << name << "\n  valkyrie dev" << std::endl;
    }
    
    write_file(".gitignore", R"(node_modules/
dist/
*.o
*.so
*.dylib
*.dll
app
*.app
*.exe
_runner.cpp
_build.cpp
.env
)");
    
    write_file(".env.example", R"(# Environment Variables
# Copy to .env and configure
API_KEY=your_key_here
DEBUG=true
)");
    
    fs::current_path(orig_dir);
    std::cout << "\nProject created successfully.\n" << std::endl;
}
