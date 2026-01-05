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
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace valkyrie {

class vfs {
public:
    struct file_entry {
        std::vector<uint8_t> data;
        std::string mime_type;
    };

    static vfs& instance() {
        static vfs inst;
        return inst;
    }

    void register_file(const std::string& path, const uint8_t* data, size_t len, const std::string& mime = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        files_[path] = file_entry{{data, data + len}, mime};
    }

    void register_file(const std::string& path, const std::string& data, const std::string& mime = "") {
        register_file(path, (const uint8_t*)data.data(), data.size(), mime);
    }

    std::optional<file_entry> read_file(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = files_.find(path);
        if (it != files_.end()) {
            return it->second;
        }
        if (path[0] == '/') {
            it = files_.find(path.substr(1));
            if (it != files_.end()) {
                return it->second;
            }
        }
        return std::nullopt;
    }

    bool exists(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        return files_.find(path) != files_.end();
    }

    std::vector<std::string> list_files() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> result;
        result.reserve(files_.size());
        for (const auto& [path, _] : files_) {
            result.push_back(path);
        }
        return result;
    }

private:
    vfs() = default;
    std::unordered_map<std::string, file_entry> files_;
    std::mutex mutex_;
};

#define VFS_REGISTER(path, data) \
    valkyrie::vfs::instance().register_file(path, data, sizeof(data) - 1)

#define VFS_REGISTER_STRING(path, str) \
    valkyrie::vfs::instance().register_file(path, str)

} // namespace valkyrie
