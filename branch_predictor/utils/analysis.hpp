# pragma once

#include "predictor/branch.hpp"
#include "utils/utils.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cstdint>
#include <deque>
#include <stack>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <set>
#include <map>



// PattenData structure to hold pattern name and percentage
struct PatternData {
    std::string pattern;
    double percentage;
    
    PatternData(const std::string& p, double pct) : pattern(p), percentage(pct) {}
    
    // Overload < operator for sorting by percentage
    bool operator<(const PatternData& other) const {
        return percentage > other.percentage;  // note: descending order
    }
};

// Structure to hold all branch analysis metrics
struct BranchMetrics {
    std::string traceName;
    size_t totalBranches = 0;
    size_t directBranches = 0;
    size_t indirectBranches = 0;
    size_t conditionalBranches = 0;
    size_t unconditionalBranches = 0;
    size_t regularBranches = 0;  // 'b'
    size_t callInstructions = 0; // 'c'
    size_t returnInstructions = 0; // 'r'
    size_t takenBranches = 0;
    size_t condTakenBranches = 0;
    size_t uniqueBranchLocations = 0;
    size_t uniqueCondBranchLocations = 0;
    size_t highlyPredictableAll = 0;
    size_t highlyPredictableCond = 0;
    double hotspotPercentage = 0.0;  // Top 5 branches percentage
    
    // Sorted vectors of PC patterns (by percentage)
    std::vector<PatternData> pcPatterns;
    
    // Sorted vectors of taken patterns (by percentage)
    std::vector<PatternData> takenPatterns;
    
    // Raw pattern counts (for reference)
    std::map<std::string, size_t> rawPCPatternCounts;       // top 5 pattern -> count
    std::map<std::string, size_t> rawTakenPatternCounts;    // top 5 pattern -> count
    
    // Top 5 hotspots
    struct Hotspot {
        uint64_t address;               // pc address
        size_t executions;              // how many times, this branch executed
        double executionPercentage;     // Percentage of total branches
        double takenPercentage;         // Percentage of taken branches
        bool isConditional;  // Whether this is a conditional branch
    };
    std::vector<Hotspot> topHotspots;
    
    // Calculate percentages
    void calculatePercentages() {
        if (totalBranches > 0) {
            directBranchesPercent = 100.0 * directBranches / totalBranches;
            indirectBranchesPercent = 100.0 * indirectBranches / totalBranches;
            conditionalBranchesPercent = 100.0 * conditionalBranches / totalBranches;
            unconditionalBranchesPercent = 100.0 * unconditionalBranches / totalBranches;
            regularBranchesPercent = 100.0 * regularBranches / totalBranches;
            callInstructionsPercent = 100.0 * callInstructions / totalBranches;
            returnInstructionsPercent = 100.0 * returnInstructions / totalBranches;
            takenBranchesPercent = 100.0 * takenBranches / totalBranches;
        }
        
        if (conditionalBranches > 0) {
            condTakenBranchesPercent = 100.0 * condTakenBranches / conditionalBranches;
        }
        
        if (uniqueBranchLocations > 0) {
            highlyPredictableAllPercent = 100.0 * highlyPredictableAll / uniqueBranchLocations;
        }
        
        if (uniqueCondBranchLocations > 0) {
            highlyPredictableCondPercent = 100.0 * highlyPredictableCond / uniqueCondBranchLocations;
        }
    }

    // Calculate pattern percentages and sort them
    void calculatePatternStats() {
        // Calculate PC pattern percentages (as % of total branches)
        pcPatterns.clear();
        for (const auto& entry : rawPCPatternCounts) {
            double percentage = 100.0 * entry.second / totalBranches;
            pcPatterns.emplace_back(entry.first, percentage);
        }
        
        // Sort PC patterns by percentage (descending)
        std::sort(pcPatterns.begin(), pcPatterns.end());
        
        // Calculate taken pattern percentages (as % of conditional branches)
        takenPatterns.clear();
        if (conditionalBranches > 0) {
            for (const auto& entry : rawTakenPatternCounts) {
                double percentage = 100.0 * entry.second / conditionalBranches;
                takenPatterns.emplace_back(entry.first, percentage);
            }
        }
        
        // Sort taken patterns by percentage (descending)
        std::sort(takenPatterns.begin(), takenPatterns.end());
    }
    
