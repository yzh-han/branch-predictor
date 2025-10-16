#pragma once

#include "predictor/branch.hpp"
#include "predictor/counter.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <unordered_map>
#include <bitset>
#include <cmath>
#include <sstream>
#include <unordered_set>

// Base class for all branch predictors
class BranchPredictor {
public:
    virtual ~BranchPredictor() {}
    
    // Make a prediction for the given branch
    virtual bool predict(const Branch& branch) = 0;
    
    // Update the predictor with the actual outcome
    virtual void update(const Branch& branch, bool predicted) = 0;
    
    // Get the name of the predictor
    virtual std::string getName() const = 0;
    
    // Reset the predictor state
    virtual void reset() = 0;
};

// Always Taken predictor - always predicts branch as taken
class AlwaysTakenPredictor : public BranchPredictor {
public:
    bool predict(const Branch& branch) override {
        return true; // Always predict taken
    }
    
    void update(const Branch& branch, bool predicted) override {
        // Nothing to update for this strategy
    }
    
    std::string getName() const override {
        return "Always Taken";
    }
    
    void reset() override {
        // Nothing to reset
    }
};

// 2-bit saturating counter predictor
class TwoBitPredictor : public BranchPredictor {
private:    
    std::vector<State> table;
    size_t tableSize;
    size_t indexMask;
    
    // Convert PC to table index
    size_t getIndex(uint64_t pc) const {
        return (pc & indexMask);
    }
    
public:
    TwoBitPredictor(size_t size) : tableSize(size) {
        // Initialize table with all entries as WEAKLY_TAKEN (2)
        table.resize(tableSize, WEAKLY_TAKEN);
        
        // Calculate mask for indexing (size - 1)
        indexMask = tableSize - 1;
    }
    
    bool predict(const Branch& branch) override {
        size_t index = getIndex(branch.pc);
        return (table[index] >= WEAKLY_TAKEN);
    }
    
    void update(const Branch& branch, bool predicted) override {
        size_t index = getIndex(branch.pc);
        State& currentState = table[index];

        // Update the counter state based on the actual outcome
        updateCounterState(branch.taken, currentState);
    }
    
    std::string getName() const override {
        std::stringstream ss;
        ss << "2-bit (" << tableSize << ")";
        return ss.str();
    }
    
    void reset() override {
        std::fill(table.begin(), table.end(), WEAKLY_TAKEN);
    }
};

// gshare predictor - uses global history with XOR indexing
class GSharePredictor : public BranchPredictor {
private:    
    std::vector<State> table;
    size_t tableSize;
    size_t indexMask;
    size_t historyRegister;
    int historyBits;
    
    // Get index using PC and history register
    size_t getIndex(uint64_t pc) const {
        return ((pc & indexMask) ^ (historyRegister & indexMask));
    }
    
public:
    GSharePredictor(size_t size) : tableSize(size), historyRegister(0) {
        // Initialize table with all entries as WEAKLY_TAKEN (2)
        table.resize(tableSize, WEAKLY_TAKEN);
        
        // Calculate mask for indexing (size - 1)
        indexMask = tableSize - 1;
        
        // Calculate number of history bits to use (log2 of table size)
        historyBits = static_cast<int>(log2(tableSize));
    }
    
    bool predict(const Branch& branch) override {
        size_t index = getIndex(branch.pc);
        return (table[index] >= WEAKLY_TAKEN);
    }
    
    void update(const Branch& branch, bool predicted) override {
        size_t index = getIndex(branch.pc);
        State& currentState = table[index];
        
        updateCounterState(branch.taken, currentState);
        
        // Update history register by shifting in the actual outcome
        historyRegister = ((historyRegister << 1) | (branch.taken ? 1 : 0)) & ((1 << historyBits) - 1);
    }
    
    std::string getName() const override {
        std::stringstream ss;
        ss << "gshare (" << tableSize << ")";
        return ss.str();
    }
    
    void reset() override {
        std::fill(table.begin(), table.end(), WEAKLY_TAKEN);
        historyRegister = 0;
    }
};


// Hardware-realistic basic Profiled predictor
class ProfiledPredictor : public BranchPredictor {
    private:
        // Profiling data (only used during profiling phase)
        std::unordered_map<uint64_t, int> takenCount;     // PC -> taken count
        std::unordered_map<uint64_t, int> totalCount;     // PC -> total count
        
        // 2-bit counters table for prediction phase (hardware realistic)
        std::vector<bool> StateTable;
        size_t tableSize;
        size_t indexMask;
        
        bool profilingMode;    // Whether in profiling or prediction mode
        
        // Get table index from branch address
        size_t getIndex(uint64_t pc) const {
            return (pc & indexMask);
        }
            
    public:
        ProfiledPredictor(size_t size = 2048) : tableSize(size), profilingMode(true) {
            // Initialize counter table
            StateTable.resize(tableSize, true);
            indexMask = tableSize - 1;
        }
        
        bool predict(const Branch& branch) override {
            if (profilingMode) {
                // In profiling mode, just return actual outcome (no prediction)
                return branch.taken;
            } else {
                // In prediction mode, use 2-bit counter table with PC indexing
                size_t index = getIndex(branch.pc);
                return (StateTable[index]);
            }
        }
        
        void update(const Branch& branch, bool predicted) override {
            // In profiling mode, collect statistics
            if (profilingMode) {
                if (branch.taken) {
                    takenCount[branch.pc]++;
                }
                totalCount[branch.pc]++;
            } 
            // In prediction mode, update 2-bit counter in the table
        }
        
        std::string getName() const override {
            std::stringstream ss;
            ss << "Profiled (" << tableSize << ")";
            // if (profilingMode) {
            //     ss << " [Training]";
            // } else {
            //     ss << " [Prediction]";
            // }
            return ss.str();
        }
        
