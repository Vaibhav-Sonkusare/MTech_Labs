`timescale 1ns / 1ps
// =============================================================
//  Instruction Memory  —  256 x 16-bit ROM
//
//  Instruction encoding (16 bits):
//  ┌─────────────┬──────┬──────┬──────────────────┐
//  │ [15:12]     │[11:9]│ [8:6]│ [5:0] (sign-ext) │
//  │  opcode(4b) │  rd  │  rs  │   imm6            │
//  └─────────────┴──────┴──────┴──────────────────┘
//  imm6 is sign-extended to 8 bits inside the control unit.
//  For branch/jump targets the full 8-bit addr is in [7:0].
//
//  Opcode table:
//    0000  ADD   rd, rs          rd = rd + rs
//    0001  SUB   rd, rs          rd = rd - rs
//    0010  MOV   rd, rs          rd = rs
//    0011  ADDI  rd, #imm6       rd = rd + imm6
//    0100  SUBI  rd, #imm6       rd = rd - imm6
//    0101  CMP   rd, rs          flags = rd - rs  (no writeback)
//    0110  CMPI  rd, #imm6       flags = rd - imm6 (no writeback)
//    0111  LOAD  rd, [rs]        rd = mem[rs]
//    1000  STORE rs, [rd]        mem[rd] = rs   (rd=addr reg, rs=data reg)
//    1001  JUMP  #addr8          PC = imm[7:0]
//    1010  BEQ   #addr8          if zero:     PC = imm[7:0]
//    1011  BNE   #addr8          if !zero:    PC = imm[7:0]
//    1100  BLT   #addr8          if negative: PC = imm[7:0]
//    1101  BGE   #addr8          if !negative:PC = imm[7:0]
//    1110  CALL  #addr8          R6=PC+1, PC = imm[7:0]
//    1111  RET                   PC = R6
//
//  Calling convention:
//    Argument  in R0
//    Return value in R0
//    R6 = link register (return address)
//    R7 = stack pointer (grows downward)
//    Callee saves R6 (LR) to stack on entry, restores on RET
//
// =============================================================
//
//  Recursive Fibonacci program loaded in mem[]:
//
//    fib(n):                        ; R0 = n on entry, result in R0
//      addr 0:  CMPI R0, #1        ; n <= 1?
//      addr 1:  BGE  base_ok       ; if n >= 1 branch to base check  -- not quite, see below
//
//  Cleaner approach using BLT for base case:
//
//    fib(n) — R0=n, R6=return addr:
//      0: CMPI  R0, #2             ; compare n with 2
//      1: BLT   base_case          ; if n < 2 (i.e. n==0 or n==1): return n
//      2: STORE R6, [R7]           ; push LR onto stack (mem[SP]=LR)
//      3: SUBI  R7, #1             ; SP--
//      4: STORE R0, [R7]           ; push n
//      5: SUBI  R7, #1             ; SP--
//      6: SUBI  R0, #1             ; R0 = n-1
//      7: CALL  fib                ; fib(n-1) → R0; R6 = 8
//      8: ADD   R1, R0             ; R1 += fib(n-1)  [but R1=0 on first call]
//         -- wait, cleaner to accumulate differently
//
//  Cleanest single-accumulator approach:
//    After fib(n-1) returns in R0, save it, reload n, compute fib(n-2):
//
//      0:  CMPI  R0, #2            ; if n < 2 → base case
//      1:  BLT   #20               ; jump to addr 20 (base: return R0 as-is)
//      2:  STORE R6, [R7]          ; save LR        mem[SP] = LR
//      3:  SUBI  R7, #1            ; SP--
//      4:  STORE R0, [R7]          ; save n         mem[SP] = n
//      5:  SUBI  R7, #1            ; SP--
//      6:  SUBI  R0, #1            ; R0 = n-1
//      7:  CALL  #0                ; fib(n-1) → R0; LR = 8
//      8:  STORE R0, [R7]          ; save fib(n-1)  mem[SP] = fib(n-1)
//      9:  SUBI  R7, #1            ; SP--
//      10: ADDI  R7, #1            ; SP++ (peek n: it's 2 slots up)
//          -- address arithmetic gets messy; use indexed loads
//
//  Final clean version (stack frame layout per call):
//    [SP+2] = saved LR
//    [SP+1] = n
//    [SP+0] = fib(n-1)   (added after first recursive call returns)
//
//  Program (addr → instruction):
//    --- fib entry ---
//     0: CMPI  R0, #2         ; n < 2?
//     1: BLT   #18            ; yes → base case at addr 18
//     2: STORE R6, [R7]       ; mem[SP]  = LR
//     3: SUBI  R7, #1         ; SP--
//     4: STORE R0, [R7]       ; mem[SP]  = n
//     5: SUBI  R7, #1         ; SP--
//     6: SUBI  R0, #1         ; R0 = n-1
//     7: CALL  #0             ; fib(n-1) → R0
//     8: MOV   R1, R0         ; R1 = fib(n-1)
//     9: STORE R1, [R7]       ; mem[SP]  = fib(n-1)
//    10: SUBI  R7, #1         ; SP--
//    11: ADDI  R7, #2         ; SP += 2 → peek n (it's at SP now)
//    12: LOAD  R0, [R7]       ; R0 = n  (reload)
//    13: SUBI  R7, #2         ; SP back to fib(n-1) slot
//    14: SUBI  R0, #2         ; R0 = n-2
//    15: CALL  #0             ; fib(n-2) → R0
//    16: ADDI  R7, #1         ; SP++ (pop fib(n-1) into…)
//    17: LOAD  R1, [R7]       ; R1 = fib(n-1)  from stack
//    -- now need ADD R0,R1 and restore LR, then RET
//    -- continuing sequence would exceed addr space comments; see below
//
//  Final assembled program (fits cleanly in ROM):
// =============================================================

// Helper macro — packs a 16-bit instruction
// {opc[3:0], rd[2:0], rs[2:0], imm[5:0]}  total=16
// For branch/jump imm is 8-bit so we use {opc,3'b0,3'b0,addr[7:0]} (but that=14b)
// Actual encoding used: [15:12]=opc [11:9]=rd [8:6]=rs [5:0]=imm6
// For JUMP/BEQ/BNE/BLT/BGE/CALL: [7:0] = full 8-bit target (rd and rs fields=0)
// Encoding: {opc[3:0], 2'b0, addr[7:0], 2'b0} — NO, keep uniform 16b:
// Uniform: {opc[3:0], rd[2:0], rs[2:0], imm[5:0]}
// Branches use imm[5:0] extended to 8b via zero-pad MSBs — limits target to addr<64
// To hit addr>=64, set rd[2:0] as imm[7:6]. Control unit reconstructs:
//   full_target = {rd[1:0], imm[5:0]}  (8-bit)
// This is clean and keeps a fixed 16-bit format.
//
// FINAL encoding:
//   [15:12] = opcode (4 bits)
//   [11:9]  = rd     (3 bits)
//   [8:6]   = rs     (3 bits)
//   [5:0]   = imm6   (6 bits, sign-extended for arithmetic)
//   For branches/jump/call: full 8-bit address = {rd[1:0], rs[2:0], imm[5:0]}
//   (i.e. treat [11:6] as the high bits — giving 12-bit imm, but we only need 8)
//   Simplest: use [7:0] directly, bits [15:8] = {opcode[3:0], 4'bxxxx}
//   → use {opcode, 4'b0, addr[7:0]} for control-flow instructions (16 bits total)
//   → use {opcode, rd, rs, imm6}    for ALU/memory instructions

module instr_memory (
    input      [7:0] addr,
    output     [15:0] instr
);

// Encoding helpers (used in comments below, not actual Verilog macros)
// ALU/Mem type : {4'opc, 3'rd, 3'rs, 6'imm6}
// CF  type     : {4'opc, 4'b0, 8'target}
// Register numbers: R0=3'b000 R1=3'b001 R2=3'b010 R3=3'b011
//                   R4=3'b100 R5=3'b101 R6=3'b110(LR) R7=3'b111(SP)
//
// Opcode constants (4-bit):
localparam ADD  = 4'h0,
           SUB  = 4'h1,
           MOV  = 4'h2,
           ADDI = 4'h3,
           SUBI = 4'h4,
           CMP  = 4'h5,
           CMPI = 4'h6,
           LOAD = 4'h7,
           STOR = 4'h8,
           JUMP = 4'h9,
           BEQ  = 4'hA,
           BNE  = 4'hB,
           BLT  = 4'hC,
           BGE  = 4'hD,
           CALL = 4'hE,
           RET  = 4'hF;

// Register aliases (3-bit)
localparam R0=3'd0, R1=3'd1, R2=3'd2, R3=3'd3,
           R4=3'd4, R5=3'd5, LR=3'd6, SP=3'd7;

// Instruction builders
// ALU / immediate / memory format: {opc,rd,rs,imm6}
`define I(opc,rd,rs,imm) {opc, rd, rs, imm[5:0]}
// Control-flow format: {opc, 4'b0, target}
// NOTE: do NOT part-select the target argument (e.g. target[7:0]) —
// Verilog macros are textual so 8'd0[7:0] expands to an illegal literal
// part-select.  The caller is always responsible for passing an 8-bit value.
`define CF(opc,target)   {opc, 4'b0, (target)}

reg [15:0] mem [255:0];

// ---------------------------------------------------------------
//  Recursive Fibonacci — fib(n) with n passed in R0
//
//  Stack frame (SP points to lowest used slot, grows downward):
//    before CALL:  [SP]   = free
//    frame entry:  SP-=1 → [SP] = LR (saved by callee)
//                  SP-=1 → [SP] = n
//
//  After fib(n-1) returns (R0 = fib(n-1)):
//                  SP-=1 → [SP] = fib(n-1)
//  Then reload n from frame, compute fib(n-2), call again.
//  After fib(n-2) returns (R0 = fib(n-2)):
//    pop fib(n-1) into R1, add, restore LR, clean frame, RET.
//
//  Addr layout:
//   0–17 : fib() body
//   18–19: base case (n<2: return n as-is)
//   20   : main entry — put n in R0, CALL fib, then HALT
// ---------------------------------------------------------------
//
//   Addr | Instruction          | Comment
//   -----+----------------------+------------------------------
//    0   | CMPI  R0, #2         | n < 2?
//    1   | BLT   #18            | base case → addr 18
//    2   | STORE LR, [SP]       | mem[SP] = LR
//    3   | SUBI  SP, #1         | SP--
//    4   | STORE R0, [SP]       | mem[SP] = n
//    5   | SUBI  SP, #1         | SP--
//    6   | SUBI  R0, #1         | R0 = n-1
//    7   | CALL  #0             | fib(n-1) → R0, LR=8
//    8   | MOV   R1, R0         | R1 = fib(n-1)
//    9   | SUBI  SP, #1         | SP-- (make room for fib(n-1))
//   10   | STORE R1, [SP]       | mem[SP] = fib(n-1)
//   11   | ADDI  SP, #2         | SP+=2 → point at saved n
//   12   | LOAD  R0, [SP]       | R0 = n  (reload from stack)
//   13   | SUBI  SP, #2         | SP back to fib(n-1) slot
//   14   | SUBI  R0, #2         | R0 = n-2
//   15   | CALL  #0             | fib(n-2) → R0, LR=16
//   16   | ADDI  SP, #1         | SP++ → point at fib(n-1)
//   17   | LOAD  R1, [SP]       | R1 = fib(n-1)
//   --- flow continues at 22 (after LOAD) ---
//   but we need more addr so let's re-layout cleanly at 18 onward:
//
//  After second CALL returns we need to:
//    (a) ADD  R0, R1             R0 = fib(n-2)+fib(n-1)
//    (b) ADDI SP, #3             clean 3 stack slots (fib(n-1),n,LR)
//    (c) LOAD LR, [SP]           restore LR
//    (d) RET
//
//  Since base case is also at high addr, split:
//   18 : base case   (BLT jumps here)
//   19 : RET
//   --- continuation after second CALL (target for CALL at addr 15 returns to 16):
//   16  LOAD R1, [SP]         R1 = saved fib(n-1)   (SP already at right slot after prior ADDI)
//  Re-check SP after each step carefully in the layout below.
//
// FINAL CLEAN LAYOUT
// ----------------------------------------------------------
// SP state annotation: SP=P means stack pointer register value
//   (SP always points to the LAST PUSHED item, i.e. pre-decrement push)
//
//  Entry: R0=n, LR=caller's return addr, SP=P
//
//  0  CMPI  R0,#2           ; compare n - 2 → flags
//  1  BLT   #19             ; n < 2 → base_case at 19
//  2  SUBI  SP,#1           ; SP=P-1
//  3  STORE LR,[SP]         ; mem[P-1]=LR
//  4  SUBI  SP,#1           ; SP=P-2
//  5  STORE R0,[SP]         ; mem[P-2]=n
//  6  SUBI  R0,#1           ; R0=n-1
//  7  CALL  #0              ; fib(n-1); LR=8, SP still P-2
//  8  SUBI  SP,#1           ; SP=P-3 (slot for fib(n-1))
//  9  STORE R0,[SP]         ; mem[P-3]=fib(n-1)
// 10  ADDI  SP,#2           ; SP=P-1 (peek at n = mem[P-2]? no → SP=P-1 → mem[SP]=LR)
//  Hmm: n is at mem[P-2], LR at mem[P-1], fib(n-1) at mem[P-3]
//  To load n: need SP=P-2 → ADDI SP,#1 from P-3
// 10  ADDI  SP,#1           ; SP=P-2 → pointing at n
// 11  LOAD  R1,[SP]         ; R1=n
// 12  SUBI  SP,#1           ; SP=P-3  (back to fib(n-1) slot)
// 13  SUBI  R1,#2           ; R1=n-2
// 14  MOV   R0,R1           ; R0=n-2
// 15  CALL  #0              ; fib(n-2)→R0; LR=16
// 16  LOAD  R1,[SP]         ; R1=fib(n-1) from mem[P-3]
// 17  ADD   R0,R1           ; R0=fib(n-2)+fib(n-1)
// 18  ADDI  SP,#3           ; SP=P  (restore SP: pop fib(n-1)+n+LR slots)
//    Wait — we need to restore LR before RET
//    LR is at mem[P-1] = mem[SP-2] when SP=P-3, mem[SP+2] after step 12
//    After step 18: SP=P, and LR is at mem[P-1] = mem[SP-1]
// 18  ADDI  SP,#2           ; SP=P-1 → pointing at LR
// 19  LOAD  LR,[SP]         ; restore LR
// 20  ADDI  SP,#1           ; SP=P  (fully restored)
// 21  RET                   ; PC=LR
//
// base_case (jumped to from addr 1):
// 22  RET                   ; R0=n already, just return
//
// main (RESET vector is addr 0; we place main at addr 30):
// 30  ADDI  R0,#7           ; R0 = 7 (compute fib(7)=13)
// 31  CALL  #0              ; call fib; LR=32
// 32  JUMP  #32             ; HALT (infinite self-loop)
// ----------------------------------------------------------

initial begin : load_program
    integer k;
    for (k = 0; k < 256; k = k + 1) mem[k] = 16'h0000; // default = NOP (ADD R0,R0,0)

    // ================================================================
    //  fib(n)  — entry at addr 0, argument in R0, result in R0
    //
    //  Stack frame layout (SP pre-decremented before each push):
    //    after prologue:   mem[SP+1] = caller's LR
    //                      mem[SP]   = n
    //  After first CALL returns:
    //                      mem[SP-1] = fib(n-1)   (pushed at addr 8-9)
    //  Second CALL returns to addr 16.
    //  Frame teardown restores n, fib(n-1), then LR before RET.
    //
    //  Addr | Encoding                        | Operation
    //  -----+---------------------------------+------------------------
    //   0   | CMPI  R0, #2                    | flags = R0 - 2
    //   1   | BLT   #22                       | if R0<2 → base_case
    //   2   | SUBI  SP, #1                    | SP--
    //   3   | STORE LR, [SP]                  | mem[SP] = LR
    //   4   | SUBI  SP, #1                    | SP--
    //   5   | STORE R0, [SP]                  | mem[SP] = n
    //   6   | SUBI  R0, #1                    | R0 = n-1
    //   7   | CALL  #0                        | fib(n-1); LR←8
    //   8   | SUBI  SP, #1                    | SP-- (slot for fib(n-1))
    //   9   | STORE R0, [SP]                  | mem[SP] = fib(n-1)
    //  10   | ADDI  SP, #1                    | SP++ → n slot
    //  11   | LOAD  R1, [SP]                  | R1 = n
    //  12   | SUBI  SP, #1                    | SP-- → fib(n-1) slot
    //  13   | SUBI  R1, #2                    | R1 = n-2
    //  14   | MOV   R0, R1                    | R0 = n-2
    //  15   | CALL  #0                        | fib(n-2); LR←16
    //  16   | LOAD  R1, [SP]                  | R1 = fib(n-1)
    //  17   | ADD   R0, R1                    | R0 = fib(n-2)+fib(n-1)
    //  18   | ADDI  SP, #1                    | SP++ → n slot
    //  19   | ADDI  SP, #1                    | SP++ → LR slot
    //  20   | LOAD  LR, [SP]                  | LR = saved LR
    //  21   | ADDI  SP, #1                    | SP++ (frame fully popped)
    //  22   | RET                             | PC = LR  (base & normal)
    // ================================================================

    //  0  CMPI R0, #2
    mem[0]  = {CMPI, R0,  3'b000, 6'd2};
    //  1  BLT  #22
    mem[1]  = `CF(BLT, 8'd22);
    //  2  SUBI SP, #1
    mem[2]  = {SUBI, SP,  3'b000, 6'd1};
    //  3  STORE LR, [SP]  — rd=SP (addr), rs=LR (data)
    mem[3]  = {STOR, SP,  LR,     6'd0};
    //  4  SUBI SP, #1
    mem[4]  = {SUBI, SP,  3'b000, 6'd1};
    //  5  STORE R0, [SP]
    mem[5]  = {STOR, SP,  R0,     6'd0};
    //  6  SUBI R0, #1
    mem[6]  = {SUBI, R0,  3'b000, 6'd1};
    //  7  CALL #0
    mem[7]  = `CF(CALL, 8'd0);
    //  8  SUBI SP, #1
    mem[8]  = {SUBI, SP,  3'b000, 6'd1};
    //  9  STORE R0, [SP]
    mem[9]  = {STOR, SP,  R0,     6'd0};
    // 10  ADDI SP, #1
    mem[10] = {ADDI, SP,  3'b000, 6'd1};
    // 11  LOAD R1, [SP]
    mem[11] = {LOAD, R1,  SP,     6'd0};
    // 12  SUBI SP, #1
    mem[12] = {SUBI, SP,  3'b000, 6'd1};
    // 13  SUBI R1, #2
    mem[13] = {SUBI, R1,  3'b000, 6'd2};
    // 14  MOV  R0, R1
    mem[14] = {MOV,  R0,  R1,     6'd0};
    // 15  CALL #0
    mem[15] = `CF(CALL, 8'd0);
    // 16  LOAD R1, [SP]
    mem[16] = {LOAD, R1,  SP,     6'd0};
    // 17  ADD  R0, R1
    mem[17] = {ADD,  R0,  R1,     6'd0};
    // 18  ADDI SP, #1
    mem[18] = {ADDI, SP,  3'b000, 6'd1};
    // 19  ADDI SP, #1
    mem[19] = {ADDI, SP,  3'b000, 6'd1};
    // 20  LOAD LR, [SP]
    mem[20] = {LOAD, LR,  SP,     6'd0};
    // 21  ADDI SP, #1
    mem[21] = {ADDI, SP,  3'b000, 6'd1};
    // 22  RET  (rs=LR so control reads LR via reg_data_rs)
    mem[22] = {RET,  3'd0, LR,    6'd0};

    // ================================================================
    //  HALT sentinel — testbench watches for PC==30
    //  main() is handled entirely by the testbench:
    //    - writes n into R0 directly
    //    - sets LR=30 so after fib returns PC lands here
    // ================================================================
    // 30  JUMP #30  (halt self-loop)
    mem[30] = `CF(JUMP, 8'd30);
end

assign instr = mem[addr];

`undef I
`undef CF

endmodule