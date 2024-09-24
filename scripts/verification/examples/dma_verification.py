#
#     Copyright EPFL contributors.
#     Licensed under the Apache License, Version 2.0, see LICENSE for details.
#     SPDX-License-Identifier: Apache-2.0
#
#     Author: Tommaso Terzano <tommaso.terzano@epfl.ch>
#                             <tommaso.terzano@gmail.com>
#
#     Info: This is a usecase of the VerifHeep tool. It generates a dataset for the im2col function 
#           and the golden result, then it runs the test on the PYNQ-Z2 board and stores the results 
#           in a file. The data can be then read and plotted using the plotter.py script.
#           
#           In order to be run, these steps has to be followed:
#           0. Set the X-Heep configuration to have 4 channels,
#              while there are no limits on the number of master ports
#           1. Run this command to include Verifheep: 
#               export PYTHONPATH="$PYTHONPATH:/<X-Heep project directory in your system>/scripts/verification"
#           2. Perform the synthesis with Vivado and to program the FPGA with the bitstream
#           3. Connect the board using GDB by running X-Heep script "make openOCD_bscan". Close Vivado to avoid conflicts
#

import re
import time
import verifheep
from tqdm import tqdm
import curses
import torch
import torch.nn.functional as F
import numpy as np

# Define the USB port to which the board is connected. Useful because the port may change
USBport = 2

# Define the parameters for the test

datatype = "uint32_t"
range_max = 65500

num_masters = 4 # Number of DMA CH
num_slaves = 2 # Number of BUS master ports
max_masters_per_slave = 2 # Maximum number of DMA CH per BUS master port

in_w_max = 10
in_w_min = 5

in_h_max = 10
in_h_min = 5

pad_top_max = 2
pad_top_min = 1
pad_bottom_max = 2
pad_bottom_min = 1
pad_left_max = 2
pad_left_min = 1
pad_right_max = 2
pad_right_min = 1

stride_d1_max = 2
stride_d1_min = 1
stride_d2_max = 2
stride_d2_min = 1

# Calculate the total number of iterations
total_iterations = ((stride_d2_max - stride_d2_min) * (stride_d1_max - stride_d1_min) *
                    (pad_right_max - pad_right_min) * (pad_left_max - pad_left_min) *
                    (pad_bottom_max - pad_bottom_min) * (pad_top_max - pad_top_min) *
                    (in_w_max - in_w_min) * (in_h_max - in_h_min))

# Define the patterns to be used for modifying the im2col_lib.h file
test_id_0_pattern = re.compile(r'#define TEST_ID_0')
test_id_1_pattern = re.compile(r'#define TEST_ID_1')
test_id_2_pattern = re.compile(r'#define TEST_ID_2')
test_id_3_pattern = re.compile(r'#define TEST_ID_3')
en_perf_pattern = re.compile(r'#define EN_PERF \d+')
en_ver_pattern = re.compile(r'#define EN_VERIF \d+')
test_en_pattern = re.compile(r'#define TEST_EN \d+')

size_extr_d1_pattern = re.compile(r'#define SIZE_EXTR_D1 \d+')
size_extr_d2_pattern = re.compile(r'#define SIZE_EXTR_D2 \d+')
stride_in_d1_pattern = re.compile(r'#define STRIDE_IN_D1 \d+')
stride_in_d2_pattern = re.compile(r'#define STRIDE_IN_D2 \d+')
stride_out_d1_pattern = re.compile(r'#define STRIDE_OUT_D1 \d+')
stride_out_d2_pattern = re.compile(r'#define STRIDE_OUT_D2 \d+')
top_pad_pattern = re.compile(r'#define TOP_PAD \d+')
bottom_pad_pattern = re.compile(r'#define BOTTOM_PAD \d+')
left_pad_pattern = re.compile(r'#define LEFT_PAD \d+')
right_pad_pattern = re.compile(r'#define RIGHT_PAD \d+')
dma_input_data_type_pattern = re.compile(r'typedef\s+(\w+)\s+dma_input_data_type;')

# Define the arrays to store the data
cpu_array = []
dma_hal_array = []

def get_dma_data_type(dma_datatype):
    switcher = {
        'uint8_t': 'DMA_DATA_TYPE_BYTE',
        'uint16_t': 'DMA_DATA_TYPE_HALF_BYTE',
        'uint32_t': 'DMA_DATA_TYPE_WORD'
    }
    return switcher.get(dma_datatype, "Unknown data type")

# Initialize the VerifHeep tool
dmaVer = verifheep.VerifHeep("pynq-z2", "../../../")

# Connect to the pynq-z2 board
print("Connecting to the board...")
serial_status = dmaVer.serialBegin(f"/dev/ttyUSB{USBport}", 9600)
if not serial_status:
    print("Error connecting to the board")
    exit(1)
else:
    print("Connected!\n")
    time.sleep(1)

# Set up the debug interface
dmaVer.setUpDeb()

