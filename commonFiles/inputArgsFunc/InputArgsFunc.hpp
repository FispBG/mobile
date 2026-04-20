//
// Created by fisp on 19.04.2026.
//

#pragma once

#include "../resultFunc/ResultFunction.hpp"
#include "../stringFunc/StringFunc.hpp"

#include <cstring>
#include <unordered_map>

template <typename helpFunc>
bool loopForInputArgs(const int argc, const char *argv[], helpFunc help, std::unordered_map<std::string, std::string> &flags) {
    for (int i = 1; i < argc; i++) {
        const std::string flag = argv[i];

        if (strcmp(argv[i],"--help") == 0) {
            help();
            exit(0);
        }

        if (i+1 >= argc) {
            logger(RES_ERROR("No argument for flag: " + flag));
            return false;
        }

        std::string argFlag = argv[++i];

        const bool isNegativeNumber = argFlag.size() > 1 &&
                            argFlag[0] == '-' &&
                            std::isdigit(static_cast<unsigned char>(argFlag[1]));

        if (argFlag[0] == '-' && !isNegativeNumber) {
            logger(RES_ERROR("Invalid arg flag: " + argFlag));
            return false;
        }
        flags[flag] = std::move(argFlag);
    }

    return true;
}

