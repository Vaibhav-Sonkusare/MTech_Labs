`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 12:56:43 PM
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


module memory (
    input clk,
    input we,
    input [2:0] addr,
    input [7:0] write_data,
    output [7:0] read_data
);

reg [7:0] mem [7:0];

// Read
assign read_data = mem[addr];

// Write
always @(posedge clk) begin
    if (we)
        mem[addr] <= write_data;
end

endmodule