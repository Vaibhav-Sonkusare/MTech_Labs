`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 12:54:57 PM
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


module alu (
    input [7:0] a,
    input [7:0] b,
    input [2:0] alu_op,
    output reg [7:0] result,
    output zero
);

always @(*) begin
    case (alu_op)
        3'b000: result = a + b;       // ADD
        3'b001: result = a - b;       // SUB
        3'b010: result = a & b;       // AND
        3'b011: result = a | b;       // OR
        3'b100: result = (a < b);     // SLT
        default: result = 8'b0;
    endcase
end

assign zero = (result == 0);

endmodule