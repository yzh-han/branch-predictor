#include "predictor/branch.hpp"
#include "predictor/predictor.hpp"
#include "utils/utils.hpp"
#include "utils/config.hpp"
#include "utils/analysis.hpp"


#include <iostream>
#include <fstream>
#include <string>

void runPredictor(std::vector<std::string> traceFiles, size_t maxLines = 0, const std::string& csvFile = "results/results_predict.csv");


int main(int argc, char* argv[]) {

    runPredictor(config.TRACES);
    
    return 0;
}

void runPredictor(std::vector<std::string> traceFiles, size_t maxLines, const std::string& csvFile) {

    std::ofstream csv(csvFile);
    if (!csv.is_open()) {
        std::cerr << "Error: Could not open CSV file " << csvFile << std::endl;
        return;
    }
    csv << "TraceFile,Predictor,TotalBranches,Mispredictions,MispredictionRate\n";

    for(std::string traceFile: traceFiles) {    
        std::string traceName = getTraceBaseName(traceFile);
        
        // write the header


        std::cout << "Branch Predictor Simulator" << std::endl;
        std::cout << "=========================" << std::endl;
        std::cout << "Trace file: " << traceFile << std::endl;
        if (maxLines > 0) {
            std::cout << "Max lines: " << maxLines << std::endl;
        }
        std::cout << std::endl;
        
        // -------------------------------------------------------------
        // Create and evaluate the Always Taken predictor
        AlwaysTakenPredictor alwaysTaken;
        std::cout << "Evaluating Always Taken predictor..." << std::endl;
        auto result = evaluatePredictor(alwaysTaken, traceFile, maxLines);
        size_t totalBranches = result[0];
        size_t mispredictions = result[1];
        double mispredictionRate = (totalBranches > 0) ? 
            (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;
        
        // write the result to csv
        csv << traceName << ","
        << alwaysTaken.getName() << ","
        << totalBranches << ","
        << mispredictions << ","
        << std::fixed << std::setprecision(2) << mispredictionRate << "\n";
        // -------------------------------------------------------------

        // -------------------------------------------------------------
        // Create and evaluate 2-bit predictors with different table sizes
        std::vector<size_t> tableSizes = {512, 1024, 2048, 4096};
        for (size_t size : tableSizes) {
            TwoBitPredictor twoBit(size);
            std::cout << "Evaluating " << twoBit.getName() << " predictor..." << std::endl;
            result = evaluatePredictor(twoBit, traceFile, maxLines);
            totalBranches = result[0];
            mispredictions = result[1];
            mispredictionRate = (totalBranches > 0) ? 
                (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;

            csv << traceName << ","
                << twoBit.getName() << ","
                << totalBranches << ","
                << mispredictions << ","
                << std::fixed << std::setprecision(2) << mispredictionRate << "\n";
        }
        // -------------------------------------------------------------

        // -------------------------------------------------------------
        // Create and evaluate gshare predictor
        GSharePredictor gshare(2048);
        std::cout << "Evaluating gshare predictor..." << std::endl;
        result = evaluatePredictor(gshare, traceFile, maxLines);
        totalBranches = result[0];
        mispredictions = result[1];
        mispredictionRate = (totalBranches > 0) ? 
            (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;

        csv << traceName << ","
            << gshare.getName() << ","
            << totalBranches << ","
            << mispredictions << ","
            << std::fixed << std::setprecision(2) << mispredictionRate << "\n";
        // -------------------------------------------------------------

        // -------------------------------------------------------------
        // Create and evaluate profiled 2bit predictor
        ProfiledPredictor profiled(2048);
        std::cout << "Evaluating profiled predictor..." << std::endl;
        auto profiledResult = evaluateProfiledPredictor(profiled, traceFile, maxLines);
        totalBranches = profiledResult[0];
        mispredictions = profiledResult[1];
        mispredictionRate = (totalBranches > 0) ? 
            (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;

        csv << traceName << ","
            << profiled.getName() << ","
            << totalBranches << ","
            << mispredictions << ","
            << std::fixed << std::setprecision(2) << mispredictionRate << "\n";
        // -------------------------------------------------------------

        // -------------------------------------------------------------
        // Create and evaluate profiled 2bit predictor
        Profiled2BitPredictor profiled2Bit(2048);
        std::cout << "Evaluating profiled predictor..." << std::endl;
        profiledResult = evaluateProfiled2BitPredictor(profiled2Bit, traceFile, maxLines);
        totalBranches = profiledResult[0];
        mispredictions = profiledResult[1];
        mispredictionRate = (totalBranches > 0) ? 
            (static_cast<double>(mispredictions) / totalBranches) * 100.0 : 0.0;

        csv << traceName << ","
            << profiled2Bit.getName() << ","
            << totalBranches << ","
            << mispredictions << ","
            << std::fixed << std::setprecision(2) << mispredictionRate << "\n";
        // -------------------------------------------------------------
    }
    csv.close();
        std::cout << "Results written to " << csvFile << std::endl;
}