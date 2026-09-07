// Copyright 2026 OpenHW Group
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Variable-latency OBI crossbar with bounded outstanding transaction support.

module xbar_varlat_outstanding #(
    parameter int unsigned AggregateGnt = 1,
    parameter int unsigned NumIn = 4,
    parameter int unsigned NumOut = 4,
    parameter int unsigned ReqDataWidth = 32,
    parameter int unsigned RespDataWidth = 32,
    parameter bit ExtPrio = 1'b0,
    parameter int unsigned MaxOutstanding = 4,
    parameter int unsigned LogNumOut = NumOut > 1 ? $clog2(NumOut) : 1,
    parameter int unsigned LogNumIn = NumIn > 1 ? $clog2(NumIn) : 1,
    localparam int unsigned SourceFifoDepth = NumIn * MaxOutstanding
) (
    input logic clk_i,
    input logic rst_ni,

    // External priority flag input.
    input logic [NumOut-1:0][LogNumIn-1:0] rr_i,

    // Master side.
    input  logic [NumIn-1:0]                    req_i,
    input  logic [NumIn-1:0][    LogNumOut-1:0] add_i,
    input  logic [NumIn-1:0][ ReqDataWidth-1:0] wdata_i,
    output logic [NumIn-1:0]                    gnt_o,
    output logic [NumIn-1:0]                    vld_o,
    output logic [NumIn-1:0][RespDataWidth-1:0] rdata_o,

    // Slave side.
    input  logic [NumOut-1:0]                    gnt_i,
    output logic [NumOut-1:0]                    req_o,
    input  logic [NumOut-1:0]                    vld_i,
    output logic [NumOut-1:0][ ReqDataWidth-1:0] wdata_o,
    input  logic [NumOut-1:0][RespDataWidth-1:0] rdata_i
);

  typedef logic [LogNumOut-1:0] out_id_t;
  typedef logic [LogNumIn-1:0] in_id_t;
  typedef logic [ReqDataWidth-1:0] req_data_t;
  typedef logic [RespDataWidth-1:0] rsp_data_t;

  logic [NumOut-1:0][NumIn-1:0] slave_req;
  logic [NumOut-1:0][NumIn-1:0] slave_gnt;
  req_data_t [NumOut-1:0][NumIn-1:0] slave_wdata;
  in_id_t [NumOut-1:0] slave_gnt_master;
  logic [NumOut-1:0] slave_req_accepted;

  logic [NumIn-1:0][NumOut-1:0] master_gnt_matrix;

  logic [NumIn-1:0] master_order_full;
  logic [NumIn-1:0] master_order_empty;
  logic [NumIn-1:0] master_order_push;
  logic [NumIn-1:0] master_order_pop;
  out_id_t [NumIn-1:0] master_order_target;

  logic [NumOut-1:0] slave_source_full;
  logic [NumOut-1:0] slave_source_empty;
  logic [NumOut-1:0] slave_source_push;
  logic [NumOut-1:0] slave_source_pop;
  in_id_t [NumOut-1:0] slave_source_master;

  logic [NumIn-1:0][NumOut-1:0] rsp_fifo_full;
  logic [NumIn-1:0][NumOut-1:0] rsp_fifo_empty;
  logic [NumIn-1:0][NumOut-1:0] rsp_fifo_push;
  logic [NumIn-1:0][NumOut-1:0] rsp_fifo_pop;
  rsp_data_t [NumIn-1:0][NumOut-1:0] rsp_fifo_data;

  for (genvar k = 0; unsigned'(k) < NumOut; k++) begin : gen_slave_side
    for (genvar j = 0; unsigned'(j) < NumIn; j++) begin : gen_slave_reqs
      assign slave_req[k][j] = req_i[j] &&
                               (add_i[j] == out_id_t'(k)) &&
                               !master_order_full[j] &&
                               !slave_source_full[k];
      assign slave_wdata[k][j] = wdata_i[j];
    end

    rr_arb_tree #(
        .NumIn   (NumIn),
        .DataType(req_data_t),
        .ExtPrio (ExtPrio),
        .IdxWidth(LogNumIn),
        .idx_t   (in_id_t)
    ) i_req_arb (
        .clk_i,
        .rst_ni,
        .flush_i(1'b0),
        .rr_i   (rr_i[k]),
        .req_i  (slave_req[k]),
        .gnt_o  (slave_gnt[k]),
        .data_i (slave_wdata[k]),
        .req_o  (req_o[k]),
        .gnt_i  (gnt_i[k]),
        .data_o (wdata_o[k]),
        .idx_o  (slave_gnt_master[k])
    );

    assign slave_req_accepted[k] = req_o[k] && gnt_i[k];
    assign slave_source_push[k]  = slave_req_accepted[k];
    assign slave_source_pop[k]   = vld_i[k] && !slave_source_empty[k];

    fifo_v3 #(
        .FALL_THROUGH(1'b0),
        .DEPTH       (SourceFifoDepth),
        .dtype       (in_id_t)
    ) i_source_fifo (
        .clk_i,
        .rst_ni,
        .flush_i   (1'b0),
        .testmode_i(1'b0),
        .full_o    (slave_source_full[k]),
        .empty_o   (slave_source_empty[k]),
        .usage_o   (),
        .data_i    (slave_gnt_master[k]),
        .push_i    (slave_source_push[k]),
        .data_o    (slave_source_master[k]),
        .pop_i     (slave_source_pop[k])
    );
  end

  for (genvar j = 0; unsigned'(j) < NumIn; j++) begin : gen_master_side
    for (genvar k = 0; unsigned'(k) < NumOut; k++) begin : gen_master_gnts
      assign master_gnt_matrix[j][k] = slave_gnt[k][j];
    end

    assign gnt_o[j] = AggregateGnt == 1 ? |master_gnt_matrix[j] : master_gnt_matrix[j][add_i[j]];
    assign master_order_push[j] = req_i[j] && gnt_o[j];
    assign master_order_pop[j] = vld_o[j];

    fifo_v3 #(
        .FALL_THROUGH(1'b0),
        .DEPTH       (MaxOutstanding),
        .dtype       (out_id_t)
    ) i_order_fifo (
        .clk_i,
        .rst_ni,
        .flush_i   (1'b0),
        .testmode_i(1'b0),
        .full_o    (master_order_full[j]),
        .empty_o   (master_order_empty[j]),
        .usage_o   (),
        .data_i    (add_i[j]),
        .push_i    (master_order_push[j]),
        .data_o    (master_order_target[j]),
        .pop_i     (master_order_pop[j])
    );

    assign vld_o[j]   = !master_order_empty[j] && !rsp_fifo_empty[j][master_order_target[j]];
    assign rdata_o[j] = rsp_fifo_data[j][master_order_target[j]];

    for (genvar k = 0; unsigned'(k) < NumOut; k++) begin : gen_rsp_fifos
      assign rsp_fifo_push[j][k] = vld_i[k] &&
                                   !slave_source_empty[k] &&
                                   (slave_source_master[k] == in_id_t'(j));
      assign rsp_fifo_pop[j][k] = vld_o[j] && (master_order_target[j] == out_id_t'(k));

      fifo_v3 #(
          .FALL_THROUGH(1'b1),
          .DEPTH       (MaxOutstanding),
          .dtype       (rsp_data_t)
      ) i_rsp_fifo (
          .clk_i,
          .rst_ni,
          .flush_i   (1'b0),
          .testmode_i(1'b0),
          .full_o    (rsp_fifo_full[j][k]),
          .empty_o   (rsp_fifo_empty[j][k]),
          .usage_o   (),
          .data_i    (rdata_i[k]),
          .push_i    (rsp_fifo_push[j][k]),
          .data_o    (rsp_fifo_data[j][k]),
          .pop_i     (rsp_fifo_pop[j][k])
      );
    end
  end

`ifndef SYNTHESIS
  initial begin
    assert (NumIn > 0)
    else $fatal(1, "NumIn must be greater than 0.");
    assert (NumOut > 0)
    else $fatal(1, "NumOut must be greater than 0.");
    assert (MaxOutstanding > 0)
    else $fatal(1, "MaxOutstanding must be greater than 0.");
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : check_bookkeeping_overflow
    if (rst_ni) begin
      for (int unsigned k = 0; k < NumOut; k++) begin
        if (vld_i[k] && slave_source_empty[k]) begin
          $error("Received a response on slave port %0d without a tracked request.", k);
        end
      end
      for (int unsigned j = 0; j < NumIn; j++) begin
        for (int unsigned k = 0; k < NumOut; k++) begin
          if (rsp_fifo_push[j][k] && rsp_fifo_full[j][k] && !rsp_fifo_pop[j][k]) begin
            $error("Response FIFO overflow on master %0d slave %0d.", j, k);
          end
        end
      end
    end
  end
`endif

endmodule : xbar_varlat_outstanding
