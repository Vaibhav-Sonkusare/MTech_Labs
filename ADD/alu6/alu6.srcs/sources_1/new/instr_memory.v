`timescale 1ns / 1ps
// =============================================================
//  Instruction Memory
//
//  Program: iterative sum  :-  sum = 1 + 2 + ... + N
//
//    C equivalent:
//      // n pre-stored in mem[0];  mem[255] = 1 (constant)
//      int sum_n(int n) {
//          int sum = 0;
//          while (n != 0) {
//              sum += n;
//              n--;
//          }
//          mem[0] = sum;
//      }
//
//  Opcodes used - ADD, SUB, CMP, LOAD, STOR, BNE, JUMP (7 total):
//    LOAD  0x7  -  Rd  = mem[Rs]          register-indirect
//    STOR  0x8  -  mem[Rd] = Rs           register-indirect
//    ADD   0x0  -  Rd  = Rd + Rs
//    SUB   0x1  -  Rd  = Rd - Rs
//    CMP   0x5  -  flags ← Rd - Rs        no writeback
//    BNE   0xB  -  if Z==0: PC ← target
//    JUMP  0x9  -  PC ← target            unconditional (used as halt)
//
//  Register assignments (all 0 at reset, except R7 = 0xFF):
//    R0  - permanent zero / address 0  (never written, used as base addr)
//    R1  - accumulator  (sum),  init 0 at reset
//    R2  - counter      (n),    loaded from mem[0]
//    R3  - constant 1,          loaded from mem[0xFF] via R7
//    R7  - SP = 0xFF at reset, used once to load mem[255] = 1 into R3
//
//  Data memory layout (set by testbench before reset release):
//    mem[0x00] = N    e.g. N=5  →  result = 15
//    mem[0xFF] = 1    constant for decrement (SUB R2, R3)
//
//  Instruction encoding (16-bit):
//    [15:12] opcode | [11:9] rd | [8:6] rs | [5:0] imm6 (0 for reg-reg)
//  CF-format (JUMP, BNE):
//    [15:12] opcode | [11:8] 0000 | [7:0] 8-bit target
//
//  Assembly listing:
//
//  PC  Hex     Mnemonic          Notes
//  --  ------  ----------------  -----------------------------------------
//   0  7400    LOAD R2, [R0]     counter = mem[0] = N
//   1  77C0    LOAD R3, [R7]     const1  = mem[255] = 1
//   2  5400    CMP  R2, R0       pre-loop: flags ← counter - 0
//   3  B006    BNE  #6           if N≠0 → loop body
//   4  8040    STOR R1, [R0]     N=0 on entry: store sum=0 → mem[0]
//   5  9005    JUMP #5           halt
//   6  0280    ADD  R1, R2       sum  += counter         ← loop body
//   7  14C0    SUB  R2, R3       counter -= 1
//   8  5400    CMP  R2, R0       flags ← counter - 0
//   9  B006    BNE  #6           if counter≠0 → loop again
//  10  8040    STOR R1, [R0]     store final sum → mem[0]
//  11  900B    JUMP #11          halt
//
//  Encoding:
//    LOAD R2,[R0]: 0111 010 000 000000 = 0x7400
//    LOAD R3,[R7]: 0111 011 111 000000 = 0x77C0
//    CMP  R2, R0:  0101 010 000 000000 = 0x5400
//    BNE  #6:      1011 0000 0000 0110 = 0xB006
//    STOR R1,[R0]: 1000 000 001 000000 = 0x8040
//    JUMP #5:      1001 0000 0000 0101 = 0x9005
//    ADD  R1, R2:  0000 001 010 000000 = 0x0280
//    SUB  R2, R3:  0001 010 011 000000 = 0x14C0
//    JUMP #11:     1001 0000 0000 1011 = 0x900B
// =============================================================

module instr_memory (
    input      [7:0]  addr,
    output reg [15:0] instr
);

always @(*) begin
    case (addr)
        // ---- Initialisation ----
        8'd0:  instr = 16'h7400; // LOAD R2, [R0]   counter = mem[0] = N
        8'd1:  instr = 16'h77C0; // LOAD R3, [R7]   const1  = mem[255] = 1
        // ---- Pre-loop guard ----
        8'd2:  instr = 16'h5400; // CMP  R2, R0     flags ← counter - 0
        8'd3:  instr = 16'hB006; // BNE  #6         if N≠0 → loop body
        8'd4:  instr = 16'h8040; // STOR R1, [R0]   sum=0 → mem[0]
        8'd5:  instr = 16'h9005; // JUMP #5         halt
        // ---- Loop body ----
        8'd6:  instr = 16'h0280; // ADD  R1, R2     sum += counter
        8'd7:  instr = 16'h14C0; // SUB  R2, R3     counter -= 1
        // ---- Loop back-edge ----
        8'd8:  instr = 16'h5400; // CMP  R2, R0     flags ← counter - 0
        8'd9:  instr = 16'hB006; // BNE  #6         if counter≠0 → loop
        // ---- Exit ----
        8'd10: instr = 16'h8040; // STOR R1, [R0]   sum → mem[0]
        8'd11: instr = 16'h900B; // JUMP #11        halt
        default: instr = 16'h0000;
    endcase
end

endmodule