    // Percentage fields (for convenience)
    double directBranchesPercent = 0.0;
    double indirectBranchesPercent = 0.0;
    double conditionalBranchesPercent = 0.0;
    double unconditionalBranchesPercent = 0.0;
    double regularBranchesPercent = 0.0;
    double callInstructionsPercent = 0.0;
    double returnInstructionsPercent = 0.0;
    double takenBranchesPercent = 0.0;
    double condTakenBranchesPercent = 0.0;
    double highlyPredictableAllPercent = 0.0;
    double highlyPredictableCondPercent = 0.0;
};



// Analyze a single trace file and return metrics
BranchMetrics analyzeBranchTrace(const std::string& filename, size_t maxLines = 0) {
    BranchMetrics metrics;
    metrics.traceName = getTraceBaseName(filename);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return metrics;
    }
    
    // ==== branch history ====
    std::unordered_map<uint64_t, size_t> branchExecutions;  // PC -> execution count
    std::unordered_map<uint64_t, std::pair<size_t, size_t>> allBranchStats;     // PC -> (taken, total)
    std::unordered_map<uint64_t, std::pair<size_t, size_t>> conditionalStats;   // PC -> (taken, total)
    
    // Track whether a branch is conditional
    std::unordered_map<uint64_t, bool> isConditional;  // PC -> isConditional flag
    
    // ==== locality analysis ====
    const size_t HISTORY_LENGTH = 4;
    std::deque<uint64_t> recentPCs;  // recent PCs for locality analysis
    
    // record patterns
    std::unordered_map<std::string, size_t> pcPatternCounts;
    std::unordered_map<std::string, size_t> takenPatternCounts;
    
    // process the trace file line by line
    std::string line;
    while (std::getline(file, line) && (maxLines == 0 || metrics.totalBranches < maxLines)) {
        Branch branch;
        std::istringstream iss(line);
        std::string pc, target, kind, direct, conditional, taken;
        
        if (!(iss >> std::hex >> branch.pc
                  >> std::hex >> branch.target
                  >> branch.kind
                  >> branch.direct
                  >> branch.conditional
                  >> branch.taken)) continue;
        
        metrics.totalBranches++;
        
        // ==== basic counters ====
        if (branch.direct) metrics.directBranches++;
        if (branch.conditional) metrics.conditionalBranches++;
        if (branch.taken) metrics.takenBranches++;
        
        // ==== branch kind ====
        if (branch.kind == 'b') metrics.regularBranches++;
        else if (branch.kind == 'c') metrics.callInstructions++;
        else if (branch.kind == 'r') metrics.returnInstructions++;
        
        // ==== branch execution count ====
        branchExecutions[branch.pc]++;
        
        // update all branches statistics, PC -> (taken, total)
        allBranchStats[branch.pc].second++;  // total execution count
        if (branch.taken) allBranchStats[branch.pc].first++;  // taken count
        
        // Track whether this branch is conditional
        isConditional[branch.pc] = branch.conditional;
        
        // update conditional branches statistics, PC -> (taken, total)
        if (branch.conditional) {
            conditionalStats[branch.pc].second++;  // total execution count
            if (branch.taken) {
                conditionalStats[branch.pc].first++;  // taken count
                metrics.condTakenBranches++;
            }
        }
        
        // ==== locality analysis ====
        if (!recentPCs.empty()) {
            // record the pattern of recent PCs
            std::string pcPattern = "";
            for (size_t i = 0; i < std::min(recentPCs.size(), HISTORY_LENGTH); i++) {
                pcPattern += (recentPCs[i] == branch.pc) ? "S" : "D";  // Same/Different PC
            }
            pcPatternCounts[pcPattern]++;
            
            // record the pattern of taken/not-taken, only for conditional branches
            if (branch.conditional && conditionalStats[branch.pc].second >= 2) {
                std::string takenPattern = "";
                for (size_t i = 0; i < std::min(recentPCs.size(), HISTORY_LENGTH); i++) {
                    if (recentPCs[i] == branch.pc) {
                        // retrieve the direction of the previous branch
                        double takenRatio = (double)conditionalStats[branch.pc].first / 
                                           conditionalStats[branch.pc].second;
                        takenPattern += (takenRatio > 0.5) ? "T" : "N";  // nominally taken/not-taken
                    } else {
                        takenPattern += "X";  // if not the same PC, use 'X'
                    }
                }
                takenPatternCounts[takenPattern]++;
            }
        }
        
        // update recent PCs
        recentPCs.push_front(branch.pc);
        if (recentPCs.size() > HISTORY_LENGTH) {
            recentPCs.pop_back();
        }
    }
    
    // Calculate derived metrics
    metrics.indirectBranches = metrics.totalBranches - metrics.directBranches;
    metrics.unconditionalBranches = metrics.totalBranches - metrics.conditionalBranches;
    metrics.uniqueBranchLocations = allBranchStats.size();
    metrics.uniqueCondBranchLocations = conditionalStats.size();
    
    // ==== calculate predictability metrics ====
    for (const auto& entry : allBranchStats) {
        double takenRatio = static_cast<double>(entry.second.first) / entry.second.second;
        if (takenRatio > 0.95 || takenRatio < 0.05) {
            metrics.highlyPredictableAll++;
        }
    }
    
    for (const auto& entry : conditionalStats) {
        double takenRatio = static_cast<double>(entry.second.first) / entry.second.second;
        if (takenRatio > 0.95 || takenRatio < 0.05) {
            metrics.highlyPredictableCond++;
        }
    }
    
    // ==== hotspot analysis ====
    std::vector<std::pair<uint64_t, size_t>> hotspots;
    for (const auto& pair : branchExecutions) {
        hotspots.push_back(pair);
    }
    std::sort(hotspots.begin(), hotspots.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Calculate hotspot percentage (top 5)
    size_t hotspotTotal = 0;
    size_t top5Count = std::min(hotspots.size(), (size_t)5);
    
    for (size_t i = 0; i < top5Count; i++) {
        hotspotTotal += hotspots[i].second;
        
        // get the taken ratio for the hotspot
        double takenRatio = 0.0;
        auto it = allBranchStats.find(hotspots[i].first);
        if (it != allBranchStats.end()) {
            takenRatio = 100.0 * it->second.first / it->second.second;
        }
        
        // Add to top hotspots
        BranchMetrics::Hotspot hotspot;
        hotspot.address = hotspots[i].first;
        hotspot.executions = hotspots[i].second;
        hotspot.executionPercentage = 100.0 * hotspots[i].second / metrics.totalBranches;
        hotspot.takenPercentage = takenRatio;
        hotspot.isConditional = isConditional[hotspots[i].first];
        metrics.topHotspots.push_back(hotspot);
    }
    
    metrics.hotspotPercentage = 100.0 * hotspotTotal / metrics.totalBranches;
    
    // Store raw PC pattern counts
    metrics.rawPCPatternCounts = std::map<std::string, size_t>(pcPatternCounts.begin(), pcPatternCounts.end());
    
    // Store raw taken pattern counts
    metrics.rawTakenPatternCounts = std::map<std::string, size_t>(takenPatternCounts.begin(), pcPatternCounts.end());
    
    // Calculate percentages
    metrics.calculatePercentages();
    metrics.calculatePatternStats();
    
    return metrics;
}