def main(stdscr):

  # Initialize the progress bar
  progress_bar = tqdm(total=total_iterations, desc="Overall Progress", ncols=100, unit=" iter",
                   bar_format='{desc}: {percentage:.2f}%|{bar}| {n_fmt}/{total_fmt}')

  cpu_done = 0
  iteration = 1
  started = False
  counter = 10

  # Set the correct output format by setting the TEST_EN define
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", en_perf_pattern, f'#define EN_PERF 1')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", en_ver_pattern, f'#define EN_VERIF 1')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", test_en_pattern, f'#define TEST_EN 1')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", test_id_0_pattern, f'//#define TEST_ID_0')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", test_id_1_pattern, f'#define TEST_ID_1')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", test_id_2_pattern, f'//#define TEST_ID_2')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", test_id_3_pattern, f'//#define TEST_ID_3') 
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", dma_input_data_type_pattern, f'typedef {datatype} dma_input_data_type;')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", stride_out_d1_pattern, f'#define STRIDE_OUT_D1 1')
  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", stride_out_d2_pattern, f'#define STRIDE_OUT_D2 1')
                                                                                       
  curses.curs_set(0)
  for j in range(in_w_min, in_w_max):
      dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", size_extr_d1_pattern, f'#define SIZE_EXTR_D1 {j}')
          
      for k in range(in_h_min, in_h_max):
        dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", size_extr_d2_pattern, f'#define SIZE_EXTR_D2 {k}')
                    
        for p in range(pad_top_min, pad_top_max):
        
          dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", top_pad_pattern, f'#define TOP_PAD {p}')
                  
          for q in range(pad_bottom_min, pad_bottom_max):

              dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", bottom_pad_pattern, f'#define BOTTOM_PAD {q}')
                  
              for r in range(pad_left_min, pad_left_max):

                  dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", left_pad_pattern, f'#define LEFT_PAD {r}')
                      
                  for s in range(pad_right_min, pad_right_max):
                      
                      dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", right_pad_pattern, f'#define RIGHT_PAD {s}')

                      for t in range(stride_d1_min, stride_d1_max):

                          dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", stride_in_d1_pattern, f'#define STRIDE_IN_D1 {t}')

                          for u in range(stride_d2_min, stride_d2_max):
                              
                              cpu = []
                              dma_hal = []
                              
                              # Start the chrono
                              if started and counter == 0:
                                dmaVer.stopDeb()
                                dmaVer.setUpDeb()
                                counter = 10
                              elif not started:
                                dmaVer.setUpDeb()
                                started = True
                                counter -= 1
                              else:                                                      
                                counter -= 1
                              
                              dmaVer.chronoStart()
                              dma_datatype = get_dma_data_type(datatype)

                              parameters = {
                                  'SIZE_IN_D1': j,
                                  'SIZE_IN_D2': k,
                                  'DMA_DATA_TYPE': dma_datatype
                              }

                              input_size = j * k

                              dmaVer.modifyFile("../../../sw/applications/example_dma_2d/main.c", stride_in_d2_pattern, f'#define STRIDE_IN_D2 {u}')


                              dmaVer.genInputDataset(input_size, row_size=j, range_max=range_max,dataset_dir="../../../sw/applications/example_dma_2d/test_data.h", parameters=parameters, dataset_name="test_data",
                                                        datatype=datatype)
                              
                              # Launch the test
                              dmaVer.launchTest("example_dma_2d", pattern=r'(\w+):(\d+):(\d+)', input_size=j*k)

                              # Format the parameters of the current run and store them for plots
                              for test in dmaVer.results:
                                  string = f'H: {j}, W: {k}, PT: {p}, PB: {q}, PL: {r}, PR: {s}, S1: {t}, S2: {u}, cycles: {test["Cycles"]}'
                                  
                                  if test["ID"] == "1a":
                                      cpu.append(string)
                                  elif test["ID"] == "1b":
                                      dma_hal.append(string)
                              
                              # Stop the chrono and calculate the remaining time of the verification
                              dmaVer.clearResults()
                              dmaVer.chronoStop()
                              time_rem = dmaVer.chronoExecutionEst(total_iterations)
                              
                              # Update the progress bar
                              message = (
                                  f"Input height:  {j:>5}\n"
                                  f"Input width:   {k:>5}\n"
                                  f"Pad top:       {p:>5}\n"
                                  f"Pad bottom:    {q:>5}\n"
                                  f"Pad left:      {r:>5}\n"
                                  f"Pad right:     {s:>5}\n"
                                  f"Stride d1:     {t:>5}\n"
                                  f"Stride d2:     {u:>5}\n"
                                  f"Remaining time:{time_rem['hours']:>2}h:{time_rem['minutes']:>2}m:{time_rem['seconds']:.2f}s\n"
                              )

                              iteration += 1
                              progress_bar.update(1)
                              
                              stdscr.addstr(1, 0, message)
                              stdscr.refresh()

                              cpu_array.append(cpu)
                              dma_hal_array.append(dma_hal)

  # Stop the debug interface and close the progress bar
  dmaVer.stopDeb()
  progress_bar.close()

  # Write the data to a file
  with open('dma_data.txt', 'w') as file:
      file.write("cpu:\n")
      for value in cpu_array:
          file.write(f"{value}\n")
      file.write("\n")
      
      file.write("dma_hal:\n")
      for value in dma_hal_array:
          file.write(f"{value}\n")
      file.write("\n")

  print("Data acquired!\n")

curses.wrapper(main)