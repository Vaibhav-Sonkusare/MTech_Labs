`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 12:55:55 PM
// Design Name: 
// Module Name: register_file
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


module register_file (
    input clk,
    input we,                    // write enable
    input [1:0] read_addr1,
    input [1:0] read_addr2,
    input [1:0] write_addr,
    input [7:0] write_data,
    output [7:0] read_data1,
    output [7:0] read_data2
);

reg [7:0] registers [3:0];

// Read (combinational)
assign read_data1 = registers[read_addr1];
assign read_data2 = registers[read_addr2];

// Write (sequential)
always @(posedge clk) begin
    if (we)
        registers[write_addr] <= write_data;
end

endmodule