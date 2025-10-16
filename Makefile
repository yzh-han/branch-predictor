# define compiler
CXX = g++

FLAG = -Wall -I ${SRC_DIR} -MMD -fPIC

## Output binaries
TARGET_PREDICTOR = branch-predictor
TARGET_ANALYZER = trace-analyzer

## Directory structure
OBJ_DIR = obj
SRC_DIR = branch_predictor

## Find source files
SRC_ALL = $(shell find $(SRC_DIR) -type f -name "*.cpp")
ALL_OBJS := $(SRC_ALL:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

## Generate object file paths
PREDICTOR_OBJS = $(filter-out $(OBJ_DIR)/analyze_traces.o,$(ALL_OBJS))
ANALYZER_OBJS = $(filter-out $(OBJ_DIR)/main.o,$(ALL_OBJS))

## Phony targets
.PHONY: clean all

all: $(TARGET_PREDICTOR) $(TARGET_ANALYZER)

clean:
	rm -rf $(OBJ_DIR) $(TARGET_PREDICTOR) $(TARGET_ANALYZER)

## Main target rule
$(TARGET_PREDICTOR): $(PREDICTOR_OBJS)
	$(CXX) -o $@ $^

## Main target rule
$(TARGET_ANALYZER): $(ANALYZER_OBJS)
	$(CXX) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(FLAG) -c $< -o $@

## Include dependency files
# INCLUDE = third_party

-include $(ALL_OBJS:.o=.d)  	# Include main program dependencies


