// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: xbar_varlat_n_to_one.sv
// Author: Michele Caon
// Date: 18/05/2023
// Description: N-to-1 crossbar

module xbar_varlat_n_to_one #(
    parameter int unsigned XBAR_NMASTER    = 2,
    parameter bit          USE_OUTSTANDING = 1'b0,
    parameter int unsigned MAX_OUTSTANDING = 4,
    parameter type         obi_req_t       = logic,
    parameter type         obi_rsp_t       = logic
) (
    input logic clk_i,
    input logic rst_ni,

    // Master ports
    input  obi_req_t [XBAR_NMASTER-1:0] master_req_i,
    output obi_rsp_t [XBAR_NMASTER-1:0] master_resp_o,

    // Slave port
    output obi_req_t slave_req_o,
    input  obi_rsp_t slave_resp_i
);
  // ARCHITECTURE
  // ------------
  //              MASTER[0] ----.
  //            MASTER[...] --- XBAR <--> SLAVE
  // MASTER[XBAR_NMASTER-1] ----'

  // PARAMETERS
  // ----------
  // Request width: we + be[3:0] + addr[31:0] + wdata[31:0]
  localparam int unsigned ReqDataWidth = 32'd1 + 32'd4 + 32'd32 + 32'd32;
  // Response width: rdata[31:0]
  localparam int unsigned RspDataWidth = 32'd32;

  // INTERNAL SIGNALS
  // ----------------
  // Crossbar input and output data
  logic [XBAR_NMASTER-1:0]                   master_xbar_req_req;
  logic [XBAR_NMASTER-1:0]                   xbar_master_rsp_gnt;
  logic [XBAR_NMASTER-1:0]                   xbar_master_rsp_rvalid;
  logic [XBAR_NMASTER-1:0][ReqDataWidth-1:0] master_xbar_req_data;
  logic [XBAR_NMASTER-1:0][RspDataWidth-1:0] xbar_master_rsp_data;
  logic [             0:0]                   xbar_slave_req_req;
  logic [             0:0]                   slave_xbar_rsp_gnt;
  logic [             0:0]                   slave_xbar_rsp_rvalid;
  logic [             0:0][ReqDataWidth-1:0] xbar_slave_req_data;
  logic [             0:0][RspDataWidth-1:0] slave_xbar_rsp_data;
  logic                                      xbar_slave_req_req_out;

  // --------
  // CROSSBAR
  // --------
  // Unroll OBI master signals
  generate
    for (genvar i = 0; unsigned'(i) < XBAR_NMASTER; i++) begin : gen_master_unroll
      assign master_xbar_req_req[i] = master_req_i[i].req;
      assign master_xbar_req_data[i] = {
        master_req_i[i].we, master_req_i[i].be, master_req_i[i].addr, master_req_i[i].wdata
      };
      assign master_resp_o[i] = '{
              gnt: xbar_master_rsp_gnt[i],
              rvalid: xbar_master_rsp_rvalid[i],
              rdata: xbar_master_rsp_data[i]
          };
    end
  endgenerate

  // Unroll OBI slave signals
  assign slave_req_o.req = xbar_slave_req_req_out;
  assign {
    slave_req_o.we,
    slave_req_o.be,
    slave_req_o.addr,
    slave_req_o.wdata
  } = xbar_slave_req_data[0];
  assign {slave_xbar_rsp_gnt[0], slave_xbar_rsp_rvalid[0], slave_xbar_rsp_data[0]} = slave_resp_i;

  generate
    if (USE_OUTSTANDING) begin : gen_outstanding_xbar
      assign xbar_slave_req_req_out = xbar_slave_req_req[0];

      xbar_varlat_outstanding #(
          .AggregateGnt(32'd0),  // all the masters from X-HEEP do not aggregate multiple masters
          .NumIn(XBAR_NMASTER),
          .NumOut(32'd1),
          .ReqDataWidth(ReqDataWidth),
          .RespDataWidth(RspDataWidth),
          .ExtPrio(1'b0),  // do not use external arbiter priority
          .MaxOutstanding(MAX_OUTSTANDING)
      ) u_xbar_varlat (
          .clk_i  (clk_i),
          .rst_ni (rst_ni),
          .rr_i   ('0),
          .req_i  (master_xbar_req_req),
          .add_i  ('0),
          .wdata_i(master_xbar_req_data),
          .gnt_o  (xbar_master_rsp_gnt),
          .vld_o  (xbar_master_rsp_rvalid),
          .rdata_o(xbar_master_rsp_data),
          .gnt_i  (slave_xbar_rsp_gnt),
          .req_o  (xbar_slave_req_req),
          .vld_i  (slave_xbar_rsp_rvalid),
          .wdata_o(xbar_slave_req_data),
          .rdata_i(slave_xbar_rsp_data)
      );
    end else begin : gen_legacy_xbar
      logic slave_xbar_rsp_gnt_in;

      xbar_varlat #(
          .AggregateGnt(32'd0),  // all the masters from X-HEEP do not aggregate multiple masters
          .NumIn(XBAR_NMASTER),
          .NumOut(32'd1),
          .ReqDataWidth(ReqDataWidth),
          .RespDataWidth(RspDataWidth),
          .ExtPrio(1'b0)  // do not use external arbiter priority
      ) u_xbar_varlat (
          .clk_i  (clk_i),
          .rst_ni (rst_ni),
          .rr_i   ('0),
          .req_i  (master_xbar_req_req),
          .add_i  ('0),
          .wdata_i(master_xbar_req_data),
          .gnt_o  (xbar_master_rsp_gnt),
          .vld_o  (xbar_master_rsp_rvalid),
          .rdata_o(xbar_master_rsp_data),
          .gnt_i  (slave_xbar_rsp_gnt_in),
          .req_o  (xbar_slave_req_req),
          .vld_i  (slave_xbar_rsp_rvalid),
          .wdata_o(xbar_slave_req_data),
          .rdata_i(slave_xbar_rsp_data)
      );

      // Block outstanding transactions for the legacy bus.
      typedef enum logic {
        OUTSTANDING_REQ,
        OUTSTANDING_WAITFOR_VALID
      } outstanding_req_fsm_e;

      outstanding_req_fsm_e state_n, state_q;

      always_comb begin
        state_n = state_q;
        xbar_slave_req_req_out = xbar_slave_req_req[0];
        slave_xbar_rsp_gnt_in = slave_xbar_rsp_gnt[0];

        unique case (state_q)
          OUTSTANDING_REQ: begin
            if (xbar_slave_req_req_out && slave_xbar_rsp_gnt_in) begin
              state_n = OUTSTANDING_WAITFOR_VALID;
            end
          end

          OUTSTANDING_WAITFOR_VALID: begin
            xbar_slave_req_req_out = 1'b0;
            slave_xbar_rsp_gnt_in  = 1'b0;
            if (slave_xbar_rsp_rvalid[0]) begin
              xbar_slave_req_req_out = xbar_slave_req_req[0];
              slave_xbar_rsp_gnt_in  = slave_xbar_rsp_gnt[0];
              if (xbar_slave_req_req_out && slave_xbar_rsp_gnt_in) begin
                state_n = OUTSTANDING_WAITFOR_VALID;
              end else begin
                state_n = OUTSTANDING_REQ;
              end
            end
          end

          default: begin
            state_n = OUTSTANDING_REQ;
          end
        endcase
      end

      always_ff @(posedge clk_i or negedge rst_ni) begin
        if (~rst_ni) begin
          state_q <= OUTSTANDING_REQ;
        end else begin
          state_q <= state_n;
        end
      end
    end
  endgenerate

`ifndef SYNTHESIS
  // pragma translate_off
  initial begin
    assert (MAX_OUTSTANDING > 0)
    else $fatal(1, "MAX_OUTSTANDING must be greater than 0.");
  end
  // pragma translate_on
`endif

endmodule
