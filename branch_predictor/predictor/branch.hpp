# pragma once

#include <string>
#include <cstdint>
#include <sstream>

// Define a struct to hold branch information from the trace
struct Branch {
    uint64_t pc;           // Program counter address
    uint64_t target;       // Target address
    char kind;             // Branch kind (b, c, r)
    bool direct;           // Is direct branch?
    bool conditional;      // Is conditional branch?
    bool taken;            // Was the branch taken?

    inline std::string toString() const {
        std::stringstream ss;
        ss << "PC: " << std::hex << pc << "\n"
           << "Target: " << std::hex  << target << "\n"
           << "Kind: " << kind << "\n"
           << "Direct: " << (direct ? "Yes" : "No") << "\n"
           << "Conditional: " << (conditional ? "Yes" : "No") << "\n"
           << "Taken: " << (taken ? "Yes" : "No");
        return ss.str();
    }
};