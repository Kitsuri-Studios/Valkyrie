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

#include "../core/vfs.hpp"
#include <quickjs/quickjs.h>
#include <uv.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstring>
#include <map>

namespace valkyrie {

struct http_request {
    std::string method;
    std::string url;
    std::string host;
    std::string path;
    int port;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct http_response {
    int status_code;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
};

class http_client {
public:
    using callback_t = std::function<void(http_response)>;
    
    static void fetch(const http_request& req, callback_t callback) {
        auto ctx = new request_context{req, callback};
        
        uv_getaddrinfo_t* resolver = new uv_getaddrinfo_t();
        resolver->data = ctx;
        
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        uv_getaddrinfo(uv_default_loop(), resolver, on_resolved, 
                      req.host.c_str(), std::to_string(req.port).c_str(), &hints);
    }

private:
    struct request_context {
        http_request req;
        callback_t callback;
        uv_tcp_t socket;
        uv_connect_t connect_req;
        std::string response_buffer;
        http_response response;
        bool headers_parsed = false;
    };
    
    static void on_resolved(uv_getaddrinfo_t* resolver, int status, struct addrinfo* res) {
        auto ctx = (request_context*)resolver->data;
        
        if (status < 0) {
            ctx->callback(http_response{0, {}, {}});
            delete ctx;
            uv_freeaddrinfo(res);
            delete resolver;
            return;
        }
        
        uv_tcp_init(uv_default_loop(), &ctx->socket);
        ctx->socket.data = ctx;
        ctx->connect_req.data = ctx;
        
        uv_tcp_connect(&ctx->connect_req, &ctx->socket, res->ai_addr, on_connect);
        
        uv_freeaddrinfo(res);
        delete resolver;
    }
    
    static void on_connect(uv_connect_t* req, int status) {
        auto ctx = (request_context*)req->data;
        
        if (status < 0) {
            ctx->callback(http_response{0, {}, {}});
            delete ctx;
            return;
        }
        
        std::string request_str = ctx->req.method + " " + ctx->req.path + " HTTP/1.1\r\n";
        request_str += "Host: " + ctx->req.host + "\r\n";
        request_str += "Connection: close\r\n";
        
        for (const auto& [key, val] : ctx->req.headers) {
            request_str += key + ": " + val + "\r\n";
        }
        
        if (!ctx->req.body.empty()) {
            request_str += "Content-Length: " + std::to_string(ctx->req.body.size()) + "\r\n";
        }
        
        request_str += "\r\n";
        if (!ctx->req.body.empty()) {
            request_str += ctx->req.body;
        }
        
        uv_buf_t buf = uv_buf_init((char*)request_str.c_str(), request_str.size());
        uv_write_t* write_req = new uv_write_t();
        write_req->data = new std::string(request_str);
        
        uv_write(write_req, (uv_stream_t*)&ctx->socket, &buf, 1, on_write);
        uv_read_start((uv_stream_t*)&ctx->socket, alloc_buffer, on_read);
    }
    
    static void alloc_buffer(uv_handle_t* handle, size_t suggested, uv_buf_t* buf) {
        buf->base = new char[suggested];
        buf->len = suggested;
    }
    
    static void on_write(uv_write_t* req, int status) {
        delete (std::string*)req->data;
        delete req;
    }
    
    static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
        auto ctx = (request_context*)stream->data;
        
        if (nread > 0) {
            ctx->response_buffer.append(buf->base, nread);
            
            if (!ctx->headers_parsed) {
                size_t header_end = ctx->response_buffer.find("\r\n\r\n");
                if (header_end != std::string::npos) {
                    std::string header_section = ctx->response_buffer.substr(0, header_end);
                    size_t status_line_end = header_section.find("\r\n");
                    std::string status_line = header_section.substr(0, status_line_end);
                    
                    size_t first_space = status_line.find(' ');
                    size_t second_space = status_line.find(' ', first_space + 1);
                    std::string status_code_str = status_line.substr(first_space + 1, second_space - first_space - 1);
                    ctx->response.status_code = std::stoi(status_code_str);
                    
                    ctx->headers_parsed = true;
                    
                    std::string body_str = ctx->response_buffer.substr(header_end + 4);
                    ctx->response.body.assign(body_str.begin(), body_str.end());
                }
            } else {
                size_t header_end = ctx->response_buffer.find("\r\n\r\n");
                std::string body_str = ctx->response_buffer.substr(header_end + 4);
                ctx->response.body.assign(body_str.begin(), body_str.end());
            }
        } else if (nread < 0) {
            if (ctx->headers_parsed) {
                ctx->callback(ctx->response);
            } else {
                ctx->callback(http_response{0, {}, {}});
            }
            
            uv_close((uv_handle_t*)stream, nullptr);
            delete ctx;
        }
        
        delete[] buf->base;
    }
};

static JSValue js_http_fetch(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    const char* url = JS_ToCString(ctx, argv[0]);
    if (!url) return JS_EXCEPTION;
    
    std::string url_str = url;
    http_request req;
    req.method = "GET";
    req.url = url_str;
    
    size_t protocol_end = url_str.find("://");
    if (protocol_end != std::string::npos) {
        url_str = url_str.substr(protocol_end + 3);
    }
    
    size_t path_start = url_str.find('/');
    if (path_start != std::string::npos) {
        req.host = url_str.substr(0, path_start);
        req.path = url_str.substr(path_start);
    } else {
        req.host = url_str;
        req.path = "/";
    }
    
    size_t port_start = req.host.find(':');
    if (port_start != std::string::npos) {
        req.port = std::stoi(req.host.substr(port_start + 1));
        req.host = req.host.substr(0, port_start);
    } else {
        req.port = 80;
    }
    
    JS_FreeCString(ctx, url);
    
    JSValue promise, resolving_funcs[2];
    promise = JS_NewPromiseCapability(ctx, resolving_funcs);
    JSValue resolve_func = resolving_funcs[0];
    JSValue reject_func = resolving_funcs[1];
    
    JSValue* resolve_ptr = new JSValue(JS_DupValue(ctx, resolve_func));
    JSContext* ctx_ptr = ctx;
    
    http_client::fetch(req, [ctx_ptr, resolve_ptr](http_response resp) {
        JSValue obj = JS_NewObject(ctx_ptr);
        JS_SetPropertyStr(ctx_ptr, obj, "status", JS_NewInt32(ctx_ptr, resp.status_code));
        
        JSValue body_arr = JS_NewArrayBufferCopy(ctx_ptr, resp.body.data(), resp.body.size());
        JS_SetPropertyStr(ctx_ptr, obj, "body", body_arr);
        
        std::string body_text(resp.body.begin(), resp.body.end());
        JS_SetPropertyStr(ctx_ptr, obj, "text", JS_NewString(ctx_ptr, body_text.c_str()));
        
        JSValue args[1] = {obj};
        JS_Call(ctx_ptr, *resolve_ptr, JS_UNDEFINED, 1, args);
        JS_FreeValue(ctx_ptr, obj);
        JS_FreeValue(ctx_ptr, *resolve_ptr);
        delete resolve_ptr;
    });
    
    JS_FreeValue(ctx, resolve_func);
    JS_FreeValue(ctx, reject_func);
    
    return promise;
}

} // namespace valkyrie
