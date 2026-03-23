`timescale 1ns / 1ps
// =============================================================
//  ALU — Arithmetic & Logic Unit
//  Opcode map (alu_op[2:0]):
//    000 = ADD   rd = a + b
//    001 = SUB   rd = a - b
//    010 = MOV   rd = b
//    011 = SUBI  rd = a - b  (b is sign-extended immediate)
//    100 = CMP   flags only  (no writeback — enforced by control)
//    101 = ADDI  rd = a + b  (b is sign-extended immediate)
//    110 = AND   rd = a & b
//    111 = OR    rd = a | b
//
//  Outputs:
//    result   — 8-bit ALU output (written to register only when reg_we=1)
//    zero     — 1 when result == 0
//    negative — 1 when result[7] == 1  (two's-complement sign bit)
// =============================================================
module alu (
    input      [7:0] a,
    input      [7:0] b,
    input      [2:0] alu_op,
    output reg [7:0] result,
    output           zero,
    output           negative
);

always @(*) begin
    case (alu_op)
        3'b000: result = a + b;        // ADD
        3'b001: result = a - b;        // SUB
        3'b010: result = b;            // MOV
        3'b011: result = a - b;        // SUBI  (imm as b)
        3'b100: result = a - b;        // CMP   (flags only, control blocks writeback)
        3'b101: result = a + b;        // ADDI  (imm as b)
        3'b110: result = a & b;        // AND
        3'b111: result = a | b;        // OR
        default: result = 8'd0;
    endcase
end

assign zero     = (result == 8'd0);
assign negative =  result[7];          // MSB = sign in two's complement

endmodule