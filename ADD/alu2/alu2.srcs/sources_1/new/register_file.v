`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 07:15:14 PM
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


module register_file(
    input clk,
    input rst,
    input we,
    input [1:0] raddr1,
    input [1:0] raddr2,
    input [1:0] waddr,
    input [7:0] write_data,
    output [7:0] read_data1,
    output [7:0] read_data2
);

reg [7:0] register [3:0];
//integer i;

// Write + Reset
always @(posedge clk) begin
    if (rst) begin
        register[0] <= 8'h10; // R0
        register[1] <= 8'h5;  // R1
        register[2] <= 8'h0;
        register[3] <= 8'h0;
    end
    else if (we) begin
        register[waddr] <= write_data;
    end
end

// Read
assign read_data1 = register[raddr1];
assign read_data2 = register[raddr2];

endmodule