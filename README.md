# Branch Predictor

This is a branch predictor performance analysis project that implements multiple modern branch prediction algorithms and provides a comprehensive performance analysis and visualisation toolkit. The project focuses on studying the performance of different branch prediction strategies on real program traces.

This is a coursework for Computer Architecture course at the University of St Andrews.

## Trace files
Download [trace files](https://drive.google.com/drive/folders/1M8-p5ltp0Yb6Cu92aSeBpabiXMBXXlLY?usp=sharing), and put to `trace/`.

## Usage

### shell script

Using shell script, `run.sh` for automating the compilation, and experiment execution.

first move to repo's directory, and then:

```bash
./run.sh
```

### run predict experiment

```bash
# 1. compile
make clean all

# 2. run experiment 
./branch-predictor
```

results will save in `results/*.csv`

### run analyze_trace

require all 8 original trace file saved in `../trace`, **(not include in this repo)**

```bash
"../trace/bwaves.out",
"../trace/cactusbssn.out",
"../trace/exchange2.out",
"../trace/gcc.out",
"../trace/leela.out",
"../trace/povray.out",
"../trace/wrf.out",
"../trace/xz.out",  
```

```bash
# 1. compile
make clean all

# 2. run analyzer 
./trace-analyzer
```

### run cut trace

require all 8 original trace file saved in `../trace`

```bash
python cut_trace.py
```
cutted trace will saved in `trace/`

### run visualize generater

```bash
# 1. pip install dependencies
pip install -r requirements.txt

# 2. run visualize.py, it based on results_reported
python visualize.py
```

generated plots will save in `results/`

## Project Structure

```bash
repo
├── branch_predictor            # predictor project root dir
│   ├── analyze_traces.cpp      # entrace of analyze_traces
│   ├── main.cpp                # entrace of excute predictor experiment
│   ├── predictor               
│   │   ├── branch.hpp          # branch struct
│   │   ├── counter.hpp         # count State and update function
│   │   └── predictor.hpp       # all predictor implementation
│   └── utils
│       ├── analysis.hpp        # trace analyzer implementation
│       ├── config.hpp          # config, save trace path to run experiment
│       └── utils.hpp           # utils, include evaluate predictor function
├── cut_trace.py                # script to cut trace
├── Makefile
├── README.md
├── requirements.txt            # visualize python dependencies
├── results                                     # saved all results
│   ├── pc_patterns_by_rank.csv                 # trace analysis results
│   ├── plots_predictor_comparison_2b.png
│   ├── plots_predictor_comparison.png
│   ├── plots_trace_comparison.png
│   ├── results_predict.csv                     # predictor experiment results
│   ├── taken_patterns_by_rank.csv              # trace analysis results
│   ├── trace_comparison.csv                    # trace analysis results
│   └── trace_hotspots.csv                      # trace analysis results
├── run.sh
├── trace                       # cutted trance using for experiment
│   ├── exchange2_cutted.out
│   ├── gcc_cutted.out
│   ├── leela_cutted.out
│   └── wrf_cutted.out
└── visualize.py                # visualize python script
```