`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 07:47:23 PM
// Design Name: 
// Module Name: alu
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


module alu(
    input [7:0] a,
    input [7:0] b,
    input [2:0] alu_op,
    output reg [7:0] result,
    output zero
);

always @(*) begin
    case (alu_op)
        3'b000: result = a + b;     // ADD
//        3'b001: result = a - b;     // SUB
        3'b001: result = (a < b)? -8'sd1: ((a == b)? 0: 8'd1);    // COMPARATOR
        3'b010: result = (a == 0)? 8'd0: 8'd1;     // BEQZ
        default: result = 8'd0;     // Default 0
    endcase
end

assign zero = (result == 0);

endmodule 