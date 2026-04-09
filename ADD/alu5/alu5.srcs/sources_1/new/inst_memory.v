`timescale 1ns / 1ps
// =============================================================
//  Instruction Memory - ROM
//  Stores the recursive sum_n program.
//
//  Instruction encoding (16-bit):
//    [15:12] = opcode
//    [11: 9] = rd  (destination / base register)
//    [ 8: 6] = rs  (source register)
//    [ 5: 0] = imm6 (sign-extended to 8 bits by control)
//
//  CF-format (JUMP/BEQ/BNE/BLT/BGE/CALL/RET):
//    [15:12] = opcode
//    [11: 8] = 0
//    [ 7: 0] = 8-bit branch/call/jump target
//
// =============================================================
//
//  Function being implemented:
//
//    int sum_n(int a) {
//        if (a <= 0) return 0;
//        return a + sum_n(a - 1);
//    }
//
//  Register convention:
//    R0  - argument on entry / return value on exit
//    R1  - scratch (holds saved 'a' after pop)
//    R6  - Link Register (LR), saved/restored across calls
//    R7  - Stack Pointer (SP), initialised to 0xFF by register_file
//
//  Stack layout per call frame (grows downward):
//    SP+1 → saved LR   (pushed first, so at higher address)
//    SP+0 → saved a    (pushed second, so at current SP)
//
//  Assembly listing:
//
//  PC  Hex     Mnemonic               Explanation
//  --- ------  ---------------------  ----------------------------------
//   0  6000    CMPI  R0, #0           a - 0  →  set Z and N flags
//   1  A00F    BEQ   #15              if a == 0  →  return_zero
//   2  C00F    BLT   #15              if a  < 0  →  return_zero
//   3  8F80    STOR  R6, [R7]         mem[SP] = LR          (push LR)
//   4  4E01    SUBI  R7, #1           SP--
//   5  8E00    STOR  R0, [R7]         mem[SP] = a           (push a)
//   6  4E01    SUBI  R7, #1           SP--
//   7  4001    SUBI  R0, #1           a = a - 1
//   8  E000    CALL  #0               call sum_n(a-1); LR←PC+1=9
//   9  3E01    ADDI  R7, #1           SP++
//  10  73C0    LOAD  R1, [R7]         R1 = mem[SP]  (pop saved a)
//  11  3E01    ADDI  R7, #1           SP++
//  12  7DC0    LOAD  R6, [R7]         R6 = mem[SP]  (pop saved LR)
//  13  0040    ADD   R0,  R1          R0 = sum_n(a-1) + a
//  14  F180    RET                    PC ← LR  (R6)
//  15  1000    SUB   R0,  R0          R0 = 0   (a <= 0 base case)
//  16  F180    RET                    PC ← LR  (R6)
// =============================================================

module instr_memory (
    input      [7:0]  addr,
    output reg [15:0] instr
);

always @(*) begin
    case (addr)
        //                         opcode  rd   rs   imm6
        8'd0:  instr = 16'h6000; // CMPI   R0   --   #0
        8'd1:  instr = 16'hA00F; // BEQ    --   --   target=15
        8'd2:  instr = 16'hC00F; // BLT    --   --   target=15
        8'd3:  instr = 16'h8F80; // STOR   R7   R6   --        (push LR)
        8'd4:  instr = 16'h4E01; // SUBI   R7   --   #1
        8'd5:  instr = 16'h8E00; // STOR   R7   R0   --        (push a)
        8'd6:  instr = 16'h4E01; // SUBI   R7   --   #1
        8'd7:  instr = 16'h4001; // SUBI   R0   --   #1        (a = a-1)
        8'd8:  instr = 16'hE000; // CALL   --   --   target=0
        8'd9:  instr = 16'h3E01; // ADDI   R7   --   #1
        8'd10: instr = 16'h73C0; // LOAD   R1   R7   --        (pop saved a)
        8'd11: instr = 16'h3E01; // ADDI   R7   --   #1
        8'd12: instr = 16'h7DC0; // LOAD   R6   R7   --        (pop saved LR)
        8'd13: instr = 16'h0040; // ADD    R0   R1   --        (result + saved a)
        8'd14: instr = 16'hF180; // RET    --   R6   --
        8'd15: instr = 16'h1000; // SUB    R0   R0   --        (R0 = 0)
        8'd16: instr = 16'hF180; // RET    --   R6   --
        default: instr = 16'h0000;
    endcase
end

endmodule