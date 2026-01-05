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
#include <uv.h>
#include <string>
#include <cstring>

namespace valkyrie {

struct native_socket {
    uv_tcp_t tcp;
    JSContext* ctx;
    JSValue on_connect;
    JSValue on_data;
    JSValue on_error;
    JSValue on_close;
    bool connected;
    
    native_socket() : ctx(nullptr), on_connect(JS_UNDEFINED), on_data(JS_UNDEFINED), 
                      on_error(JS_UNDEFINED), on_close(JS_UNDEFINED), connected(false) {}
};

static void socket_alloc_cb(uv_handle_t* handle, size_t suggested, uv_buf_t* buf) {
    buf->base = new char[suggested];
    buf->len = suggested;
}

static void socket_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    native_socket* sock = (native_socket*)stream->data;
    
    if (nread > 0) {
        if (JS_IsFunction(sock->ctx, sock->on_data)) {
            JSValue arr = JS_NewArrayBufferCopy(sock->ctx, (uint8_t*)buf->base, nread);
            JSValue args[1] = {arr};
            JS_Call(sock->ctx, sock->on_data, JS_UNDEFINED, 1, args);
            JS_FreeValue(sock->ctx, arr);
        }
    } else if (nread < 0) {
        if (nread != UV_EOF && JS_IsFunction(sock->ctx, sock->on_error)) {
            JSValue err = JS_NewString(sock->ctx, uv_strerror(nread));
            JSValue args[1] = {err};
            JS_Call(sock->ctx, sock->on_error, JS_UNDEFINED, 1, args);
            JS_FreeValue(sock->ctx, err);
        }
        if (nread == UV_EOF && JS_IsFunction(sock->ctx, sock->on_close)) {
            JS_Call(sock->ctx, sock->on_close, JS_UNDEFINED, 0, nullptr);
        }
    }
    
    delete[] buf->base;
}

static void socket_connect_cb(uv_connect_t* req, int status) {
    native_socket* sock = (native_socket*)req->data;
    
    if (status == 0) {
        sock->connected = true;
        if (JS_IsFunction(sock->ctx, sock->on_connect)) {
            JS_Call(sock->ctx, sock->on_connect, JS_UNDEFINED, 0, nullptr);
        }
        uv_read_start((uv_stream_t*)&sock->tcp, socket_alloc_cb, socket_read_cb);
    } else {
        if (JS_IsFunction(sock->ctx, sock->on_error)) {
            JSValue err = JS_NewString(sock->ctx, uv_strerror(status));
            JSValue args[1] = {err};
            JS_Call(sock->ctx, sock->on_error, JS_UNDEFINED, 1, args);
            JS_FreeValue(sock->ctx, err);
        }
    }
    
    delete req;
}

static void socket_write_cb(uv_write_t* req, int status) {
    auto* buf = (uv_buf_t*)req->data;
    delete[] buf->base;
    delete buf;
    delete req;
}

static void socket_close_cb(uv_handle_t* handle) {
}

static JSClassID js_socket_class_id;

static void js_socket_finalizer(JSRuntime* rt, JSValue val) {
    native_socket* sock = (native_socket*)JS_GetOpaque(val, js_socket_class_id);
    if (sock) {
        JS_FreeValue(sock->ctx, sock->on_connect);
        JS_FreeValue(sock->ctx, sock->on_data);
        JS_FreeValue(sock->ctx, sock->on_error);
        JS_FreeValue(sock->ctx, sock->on_close);
        if (!uv_is_closing((uv_handle_t*)&sock->tcp)) {
            uv_close((uv_handle_t*)&sock->tcp, socket_close_cb);
        }
        delete sock;
    }
}

static JSClassDef js_socket_class = {
    "NativeSocket",
    js_socket_finalizer,
};

static JSValue js_socket_new(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    JSValue obj = JS_NewObjectClass(ctx, js_socket_class_id);
    
    native_socket* sock = new native_socket();
    sock->ctx = ctx;
    
    uv_tcp_init(uv_default_loop(), &sock->tcp);
    sock->tcp.data = sock;
    
    JS_SetOpaque(obj, sock);
    return obj;
}

