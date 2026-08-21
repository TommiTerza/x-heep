// Copyright 2026 EPFL
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

module i2s_tx_sink #(
    parameter type reg_req_t = logic,
    parameter type reg_rsp_t = logic,
    parameter int unsigned WordWidth = 32
) (
    input logic clk_i,
    input logic rst_ni,

    input  reg_req_t reg_req_i,
    output reg_rsp_t reg_rsp_o,

    input  logic i2s_sck_i,
    input  logic i2s_ws_i,
    input  logic i2s_sd_i,
    output logic i2s_mic_enable_o
);

  import i2s_tx_sink_reg_pkg::*;

  i2s_tx_sink_reg2hw_t reg2hw;
  i2s_tx_sink_hw2reg_t hw2reg;

  logic sink_en_sck;
  logic sample_valid_sck;
  logic sample_ready_sck;
  logic [WordWidth-1:0] sample_sck;
  logic overflow_sck;
  logic overflow;
  logic rx_valid;
  logic [WordWidth-1:0] rx_data;

  assign i2s_mic_enable_o = reg2hw.control.q[0];
  assign hw2reg.rxdata.d = rx_data;
  assign hw2reg.status.empty.de = 1'b1;
  assign hw2reg.status.empty.d = ~rx_valid;
  assign hw2reg.status.available.de = 1'b1;
  assign hw2reg.status.available.d = rx_valid;
  assign hw2reg.status.overflow.de = 1'b1;
  assign hw2reg.status.overflow.d = overflow;

  i2s_tx_sink_reg_top #(
      .reg_req_t(reg_req_t),
      .reg_rsp_t(reg_rsp_t)
  ) i2s_tx_sink_reg_top_i (
      .clk_i,
      .rst_ni,
      .reg_req_i,
      .reg_rsp_o,
      .reg2hw,
      .hw2reg,
      .devmode_i(1'b1)
  );

  sync #(
      .STAGES(2),
      .ResetValue(1'b0)
  ) sink_en_sync_i (
      .clk_i(i2s_sck_i),
      .rst_ni,
      .serial_i(reg2hw.control.q[1]),
      .serial_o(sink_en_sck)
  );

  i2s_tx_sink_deserializer #(
      .WordWidth(WordWidth)
  ) deserializer_i (
      .sck_i(i2s_sck_i),
      .rst_ni(rst_ni),
      .en_i(sink_en_sck),
      .ws_i(i2s_ws_i),
      .sd_i(i2s_sd_i),
      .data_o(sample_sck),
      .data_valid_o(sample_valid_sck)
  );

  cdc_fifo_gray #(
      .T(logic [WordWidth-1:0]),
      .LOG_DEPTH(2)
  ) sample_cdc_i (
      .src_clk_i  (i2s_sck_i),
      .src_rst_ni (rst_ni),
      .src_ready_o(sample_ready_sck),
      .src_data_i (sample_sck),
      .src_valid_i(sample_valid_sck),

      .dst_rst_ni (rst_ni),
      .dst_clk_i  (clk_i),
      .dst_data_o (rx_data),
      .dst_valid_o(rx_valid),
      .dst_ready_i(reg2hw.rxdata.re)
  );

  always_ff @(posedge i2s_sck_i or negedge rst_ni) begin
    if (~rst_ni) begin
      overflow_sck <= 1'b0;
    end else if (sample_valid_sck && !sample_ready_sck) begin
      overflow_sck <= 1'b1;
    end
  end

  sync #(
      .STAGES(2),
      .ResetValue(1'b0)
  ) overflow_sync_i (
      .clk_i,
      .rst_ni,
      .serial_i(overflow_sck),
      .serial_o(overflow)
  );

endmodule : i2s_tx_sink
