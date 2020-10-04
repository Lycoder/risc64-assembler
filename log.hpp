#pragma once

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cstdio>
#include <memory>

namespace log {
    char buf[512];

    std::ofstream file;

    namespace type {
        const char *none    = "\u001b[30;1m[.]",
                    *debug   = "\u001b[34m[d]",
                    *ok      = "\u001b[32;1m[k]",
                    *info    = "\u001b[34;1m[i]",
                    *warning = "\u001b[35;1m[w]",
                    *error   = "\u001b[31;1m[e]";
    }

    template <class... Args> void log(const char* t, std::string fmt, Args... args) {
        sprintf(buf, fmt.c_str(), args...);
        std::cout << t << "\u001b[0m risc64-a: " << buf << std::endl;

        if (file.is_open()) {
            std::string tstr(t), l = tstr.substr(tstr.find_last_of('['), 3) + " ";
            file << l << buf << std::endl;
        }
    }

    void init(const std::string& fn = "") {
        if (fn.size()) {
            file.open(fn);
            if (!file.good()) {
                log(type::warning, "Couldn't open log file \"%s\"", fn.c_str());
            }
        }
    }
}

#define _log(t, ...) log::log(log::type::t, __VA_ARGS__)