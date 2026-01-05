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

namespace valkyrie {

static const char* RUNTIME_JS = R"js(
globalThis.process = {
    versions: { node: '16.0.0' },
    platform: 'linux',
    env: {},
    cwd: () => '/',
    nextTick: (fn) => setTimeout(fn, 0)
};

globalThis.console = {
    log: (...args) => {
        const msg = args.map(a => {
            if (typeof a === 'object') return JSON.stringify(a);
            return String(a);
        }).join(' ');
        if (typeof native_print !== 'undefined') {
            native_print(msg);
        }
    },
    error: (...args) => globalThis.console.log('[ERROR]', ...args),
    warn: (...args) => globalThis.console.log('[WARN]', ...args),
    info: (...args) => globalThis.console.log('[INFO]', ...args)
};

class EventEmitter {
    constructor() {
        this._events = {};
    }
    
    on(event, listener) {
        if (!this._events[event]) this._events[event] = [];
        this._events[event].push(listener);
        return this;
    }
    
    once(event, listener) {
        const wrapper = (...args) => {
            this.off(event, wrapper);
            listener(...args);
        };
        return this.on(event, wrapper);
    }
    
    off(event, listener) {
        if (!this._events[event]) return this;
        this._events[event] = this._events[event].filter(l => l !== listener);
        return this;
    }
    
    emit(event, ...args) {
        if (!this._events[event]) return false;
        this._events[event].forEach(listener => {
            try {
                listener(...args);
            } catch (e) {
                console.error('EventEmitter error:', e);
            }
        });
        return true;
    }
    
    removeAllListeners(event) {
        if (event) {
            delete this._events[event];
        } else {
            this._events = {};
        }
        return this;
    }
}

globalThis.EventEmitter = EventEmitter;

globalThis.Buffer = class Buffer extends Uint8Array {
    static alloc(size) {
        return new Buffer(size);
    }
    
    static from(data) {
        if (typeof data === 'string') {
            return Buffer.fromString(data);
        }
        if (Array.isArray(data) || data instanceof Uint8Array) {
            return new Buffer(data);
        }
        throw new Error('Unsupported buffer source');
    }
    
    static fromString(str, encoding = 'utf8') {
        const encoder = new TextEncoder();
        return new Buffer(encoder.encode(str));
    }
    
    toString(encoding = 'utf8') {
        const decoder = new TextDecoder(encoding);
        return decoder.decode(this);
    }
    
    slice(start, end) {
        return new Buffer(super.slice(start, end));
    }
};

globalThis.module = { exports: {} };
globalThis.exports = globalThis.module.exports;

)js";

} // namespace valkyrie