        void reset() override {
            takenCount.clear();
            totalCount.clear();
            std::fill(StateTable.begin(), StateTable.end(), true);
            profilingMode = true;
        }
        
        // Switch from profiling to prediction mode and initialize 2-bit counters
        void switchToPredict() {
            profilingMode = false;
            
            // First reset all counters to a default state
            std::fill(StateTable.begin(), StateTable.end(), true);
            
            // Create a mapping of table indices to profile statistics
            std::vector<std::pair<int, int>> indexStats(tableSize, {0, 0}); // (taken, total) per index
            
            // Aggregate profile data by table index
            for (const auto& entry : totalCount) {
                uint64_t pc = entry.first;
                size_t index = getIndex(pc);
                
                indexStats[index].first += takenCount[pc];    // taken count
                indexStats[index].second += entry.second;     // total count
            }
            
            // Initialize counter table based on aggregated profile data
            for (size_t i = 0; i < tableSize; i++) {
                if (indexStats[i].second > 0) {  // If we have data for this index
                    double takenRate = static_cast<double>(indexStats[i].first) / indexStats[i].second;
                    
                    // Initialize counter state based on historical taken rate
                    if (takenRate > 0.5) {
                        StateTable[i] = true;
                    } else {
                        StateTable[i] = false;
                    }
                }
            }
        }
        
        // Get profile size for reporting
        size_t getProfileSize() const {
            return totalCount.size();
        }
        
        // Get number of indices with profile data
        size_t getInitializedIndices() const {
            std::unordered_set<size_t> indices;
            for (const auto& entry : totalCount) {
                indices.insert(getIndex(entry.first));
            }
            return indices.size();
        }
    };

// Hardware-realistic Profiled predictor with 2-bit counter implementation
class Profiled2BitPredictor : public BranchPredictor {
private:
    // Profiling data (only used during profiling phase)
    std::unordered_map<uint64_t, int> takenCount;     // PC -> taken count
    std::unordered_map<uint64_t, int> totalCount;     // PC -> total count
    
    // 2-bit counters table for prediction phase (hardware realistic)
    std::vector<State> counterTable;
    size_t tableSize;
    size_t indexMask;
    
    bool profilingMode;    // Whether in profiling or prediction mode
    
    // Get table index from branch address
    size_t getIndex(uint64_t pc) const {
        return (pc & indexMask);
    }
        
public:
    Profiled2BitPredictor(size_t size = 2048) : tableSize(size), profilingMode(true) {
        // Initialize counter table
        counterTable.resize(tableSize, WEAKLY_TAKEN);
        indexMask = tableSize - 1;
    }
    
    bool predict(const Branch& branch) override {
        if (profilingMode) {
            // In profiling mode, just return actual outcome (no prediction)
            return branch.taken;
        } else {
            // In prediction mode, use 2-bit counter table with PC indexing
            size_t index = getIndex(branch.pc);
            return (counterTable[index] >= WEAKLY_TAKEN);
        }
    }
    
    void update(const Branch& branch, bool predicted) override {
        // In profiling mode, collect statistics
        if (profilingMode) {
            if (branch.taken) {
                takenCount[branch.pc]++;
            }
            totalCount[branch.pc]++;
        } 
        // In prediction mode, update 2-bit counter in the table
        else {
            size_t index = getIndex(branch.pc);
            State& currentState = counterTable[index];
            
            updateCounterState(branch.taken, currentState);
        }
    }
    
    std::string getName() const override {
        std::stringstream ss;
        ss << "Profiled 2-bit (" << tableSize << ")";
        // if (profilingMode) {
        //     ss << " [Training]";
        // } else {
        //     ss << " [Prediction]";
        // }
        return ss.str();
    }
    
    void reset() override {
        takenCount.clear();
        totalCount.clear();
        std::fill(counterTable.begin(), counterTable.end(), WEAKLY_TAKEN);
        profilingMode = true;
    }
    
    // Switch from profiling to prediction mode and initialize 2-bit counters
    void switchToPredict() {
        profilingMode = false;
        
        // First reset all counters to a default state
        std::fill(counterTable.begin(), counterTable.end(), WEAKLY_TAKEN);
        
        // Create a mapping of table indices to profile statistics
        std::vector<std::pair<int, int>> indexStats(tableSize, {0, 0}); // (taken, total) per index
        
        // Aggregate profile data by table index
        for (const auto& entry : totalCount) {
            uint64_t pc = entry.first;
            size_t index = getIndex(pc);
            
            indexStats[index].first += takenCount[pc];    // taken count
            indexStats[index].second += entry.second;     // total count
        }
        
        // Initialize counter table based on aggregated profile data
        for (size_t i = 0; i < tableSize; i++) {
            if (indexStats[i].second > 0) {  // If we have data for this index
                double takenRate = static_cast<double>(indexStats[i].first) / indexStats[i].second;
                
                // Initialize counter state based on historical taken rate
                if (takenRate > 0.75) {
                    counterTable[i] = STRONGLY_TAKEN;
                } else if (takenRate > 0.5) {
                    counterTable[i] = WEAKLY_TAKEN;
                } else if (takenRate > 0.25) {
                    counterTable[i] = WEAKLY_NOT_TAKEN;
                } else {
                    counterTable[i] = STRONGLY_NOT_TAKEN;
                }
            }
            // If no data, keep default WEAKLY_TAKEN
        }
    }
    
    // Get profile size for reporting
    size_t getProfileSize() const {
        return totalCount.size();
    }
    
    // Get number of indices with profile data
    size_t getInitializedIndices() const {
        std::unordered_set<size_t> indices;
        for (const auto& entry : totalCount) {
            indices.insert(getIndex(entry.first));
        }
        return indices.size();
    }
};

