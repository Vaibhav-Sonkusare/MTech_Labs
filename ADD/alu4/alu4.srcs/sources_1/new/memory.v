`timescale 1ns / 1ps
// =============================================================
//  Data Memory  —  256 x 8-bit synchronous SRAM
//
//  Used as both data store and call stack.
//  Stack grows downward: PUSH decrements addr before write,
//  POP reads then increments addr (managed by control unit).
//
//  Ports:
//    addr       — 8-bit byte address (comes from SP register or imm)
//    write_data — data to store
//    read_data  — async read; always reflects mem[addr]
//    we         — synchronous write enable
// =============================================================
module memory (
    input            clk,
    input            rst,
    input            we,
    input      [7:0] addr,
    input      [7:0] write_data,
    output     [7:0] read_data
);

reg [7:0] mem [255:0];  // 2^8

integer i;
always @(posedge clk) begin
    if (rst) begin
        for (i = 0; i < 256; i = i + 1)
            mem[i] <= 8'd0;
    end else if (we) begin
        mem[addr] <= write_data;
    end
end

// Asynchronous read so LOAD result is available the same cycle
assign read_data = mem[addr];

endmodule