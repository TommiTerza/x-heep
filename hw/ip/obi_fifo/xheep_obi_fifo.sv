// Copyright 2022 EPFL
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

module xheep_obi_fifo #(
    parameter  int unsigned FIFO_DEPTH          = 1,
    // OBI data types
    parameter  type         obi_req_t           = xheep_obi_pkg::xheep_obi_req_t,
    parameter  type         obi_rsp_t           = xheep_obi_pkg::xheep_obi_rsp_t,
    localparam int unsigned CountWidth          = FIFO_DEPTH > 1 ? $clog2(FIFO_DEPTH + 1) : 1,
    localparam bit          ResponseFallThrough = FIFO_DEPTH > 1
) (
    input logic clk_i,
    input logic rst_ni,

    input  obi_req_t producer_req_i,
    output obi_rsp_t producer_resp_o,

    output obi_req_t consumer_req_o,
    input  obi_rsp_t consumer_resp_i
);

  typedef struct packed {
    logic        we;
    logic [3:0]  be;
    logic [31:0] addr;
    logic [31:0] wdata;
  } obi_data_req_t;

  obi_data_req_t producer_data_req, consumer_data_req;

  logic fifo_req_full, fifo_req_empty, fifo_req_push, fifo_req_pop;
  logic fifo_resp_full, fifo_resp_empty, fifo_resp_push, fifo_resp_pop;

  localparam logic [CountWidth-1:0] FifoDepthCount = CountWidth'(FIFO_DEPTH);

  logic [CountWidth-1:0] pending_count_d, pending_count_q;
  logic pending_full;

  assign {producer_data_req.we, producer_data_req.be, producer_data_req.addr, producer_data_req.wdata} =
          {
    producer_req_i.we, producer_req_i.be, producer_req_i.addr, producer_req_i.wdata
  };

  assign pending_full = pending_count_q == FifoDepthCount;

  assign producer_resp_o.gnt = !pending_full && !fifo_req_full;
  assign fifo_req_push = producer_req_i.req && producer_resp_o.gnt;

  assign consumer_req_o.req = !fifo_req_empty;
  assign {consumer_req_o.we, consumer_req_o.be, consumer_req_o.addr, consumer_req_o.wdata} = {
    consumer_data_req.we, consumer_data_req.be, consumer_data_req.addr, consumer_data_req.wdata
  };
  assign fifo_req_pop = consumer_req_o.req && consumer_resp_i.gnt;

  assign fifo_resp_push = consumer_resp_i.rvalid && !fifo_resp_full;
  assign fifo_resp_pop = !fifo_resp_empty;
  assign producer_resp_o.rvalid = fifo_resp_pop;

  always_comb begin
    unique case ({
      fifo_req_push, fifo_resp_pop
    })
      2'b00: pending_count_d = pending_count_q;
      2'b01: pending_count_d = pending_count_q - 1'b1;
      2'b10: pending_count_d = pending_count_q + 1'b1;
      2'b11: pending_count_d = pending_count_q;
    endcase
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      pending_count_q <= '0;
    end else begin
      pending_count_q <= pending_count_d;
    end
  end

  fifo_v3 #(
      .DEPTH(FIFO_DEPTH),
      .dtype(obi_data_req_t)
  ) obi_req_fifo_i (
      .clk_i,
      .rst_ni,
      .flush_i(1'b0),
      .testmode_i(1'b0),
      .full_o(fifo_req_full),
      .empty_o(fifo_req_empty),
      .usage_o(),
      .data_i(producer_data_req),
      .push_i(fifo_req_push),
      .data_o(consumer_data_req),
      .pop_i(fifo_req_pop)
  );

  fifo_v3 #(
      .FALL_THROUGH(ResponseFallThrough),
      .DEPTH(FIFO_DEPTH),
      .dtype(logic [31:0])
  ) obi_resp_fifo_i (
      .clk_i,
      .rst_ni,
      .flush_i(1'b0),
      .testmode_i(1'b0),
      .full_o(fifo_resp_full),
      .empty_o(fifo_resp_empty),
      .usage_o(),
      .data_i(consumer_resp_i.rdata),
      .push_i(fifo_resp_push),
      // grant is given above
      .data_o(producer_resp_o.rdata),
      .pop_i(fifo_resp_pop)
  );

`ifndef SYNTHESIS
  initial begin
    assert (FIFO_DEPTH > 0)
    else $fatal(1, "FIFO_DEPTH must be greater than 0.");
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : check_fifo_protocol
    if (rst_ni) begin
      if (consumer_resp_i.rvalid && (pending_count_q == '0)) begin
        $error("Received an OBI FIFO response without a pending transaction.");
      end
      if (consumer_resp_i.rvalid && fifo_resp_full) begin
        $error("OBI FIFO response buffer overflow.");
      end
    end
  end
`endif

endmodule