// Function to analyze multiple trace files and create pandas-friendly CSV files
void createPandasFriendlyCSV(const std::vector<std::string>& traceFiles, 
                             const std::string& outputDir = "results",
                             size_t maxLines = 0) {
    
    // Create directory for analysis if it doesn't exist
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }
    
    // Define output paths
    std::string mainCSV = outputDir + "/trace_comparison.csv";
    std::string pcPatternsCSV = outputDir + "/pc_patterns_by_rank.csv";
    std::string takenPatternsCSV = outputDir + "/taken_patterns_by_rank.csv";
    std::string hotspotsCSV = outputDir + "/trace_hotspots.csv";
    
    // Analyze each trace file
    std::vector<BranchMetrics> allMetrics;
    for (const auto& traceFile : traceFiles) {
        std::cout << "Analyzing " << traceFile << "..." << std::endl;
        BranchMetrics metrics = analyzeBranchTrace(traceFile, maxLines);
        allMetrics.push_back(metrics);
    }
    
    // Create main CSV file with basic metrics
    std::ofstream mainFile(mainCSV);
    if (!mainFile.is_open()) {
        std::cerr << "Error: Could not create CSV file " << mainCSV << std::endl;
        return;
    }
    
    // Write main CSV header
    mainFile << "TraceName,"
             << "TotalBranches,"
             << "DirectBranches_pct,"
             << "IndirectBranches_pct,"
             << "ConditionalBranches_pct,"
             << "UnconditionalBranches_pct,"
             << "RegularBranches_pct,"
             << "FunctionCalls_pct,"
             << "FunctionReturns_pct,"
             << "TakenBranches_pct,"
             << "ConditionalTaken_pct,"
             << "UniqueBranchLocations,"
             << "UniqueCondBranchLocations,"
             << "HighlyPredictableAll_pct,"
             << "HighlyPredictableCond_pct,"
             << "Top5HotspotPercentage" << std::endl;
    
    // Write main CSV data for each trace
    for (const auto& metrics : allMetrics) {
        mainFile << metrics.traceName << ","
                 << metrics.totalBranches << ","
                 << std::fixed << std::setprecision(2)
                 << metrics.directBranchesPercent << ","
                 << metrics.indirectBranchesPercent << ","
                 << metrics.conditionalBranchesPercent << ","
                 << metrics.unconditionalBranchesPercent << ","
                 << metrics.regularBranchesPercent << ","
                 << metrics.callInstructionsPercent << ","
                 << metrics.returnInstructionsPercent << ","
                 << metrics.takenBranchesPercent << ","
                 << metrics.condTakenBranchesPercent << ","
                 << metrics.uniqueBranchLocations << ","
                 << metrics.uniqueCondBranchLocations << ","
                 << metrics.highlyPredictableAllPercent << ","
                 << metrics.highlyPredictableCondPercent << ","
                 << metrics.hotspotPercentage << std::endl;
    }
    
    mainFile.close();
    std::cout << "Main metrics CSV exported to " << mainCSV << std::endl;
    
    // =================== Create PC patterns by rank CSV file ===================
    std::ofstream pcPatternsFile(pcPatternsCSV);
    if (!pcPatternsFile.is_open()) {
        std::cerr << "Error: Could not create CSV file " << pcPatternsCSV << std::endl;
        return;
    }
    
    // Find the maximum number of PC patterns across all traces
    size_t maxPCPatterns = 0;
    for (const auto& metrics : allMetrics) {
        maxPCPatterns = std::max(maxPCPatterns, metrics.pcPatterns.size());
    }
    
    // Write PC patterns CSV header
    pcPatternsFile << "Rank";
    for (const auto& metrics : allMetrics) {
        pcPatternsFile << "," << metrics.traceName << "_Pattern," 
                       << metrics.traceName << "_pct";
    }
    pcPatternsFile << std::endl;
    
    // Write PC patterns by rank
    for (size_t rank = 0; rank < maxPCPatterns; rank++) {
        pcPatternsFile << (rank + 1);  // Rank starts from 1
        
        for (const auto& metrics : allMetrics) {
            if (rank < metrics.pcPatterns.size()) {
                pcPatternsFile << "," << metrics.pcPatterns[rank].pattern
                              << "," << std::fixed << std::setprecision(2) << metrics.pcPatterns[rank].percentage;
            } else {
                pcPatternsFile << ",-,0.00";  // Placeholder for missing patterns
            }
        }
        
        pcPatternsFile << std::endl;
    }
    
    pcPatternsFile.close();
    std::cout << "PC patterns by rank CSV exported to " << pcPatternsCSV << std::endl;
    
    // =================== Create taken patterns by rank CSV file ===================
    std::ofstream takenPatternsFile(takenPatternsCSV);
    if (!takenPatternsFile.is_open()) {
        std::cerr << "Error: Could not create CSV file " << takenPatternsCSV << std::endl;
        return;
    }
    
    // Find the maximum number of taken patterns across all traces
    size_t maxTakenPatterns = 0;
    for (const auto& metrics : allMetrics) {
        maxTakenPatterns = std::max(maxTakenPatterns, metrics.takenPatterns.size());
    }
    
    // Write taken patterns CSV header
    takenPatternsFile << "Rank";
    for (const auto& metrics : allMetrics) {
        takenPatternsFile << "," << metrics.traceName << "_Pattern," 
                          << metrics.traceName << "_pct";
    }
    takenPatternsFile << std::endl;
    
    // Write taken patterns by rank
    for (size_t rank = 0; rank < maxTakenPatterns; rank++) {
        takenPatternsFile << (rank + 1);  // Rank starts from 1
        
        for (const auto& metrics : allMetrics) {
            if (rank < metrics.takenPatterns.size()) {
                takenPatternsFile << "," << metrics.takenPatterns[rank].pattern
                                 << "," << std::fixed << std::setprecision(2) << metrics.takenPatterns[rank].percentage;
            } else {
                takenPatternsFile << ",-,0.00";  // Placeholder for missing patterns
            }
        }
        
        takenPatternsFile << std::endl;
    }
    
    takenPatternsFile.close();
    std::cout << "Taken patterns by rank CSV exported to " << takenPatternsCSV << std::endl;
    
    // =================== Create hotspots CSV file ===================
    std::ofstream hotspotsFile(hotspotsCSV);
    if (!hotspotsFile.is_open()) {
        std::cerr << "Error: Could not create CSV file " << hotspotsCSV << std::endl;
        return;
    }
    
    // Write hotspots CSV header
    hotspotsFile << "TraceName";
    for (int i = 1; i <= 5; i++) {
        hotspotsFile << ",Hotspot" << i << "_Addr,"
                     << "Hotspot" << i << "_ExecPct,"
                     << "Hotspot" << i << "_TakenPct,"
                     << "Hotspot" << i << "_IsConditional";
    }
    hotspotsFile << std::endl;
    
    // Write hotspots CSV data for each trace
    for (const auto& metrics : allMetrics) {
        hotspotsFile << metrics.traceName;
        
        // Add hotspot data
        for (size_t i = 0; i < 5; i++) {
            if (i < metrics.topHotspots.size()) {
                const auto& hotspot = metrics.topHotspots[i];
                hotspotsFile << ",0x" << std::hex << hotspot.address << std::dec << ","
                             << std::fixed << std::setprecision(2) << hotspot.executionPercentage << ","
                             << hotspot.takenPercentage << ","
                             << (hotspot.isConditional ? "1" : "0");
            } else {
                hotspotsFile << ",0,0.00,0.00,0";
            }
        }
        
        hotspotsFile << std::endl;
    }
    
    hotspotsFile.close();
    std::cout << "Branch hotspots CSV exported to " << hotspotsCSV << std::endl;
    
    // Also print a simple summary to console
    std::cout << "\n===== Trace Analysis Summary =====\n";
    for (const auto& metrics : allMetrics) {
        std::cout << "Trace: " << metrics.traceName << "\n"
                  << "  - Total branches: " << metrics.totalBranches << "\n"
                  << "  - Conditional branches: " << metrics.conditionalBranchesPercent << "%\n"
                  << "  - Highly predictable cond. branches: " << metrics.highlyPredictableCondPercent << "%\n"
                  << "  - Top 5 hotspot percentage: " << metrics.hotspotPercentage << "%\n";
                  
        // Show top taken patterns
        if (!metrics.takenPatterns.empty()) {
            std::cout << "  - Top taken pattern: ";
            std::cout << metrics.takenPatterns[0].pattern << " (" << metrics.takenPatterns[0].percentage << "% of conditional branches)\n";
        }
    }
    std::cout << "================================\n\n";
}
