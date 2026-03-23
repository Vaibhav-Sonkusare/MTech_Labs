`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/18/2026 01:09:00 PM
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
        3'b000: result = a + b;     
        3'b001: result = (a < b)? -8'sd1 : ((a == b)? 8'd0 : 8'd1);
        3'b010: result = (a == 0)? 8'd0 : 8'd1;
        3'b011: result = a - b;
        default: result = 8'd0;
    endcase
end

assign zero = (result == 0);

endmodule