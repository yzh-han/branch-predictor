#!/usr/bin/env python3

import os
import sys
import argparse
from pathlib import Path

def main():
    if len(sys.argv) > 4:
        print("Usage: python cut_trace.py [segment_size] [input_file] [output_dir]")
        sys.exit(1)

    segment_size = 1000000
    trace_path = None
    trace_paths = [
        # "../trace/bwaves.out",
        # "../trace/cactusbssn.out",
        "../trace/exchange2.out",
        "../trace/gcc.out",
        "../trace/leela.out",
        # "../trace/povray.out",
        "../trace/wrf.out",
        # "../trace/xz.out",
    ]
    output_dir = "trace"

    if len(sys.argv) >= 2:
        segment_size = int(sys.argv[1])
    if len(sys.argv) >= 3:
        trace_path = sys.argv[2]
    if len(sys.argv) >= 4:
        output_dir = sys.argv[3]

    if trace_path:
        extract_segments(trace_path, output_dir, segment_size)
    else:
        for trace_path in trace_paths:
            extract_segments(trace_path, output_dir, segment_size)



 

def count_lines(filename):
    """Count the number of lines in a file."""
    with open(filename, 'r') as f:
        return sum(1 for _ in f)

def extract_segments(input_file, output_dir, segment_size):
    """Extract segments from beginning, middle, and end of the trace file."""
    # Get total number of lines
    total_lines = count_lines(input_file)
    print(f"Total lines in {input_file}: {total_lines}")

    if total_lines < 4 * segment_size:
        print(f"File too small for segments of size {segment_size}.")
        return
    
    # Calculate middle starting point
    middle_start = (total_lines // 2) - (segment_size // 2)
    
    
    # Calculate end starting point
    end_start = total_lines - segment_size
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Get base filename without extension
    base_name = Path(input_file).stem
    
    # Define output files
    merged_output = os.path.join(output_dir, f"{base_name}_cutted.out")
    
    with open(input_file, 'r') as f, open(merged_output, 'w') as merged_file:
        # Extract and save beginning segment
        print(f"Processing beginning segment...")

        # Beginning segment
        # ------------------------------------------------
        for _ in range(segment_size):
            line = f.readline()
            if not line:
                break
            merged_file.write(line)
        # -------------------------------------------------
        
        # Skip to middle
        # -------------------------------------------------
        print(f"Skipping to middle segment...")
        skip_lines = middle_start - segment_size
        for _ in range(skip_lines):
            f.readline()
        
        # Extract and save middle segment
        print(f"Processing middle segment...")
        # -------------------------------------------------
        for _ in range(segment_size):
            line = f.readline()
            if not line:
                break
            merged_file.write(line)
        # -------------------------------------------------
        
        # Skip to end
        # -------------------------------------------------
        print(f"Skipping to end segment...")
        skip_lines = end_start - (middle_start + segment_size)
        for _ in range(skip_lines):
            f.readline()
        # -------------------------------------------------


        # Extract and save end segment
        # -------------------------------------------------
        print(f"Processing end segment...")

        for _ in range(segment_size):
            line = f.readline()
            if not line:
                break
            merged_file.write(line)
        # -------------------------------------------------
    
    print(f"Merged trace file written to {merged_output}")
    return merged_output


if __name__ == "__main__":
    main()