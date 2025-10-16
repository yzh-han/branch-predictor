# pragma once

#include "predictor/branch.hpp"
#include "predictor/predictor.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cstdint>
#include <deque>
#include <stack>


// Helper function to get trace file name without path and extension
std::string getTraceBaseName(const std::string& filepath) {
    // Get filename without path
    std::string filename = filepath;
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filepath.substr(lastSlash + 1);
    }
    
    // Remove extension
    size_t lastDot = filename.find_last_of(".");
    if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
    }
    
    return filename;
}

// Utility function to parse a line from the trace file
Branch parseLineToBranch(const std::string& line) {
    Branch branch;
    std::istringstream iss(line);
    std::string pc, target, kind, direct, conditional, taken;
    
    if (!(iss >> std::hex >> branch.pc
              >> std::hex >> branch.target
              >> branch.kind
              >> branch.direct
              >> branch.conditional
              >> branch.taken
    )) throw std::runtime_error("Error parsing line: " + line);

    return branch;
}


// Function to evaluate a predictor on a trace file, returning the total branches and mispredictions
std::vector<size_t> evaluatePredictor(BranchPredictor& predictor, const std::string& traceFile, size_t maxLines = 0) {
    std::ifstream file(traceFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << traceFile << std::endl;
        throw std::runtime_error("File not found");
    }
    
    predictor.reset();
    
    size_t totalBranches = 0;
    size_t mispredictions = 0;
    std::string line;
    
    while (std::getline(file, line) && (maxLines == 0 || totalBranches < maxLines)) {
        Branch branch = parseLineToBranch(line);
        
        bool prediction = predictor.predict(branch);
        bool correct = (prediction == branch.taken);
        
        if (!correct) {
            mispredictions++;
        }
        
        predictor.update(branch, prediction);
        totalBranches++;
    }
    
    double mispredictionRate = (totalBranches > 0) ? 
        (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;
    
    std::cout << "Predictor: " << predictor.getName() << std::endl;
    std::cout << "Total branches: " << totalBranches << std::endl;
    std::cout << "Mispredictions: " << mispredictions << std::endl;
    std::cout << "Misprediction rate: " << std::fixed << std::setprecision(2) << mispredictionRate << "%" << std::endl;
    std::cout << std::endl;

    return {totalBranches, mispredictions};
}

//  evaluation function for the Profiled predictor
std::vector<size_t> evaluateProfiledPredictor(ProfiledPredictor& predictor, const std::string& traceFile, size_t maxLines = 0) {
    // First pass: profiling mode
    std::ifstream file1(traceFile);
    if (!file1.is_open()) {
        std::cerr << "Error: Could not open file " << traceFile << std::endl;
        throw std::runtime_error("File not found");
    }
    
    predictor.reset();
    
    size_t totalBranches = 0;
    std::string line;
    
    std::cout << "Starting profiling phase..." << std::endl;
    
    while (std::getline(file1, line) && (maxLines == 0 || totalBranches < maxLines)) {
        Branch branch = parseLineToBranch(line);
        
        bool prediction = predictor.predict(branch);
        predictor.update(branch, prediction);
        totalBranches++;
    }
    
    size_t uniqueBranches = predictor.getProfileSize();
    size_t initializedIndices = predictor.getInitializedIndices();
    
    std::cout << "Profiling complete. Collected data for " << uniqueBranches 
              << " unique branch locations, affecting " << initializedIndices 
              << " table entries." << std::endl;
    
    // Calculate and report aliasing rate
    double aliasingRate = 1.0 - (static_cast<double>(initializedIndices) / uniqueBranches);
    std::cout << "Aliasing rate in the prediction table: " 
              << std::fixed << std::setprecision(2) << (aliasingRate * 100) << "%" << std::endl;
    
    // Switch to prediction mode and initialize 2-bit counters based on profile
    predictor.switchToPredict();
    
    // Second pass: prediction mode
    std::ifstream file2(traceFile);
    if (!file2.is_open()) {
        std::cerr << "Error: Could not open file " << traceFile << " for second pass" << std::endl;
        throw std::runtime_error("File not found");
    }
    
    totalBranches = 0;
    size_t mispredictions = 0;
    
    std::cout << "Starting prediction phase..." << std::endl;
    
    while (std::getline(file2, line) && (maxLines == 0 || totalBranches < maxLines)) {
        Branch branch = parseLineToBranch(line);
        
        bool prediction = predictor.predict(branch);
        bool correct = (prediction == branch.taken);
        
        if (!correct) {
            mispredictions++;
        }
        
        predictor.update(branch, prediction);
        totalBranches++;
    }
    
    double mispredictionRate = (totalBranches > 0) ? 
        (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;
    
    std::cout << "Predictor: " << predictor.getName() << std::endl;
    std::cout << "Total branches: " << totalBranches << std::endl;
    std::cout << "Mispredictions: " << mispredictions << std::endl;
    std::cout << "Misprediction rate: " << std::fixed << std::setprecision(2) << mispredictionRate << "%" << std::endl;
    std::cout << std::endl;

    return {totalBranches, mispredictions};
}

//  evaluation function for the Profiled 2Bit predictor
std::vector<size_t> evaluateProfiled2BitPredictor(Profiled2BitPredictor& predictor, const std::string& traceFile, size_t maxLines = 0) {
    // First pass: profiling mode
    std::ifstream file1(traceFile);
    if (!file1.is_open()) {
        std::cerr << "Error: Could not open file " << traceFile << std::endl;
        throw std::runtime_error("File not found");
    }
    
    predictor.reset();
    
    size_t totalBranches = 0;
    std::string line;
    
    std::cout << "Starting profiling phase..." << std::endl;
    
    while (std::getline(file1, line) && (maxLines == 0 || totalBranches < maxLines)) {
        Branch branch = parseLineToBranch(line);
        
        bool prediction = predictor.predict(branch);
        predictor.update(branch, prediction);
        totalBranches++;
    }
    
    size_t uniqueBranches = predictor.getProfileSize();
    size_t initializedIndices = predictor.getInitializedIndices();
    
    std::cout << "Profiling complete. Collected data for " << uniqueBranches 
              << " unique branch locations, affecting " << initializedIndices 
              << " table entries." << std::endl;
    
    // Calculate and report aliasing rate
    double aliasingRate = 1.0 - (static_cast<double>(initializedIndices) / uniqueBranches);
    std::cout << "Aliasing rate in the prediction table: " 
              << std::fixed << std::setprecision(2) << (aliasingRate * 100) << "%" << std::endl;
    
    // Switch to prediction mode and initialize 2-bit counters based on profile
    predictor.switchToPredict();
    
    // Second pass: prediction mode
    std::ifstream file2(traceFile);
    if (!file2.is_open()) {
        std::cerr << "Error: Could not open file " << traceFile << " for second pass" << std::endl;
        throw std::runtime_error("File not found");
    }
    
    totalBranches = 0;
    size_t mispredictions = 0;
    
    std::cout << "Starting prediction phase..." << std::endl;
    
    while (std::getline(file2, line) && (maxLines == 0 || totalBranches < maxLines)) {
        Branch branch = parseLineToBranch(line);
        
        bool prediction = predictor.predict(branch);
        bool correct = (prediction == branch.taken);
        
        if (!correct) {
            mispredictions++;
        }
        
        predictor.update(branch, prediction);
        totalBranches++;
    }
    
    double mispredictionRate = (totalBranches > 0) ? 
        (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;
    
    std::cout << "Predictor: " << predictor.getName() << std::endl;
    std::cout << "Total branches: " << totalBranches << std::endl;
    std::cout << "Mispredictions: " << mispredictions << std::endl;
    std::cout << "Misprediction rate: " << std::fixed << std::setprecision(2) << mispredictionRate << "%" << std::endl;
    std::cout << std::endl;

    return {totalBranches, mispredictions};
}