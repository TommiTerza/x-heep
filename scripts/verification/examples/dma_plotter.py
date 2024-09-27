#
#     Copyright EPFL contributors.
#     Licensed under the Apache License, Version 2.0, see LICENSE for details.
#     SPDX-License-Identifier: Apache-2.0
#
#     Author: Tommaso Terzano <tommaso.terzano@epfl.ch>
#                             <tommaso.terzano@gmail.com>
#
#     Info: Plotter script for im2col spc verification data.
#
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Function to read and parse data from file
def read_data(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    cpu_data = []
    dma_hal_data = []
    dma_no_hal_data = []

    current_test_type = None

    for line in lines:
        line = line.strip()
        if line == '':
            continue  # Skip empty lines
        if line.startswith('cpu:'):
            current_test_type = 'cpu'
            continue
        elif line.startswith('dma_hal:'):
            current_test_type = 'dma_hal'
            continue
        elif line.startswith('dma_no_hal:'):
            current_test_type = 'dma_no_hal'
            continue
        else:
            # This line should be a list like: ['...', '...']
            # Let's parse it
            line = line.strip('[]')
            entries = line.split("', '")
            # Remove leading and trailing single quotes
            entries[0] = entries[0].lstrip("'")
            entries[-1] = entries[-1].rstrip("'")
            # Now entries is a list of strings
            if current_test_type == 'cpu':
                cpu_data.extend(entries)
            elif current_test_type == 'dma_hal':
                dma_hal_data.extend(entries)
            elif current_test_type == 'dma_no_hal':
                dma_no_hal_data.extend(entries)

    return cpu_data, dma_hal_data, dma_no_hal_data

# Function to parse each entry into a dictionary
def parse_entry(entry):
    pairs = entry.split(', ')
    entry_dict = {}
    for pair in pairs:
        key, value = pair.split(': ')
        # Convert values to integers
        entry_dict[key] = int(value)
    return entry_dict

# Function to compute loop size
def compute_loop_size(entry):
    loop_size = int((entry['H'] + entry['PT'] + entry['PB'])/ entry['S2']) * ((entry['W'] + entry['PL'] + entry['PR'])/ entry['S1'])
    return loop_size

# Function to calculate axis limits based on data
def calculate_axis_limits(df_cpu, df_dma_hal, df_dma_no_hal):
    # Combine all loop_size and cycle data
    all_loop_sizes = pd.concat([df_cpu['loop_size'], df_dma_hal['loop_size'], df_dma_no_hal['loop_size']])
    all_cycles = pd.concat([df_cpu['cycles'], df_dma_hal['cycles'], df_dma_no_hal['cycles']])
    
    # Compute the limits for both axes
    xlim = (0, all_loop_sizes.max()*1.1)
    ylim = (0, all_cycles.max()*1.1)
    
    return xlim, ylim

# Function to plot data
def plot_data(df_cpu, df_dma_hal, df_dma_no_hal, padding, title_suffix, filename_suffix, xlim=None, ylim=None):
    plt.figure(figsize=(20, 12))
    
    if padding is None:
        df_cpu_filtered = df_cpu
        df_dma_hal_filtered = df_dma_hal
        df_dma_no_hal_filtered = df_dma_no_hal
    else:
        # Filter data based on padding values
        df_cpu_filtered = df_cpu[(df_cpu['PT'] == padding) & (df_cpu['PB'] == padding) & (df_cpu['PL'] == padding) & (df_cpu['PR'] == padding)]
        df_dma_hal_filtered = df_dma_hal[(df_dma_hal['PT'] == padding) & (df_dma_hal['PB'] == padding) & (df_dma_hal['PL'] == padding) & (df_dma_hal['PR'] == padding)]
        df_dma_no_hal_filtered = df_dma_no_hal[(df_dma_no_hal['PT'] == padding) & (df_dma_no_hal['PB'] == padding) & (df_dma_no_hal['PL'] == padding) & (df_dma_no_hal['PR'] == padding)]

    # Scatter plots
    plt.scatter(df_cpu_filtered['loop_size'], df_cpu_filtered['cycles'], color='blue', label='CPU')
    plt.scatter(df_dma_hal_filtered['loop_size'], df_dma_hal_filtered['cycles'], color='red', label='DMA with HAL')
    plt.scatter(df_dma_no_hal_filtered['loop_size'], df_dma_no_hal_filtered['cycles'], color='green', label='DMA with Direct Register Configuration')

    # Trendline plots
    if not df_cpu_filtered.empty:
        p_cpu = np.polyfit(df_cpu_filtered['loop_size'], df_cpu_filtered['cycles'], 1)
        trendline_cpu = np.polyval(p_cpu, df_cpu_filtered['loop_size'])
        plt.plot(df_cpu_filtered['loop_size'], trendline_cpu, color='blue', linestyle='--')
    
    if not df_dma_hal_filtered.empty:
        p_dma = np.polyfit(df_dma_hal_filtered['loop_size'], df_dma_hal_filtered['cycles'], 1)
        trendline_dma = np.polyval(p_dma, df_dma_hal_filtered['loop_size'])
        plt.plot(df_dma_hal_filtered['loop_size'], trendline_dma, color='red', linestyle='--')

    if not df_dma_no_hal_filtered.empty:
        p_spc = np.polyfit(df_dma_no_hal_filtered['loop_size'], df_dma_no_hal_filtered['cycles'], 1)
        trendline_spc = np.polyval(p_spc, df_dma_no_hal_filtered['loop_size'])
        plt.plot(df_dma_no_hal_filtered['loop_size'], trendline_spc, color='green', linestyle='--')

    # Set xlim and ylim if provided
    if xlim and ylim:
        plt.xlim(xlim)
        plt.ylim(ylim)

    # Labels and title
    #plt.xlabel('Output Size')
    #plt.ylabel('Cycles')
    #plt.title(f'Output Size vs Cycles - {title_suffix}')
    #plt.legend()
    plt.grid(True)
    
    # Adjust tick label size
    plt.tick_params(axis='both', which='major', labelsize=22)

    # Save plot as SVG
    plt.savefig(f'dma_plot_padding_{filename_suffix}.svg', format='svg')

    # Show plot
    plt.show()

def main():
    # Path to the file containing the data
    file_path = 'dma_data.txt'
    
    # Read data from the file
    cpu_data, dma_hal_data, dma_no_hal_data = read_data(file_path)
    
    # Parse entries
    cpu_entries = [parse_entry(entry) for entry in cpu_data]
    dma_hal_entries = [parse_entry(entry) for entry in dma_hal_data]
    dma_no_hal_entries = [parse_entry(entry) for entry in dma_no_hal_data]
    
    # Compute loop_size and add to entries
    for entry in cpu_entries:
        entry['loop_size'] = compute_loop_size(entry)
    for entry in dma_hal_entries:
        entry['loop_size'] = compute_loop_size(entry)
    for entry in dma_no_hal_entries:
        entry['loop_size'] = compute_loop_size(entry)
    
    # Create DataFrames
    df_cpu = pd.DataFrame(cpu_entries)
    df_dma_hal = pd.DataFrame(dma_hal_entries)
    df_dma_no_hal = pd.DataFrame(dma_no_hal_entries)
    
    # Calculate the axis limits based on all data (with padding=None)
    xlim, ylim = calculate_axis_limits(df_cpu, df_dma_hal, df_dma_no_hal)
    
    # Main plot (all data, padding=None)
    plot_data(df_cpu, df_dma_hal, df_dma_no_hal, padding=None, title_suffix="(All Padding)", filename_suffix="all", xlim=xlim, ylim=ylim)

    # Additional plots for specific padding values using the same limits
    plot_data(df_cpu, df_dma_hal, df_dma_no_hal, padding=0, title_suffix="(Padding 0)", filename_suffix="0", xlim=xlim, ylim=ylim)
    plot_data(df_cpu, df_dma_hal, df_dma_no_hal, padding=1, title_suffix="(Padding 1)", filename_suffix="1", xlim=xlim, ylim=ylim)
    plot_data(df_cpu, df_dma_hal, df_dma_no_hal, padding=2, title_suffix="(Padding 2)", filename_suffix="2", xlim=xlim, ylim=ylim)

if __name__ == '__main__':
    main()
