#include "utils/config.hpp"
#include "utils/analysis.hpp"

int main(int argc, char* argv[]) {
    createPandasFriendlyCSV(config.ORIGINAL_TRACES, "results", 0);
    return 0;
}