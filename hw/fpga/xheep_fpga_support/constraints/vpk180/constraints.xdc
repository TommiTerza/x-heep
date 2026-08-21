# Copyright 2026 EPFL
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

# VPK180 timing constraints for the PS-enabled flow.

# The PL clock wizard and NoC IP XDCs create clocks for their LPDDR4 inputs.

# The SPI slave debug port remains an external clock domain even when PS_ENABLE=1.
create_clock -add -name spi_slave_clk_pin -period 16.000 -waveform {0.000 8.000} \
  [get_ports {spi_slave_sck_io}]

# Treat JTAG TAP clocking as asynchronous to the main X-HEEP system clock.
# The AXI JTAG IP XDC creates the generated TCK clock on tck_i_reg/Q, so do not
# create another clock here; collect the IP-owned clock from the generated pin.
set_clock_groups -quiet -asynchronous \
  -group [get_clocks -quiet -include_generated_clocks -of_objects \
    [get_ports -quiet {lpddr4_clk3_clk_p}]] \
  -group [get_clocks -quiet -of_objects \
    [get_pins -quiet -hierarchical -filter {NAME =~ "*/axi_jtag/inst/u_jtag_proc/tck_i_reg/Q"}]]

# False paths
set_false_path -quiet -from [get_pins -quiet \
  {x_heep_system_i/core_v_mini_mcu_i/debug_subsystem_i/dm_obi_top_i/i_dm_top/i_dm_csrs/dmcontrol_q_reg[ndmreset]/C}]
set_false_path -quiet -from [get_pins -quiet \
  {x_heep_system_i/rstgen_i/synch_regs_q_reg[3]/C}]
set_false_path -quiet -hold -through [get_pins -quiet \
  {x_heep_system_i/core_v_mini_mcu_i/debug_subsystem_i/dmi_jtag_i/i_dmi_cdc/i_cdc_resp/i_src/async*}]
set_false_path -quiet -hold -through [get_pins -quiet \
  {x_heep_system_i/core_v_mini_mcu_i/debug_subsystem_i/dmi_jtag_i/i_dmi_cdc/i_cdc_req/i_src/async*}]
set_false_path -quiet -hold -from [get_pins -quiet \
  {xilinx_ps_wizard_wrapper_i/xilinx_ps_wizard_i/axi_gpio/U0/gpio_core_1/Dual.gpio_Data_Out_reg[1]/C}]
