#pragma once

#include <bee/subprocess/common.h>
#include <bee/utility/zstring_view.h>
#include <spawn.h>

#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace bee::subprocess {
    class envbuilder {
    public:
        void set(const std::string& key, const std::string& value);
        void del(const std::string& key);
        environment release();

    private:
        std::map<std::string, std::string> set_env_;
        std::set<std::string> del_env_;
    };

    class spawn;
    class process {
    public:
        process(spawn& spawn) noexcept;
        uint32_t get_id() const noexcept;
        uintptr_t native_handle() const noexcept;
        bool kill(int signum) noexcept;
        bool is_running() noexcept;
        std::optional<uint32_t> wait() noexcept;
        bool resume() noexcept;
        int pid;
        std::optional<uint32_t> status;
    };

    struct args_t {
        ~args_t();
        void push(char* str);
        void push(zstring_view str);
        char*& operator[](size_t i) {
            return data_[i];
        }
        char* const& operator[](size_t i) const {
            return data_[i];
        }
        char* const* data() const {
            return data_.data();
        }
        char** data() {
            return data_.data();
        }
        size_t size() const {
            return data_.size();
        }

    private:
        std::vector<char*> data_;
    };

    class spawn {
        friend class process;

    public:
        spawn();
        ~spawn();
        void suspended();
        void detached();
        void redirect(stdio type, file_handle f);
        void env(environment&& env);
        bool exec(args_t& args, const char* cwd);

    private:
        environment env_ = nullptr;
        int fds_[3];
        int pid_ = -1;
        posix_spawnattr_t spawnattr_;
        posix_spawn_file_actions_t spawnfile_;
    };
}
