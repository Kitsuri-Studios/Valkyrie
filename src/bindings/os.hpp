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
#include <cstdlib>
#include <cstdio>

#ifdef _WIN32
    #include <windows.h>
    #define popen _popen
    #define pclose _pclose
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

namespace valkyrie {

static JSValue js_os_homedir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    if (!home) {
        const char* drive = getenv("HOMEDRIVE");
        const char* path = getenv("HOMEPATH");
        if (drive && path) {
            std::string full = std::string(drive) + path;
            return JS_NewString(ctx, full.c_str());
        }
    }
    return home ? JS_NewString(ctx, home) : JS_NULL;
#else
    const char* home = getenv("HOME");
    return home ? JS_NewString(ctx, home) : JS_NULL;
#endif
}

static JSValue js_os_tmpdir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
#ifdef _WIN32
    const char* tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    return tmp ? JS_NewString(ctx, tmp) : JS_NewString(ctx, "C:\\Windows\\Temp");
#else
    return JS_NewString(ctx, "/tmp");
#endif
}

static JSValue js_os_env(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc == 0) return JS_NULL;
    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_EXCEPTION;
    const char* val = getenv(name);
    JS_FreeCString(ctx, name);
    return val ? JS_NewString(ctx, val) : JS_NULL;
}

static JSValue js_exec(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* cmd = JS_ToCString(ctx, argv[0]);
    if (!cmd) return JS_EXCEPTION;
    
    FILE* pipe = popen(cmd, "r");
    JS_FreeCString(ctx, cmd);
    
    if (!pipe) return JS_NULL;
    
    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    int status = pclose(pipe);
    
    JSValue ret = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, ret, "stdout", JS_NewString(ctx, result.c_str()));
#ifdef _WIN32
    JS_SetPropertyStr(ctx, ret, "status", JS_NewInt32(ctx, status));
#else
    JS_SetPropertyStr(ctx, ret, "status", JS_NewInt32(ctx, WEXITSTATUS(status)));
#endif
    return ret;
}

static JSValue js_spawn(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* cmd = JS_ToCString(ctx, argv[0]);
    if (!cmd) return JS_EXCEPTION;
    
#ifdef _WIN32
    std::string full_cmd = "start /b " + std::string(cmd);
#else
    std::string full_cmd = std::string(cmd) + " &";
#endif
    int result = system(full_cmd.c_str());
    JS_FreeCString(ctx, cmd);
    
    return JS_NewBool(ctx, result == 0);
}

static JSValue js_system_info(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSValue obj = JS_NewObject(ctx);
    
#ifdef __linux__
    JS_SetPropertyStr(ctx, obj, "platform", JS_NewString(ctx, "linux"));
#elif __APPLE__
    JS_SetPropertyStr(ctx, obj, "platform", JS_NewString(ctx, "darwin"));
#elif _WIN32
    JS_SetPropertyStr(ctx, obj, "platform", JS_NewString(ctx, "win32"));
#else
    JS_SetPropertyStr(ctx, obj, "platform", JS_NewString(ctx, "unknown"));
#endif
    
    JS_SetPropertyStr(ctx, obj, "arch", JS_NewString(ctx, "x64"));
    
#ifdef _WIN32
    char hostname[256];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        JS_SetPropertyStr(ctx, obj, "hostname", JS_NewString(ctx, hostname));
    }
#else
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        JS_SetPropertyStr(ctx, obj, "hostname", JS_NewString(ctx, hostname));
    }
#endif
    
    return obj;
}

} // namespace valkyrie
