`timescale 1ns / 1ps
// =============================================================
//  Register File  —  8 x 8-bit registers
//
//  Register conventions:
//    R0 — argument / return value  (fib result lives here)
//    R1 — scratch / second arg
//    R2 — scratch / third arg
//    R3 — general purpose
//    R4 — general purpose
//    R5 — general purpose
//    R6 — link register (LR) — CALL saves return address here
//    R7 — stack pointer  (SP) — initialised to top of data memory
//
//  The link register + SP are general-purpose from the register
//  file's perspective; the control unit assigns them special roles.
//
//  Two independent async read ports, one synchronous write port.
//  Writes are suppressed when we=0.
// =============================================================
module register_file (
    input            clk,
    input            rst,
    input            we,
    input      [2:0] raddr1,      // read port 1
    input      [2:0] raddr2,      // read port 2
    input      [2:0] waddr,       // write port
    input      [7:0] write_data,
    output     [7:0] read_data1,
    output     [7:0] read_data2
);

reg [7:0] regs [7:0];

// Reset initialises the calling convention state.
// Everything else starts at 0; SP starts at 0xFF (top of 256-byte memory).
integer i;
always @(posedge clk) begin
    if (rst) begin
        for (i = 0; i < 6; i = i + 1)
            regs[i] <= 8'd0;
        regs[6] <= 8'd0;    // LR = 0
        regs[7] <= 8'hFF;   // SP = 255
    end else if (we) begin
        regs[waddr] <= write_data;
    end
end

// Asynchronous read — reflects latest value including same-cycle write
assign read_data1 = regs[raddr1];
assign read_data2 = regs[raddr2];

endmodule