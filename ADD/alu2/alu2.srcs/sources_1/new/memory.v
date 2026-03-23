`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 07:47:38 PM
// Design Name: 
// Module Name: memory
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module memory(
    input clk,
    input rst,
    input we,
    input [2:0] addr,
    input [7:0] write_data,
    output [7:0] read_data
);

reg [7:0] mem [7:0];
//integer i;

// Write + Reset
always @(posedge clk) begin
    if (rst) begin
        mem[0] <= 8'h0;
        mem[1] <= 8'h0;
        mem[2] <= 8'h0;
        mem[3] <= 8'h50; // useful test value
        mem[4] <= 8'h0;
        mem[5] <= 8'h99; // another test value
        mem[6] <= 8'h0;
        mem[7] <= 8'h0;
    end
    else if (we) begin
        mem[addr] <= write_data;
    end
end

// Read
assign read_data = mem[addr];

endmodule
