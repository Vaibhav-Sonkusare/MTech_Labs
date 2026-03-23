`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/18/2026 01:09:43 PM
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

// Reset + Write
always @(posedge clk) begin
    if (rst) begin
        mem[0] <= 8'd0;
        mem[1] <= 8'd0;
        mem[2] <= 8'd0;
        mem[3] <= 8'd50; // preload
        mem[4] <= 8'd0;
        mem[5] <= 8'd99; // preload
        mem[6] <= 8'd0;
        mem[7] <= 8'd0;
    end
    else if (we) begin
        mem[addr] <= write_data;
    end
end

assign read_data = mem[addr];

endmodule