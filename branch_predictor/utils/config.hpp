#pragma once

#include <vector>
#include <string>

inline struct Config
{
    std::string TRACE = "../trace/bwaves.out"; // Default trace file

    // useing for predictor evaluation
    std::vector<std::string> TRACES = {
        "trace/exchange2_cutted.out",
        "trace/gcc_cutted.out",
        "trace/leela_cutted.out",
        "trace/wrf_cutted.out",
    };

    // useing for traces analysis
    std::vector<std::string> ORIGINAL_TRACES = {
        "../trace/bwaves.out",
        "../trace/cactusbssn.out",
        "../trace/exchange2.out",
        "../trace/gcc.out",
        "../trace/leela.out",
        "../trace/povray.out",
        "../trace/wrf.out",
        "../trace/xz.out",        
    };
} config;
