// Copyright 2022 EPFL
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

module pad_ring (
% for pad in pad_list:
${pad.interface}
% endfor
    input logic [core_v_mini_mcu_pkg::NUM_PAD-1:0][15:0] pad_attributes_i
);

% for pad in pad_list:
${pad.instance}
% endfor


endmodule  // pad_ring