static JSValue js_socket_connect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    native_socket* sock = (native_socket*)JS_GetOpaque2(ctx, this_val, js_socket_class_id);
    if (!sock) return JS_EXCEPTION;
    
    const char* host = JS_ToCString(ctx, argv[0]);
    int port;
    JS_ToInt32(ctx, &port, argv[1]);
    
    struct sockaddr_in dest;
    uv_ip4_addr(host, port, &dest);
    
    uv_connect_t* req = new uv_connect_t();
    req->data = sock;
    
    uv_tcp_connect(req, &sock->tcp, (const sockaddr*)&dest, socket_connect_cb);
    
    JS_FreeCString(ctx, host);
    return JS_UNDEFINED;
}

static JSValue js_socket_write(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    native_socket* sock = (native_socket*)JS_GetOpaque2(ctx, this_val, js_socket_class_id);
    if (!sock || !sock->connected) return JS_EXCEPTION;
    
    size_t size;
    uint8_t* data = JS_GetArrayBuffer(ctx, &size, argv[0]);
    
    if (!data) {
        const char* str = JS_ToCString(ctx, argv[0]);
        if (!str) return JS_EXCEPTION;
        size = strlen(str);
        
        uv_buf_t* buf = new uv_buf_t();
        buf->base = new char[size];
        memcpy(buf->base, str, size);
        buf->len = size;
        
        uv_write_t* req = new uv_write_t();
        req->data = buf;
        
        uv_write(req, (uv_stream_t*)&sock->tcp, buf, 1, socket_write_cb);
        JS_FreeCString(ctx, str);
    } else {
        uv_buf_t* buf = new uv_buf_t();
        buf->base = new char[size];
        memcpy(buf->base, data, size);
        buf->len = size;
        
        uv_write_t* req = new uv_write_t();
        req->data = buf;
        
        uv_write(req, (uv_stream_t*)&sock->tcp, buf, 1, socket_write_cb);
    }
    
    return JS_UNDEFINED;
}

#define SOCKET_MAKE_SETTER(name, field) \
static JSValue js_socket_set_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) { \
    native_socket* sock = (native_socket*)JS_GetOpaque2(ctx, this_val, js_socket_class_id); \
    if (!sock) return JS_EXCEPTION; \
    JS_FreeValue(ctx, sock->field); \
    sock->field = JS_DupValue(ctx, argv[0]); \
    return JS_UNDEFINED; \
}

SOCKET_MAKE_SETTER(on_connect, on_connect)
SOCKET_MAKE_SETTER(on_data, on_data)
SOCKET_MAKE_SETTER(on_error, on_error)
SOCKET_MAKE_SETTER(on_close, on_close)

struct timer_ctx {
    JSContext* ctx;
    JSValue callback;
    uv_timer_t timer;
};

static void timer_cb(uv_timer_t* handle) {
    timer_ctx* tctx = (timer_ctx*)handle->data;
    JS_Call(tctx->ctx, tctx->callback, JS_UNDEFINED, 0, nullptr);
    JS_FreeValue(tctx->ctx, tctx->callback);
    uv_close((uv_handle_t*)handle, nullptr);
    delete tctx;
}

static JSValue js_set_timeout(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    timer_ctx* tctx = new timer_ctx();
    tctx->ctx = ctx;
    tctx->callback = JS_DupValue(ctx, argv[0]);
    
    int delay = 0;
    if (argc > 1) {
        JS_ToInt32(ctx, &delay, argv[1]);
    }
    
    uv_timer_init(uv_default_loop(), &tctx->timer);
    tctx->timer.data = tctx;
    uv_timer_start(&tctx->timer, timer_cb, delay, 0);
    
    return JS_UNDEFINED;
}

static JSValue js_buffer_alloc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    int size;
    JS_ToInt32(ctx, &size, argv[0]);
    return JS_NewArrayBuffer(ctx, (uint8_t*)nullptr, size, nullptr, nullptr, 0);
}

static JSValue js_buffer_from_bytes(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    size_t size;
    uint8_t* data = JS_GetArrayBuffer(ctx, &size, argv[0]);
    if (!data) return JS_EXCEPTION;
    
    return JS_NewArrayBufferCopy(ctx, data, size);
}

} // namespace valkyrie
