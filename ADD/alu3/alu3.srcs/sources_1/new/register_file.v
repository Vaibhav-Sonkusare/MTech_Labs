`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/18/2026 01:10:13 PM
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

// Reset + Write
always @(posedge clk) begin
    if (rst) begin
        register[0] <= 8'd10; // preload
        register[1] <= 8'd5;  // preload
        register[2] <= 8'd0;
        register[3] <= 8'd0;
    end
    else if (we) begin
        register[waddr] <= write_data;
    end
end

assign read_data1 = register[raddr1];
assign read_data2 = register[raddr2];

endmodule