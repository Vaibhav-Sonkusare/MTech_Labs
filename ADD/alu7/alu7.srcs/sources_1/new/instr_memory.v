`timescale 1ns / 1ps
// =============================================================
//  A simple program that add two integers from memory.
//  The two integers are selected based on the value of mem[0]
//
//  Program Mapping:
//  I1 (PC=0): load r[0] mem[0]  -> LOAD R0, #0
//  I2 (PC=1): load r[1] mem[1]  -> LOAD R1, #1
//  I3 (PC=2): bnz r[0] I6       -> BNZ R0, #5
//  I4 (PC=3): load r[2] mem[2]  -> LOAD R2, #2
//  I5 (PC=4): jump I7           -> JUMP #6
//  I6 (PC=5): load r[2] mem[3]  -> LOAD R2, #3
//  I7 (PC=6): add r[1] r[2]     -> ADD R1, R2
//  I8 (PC=7): store r[1] mem[4] -> STOR R1, #4
//     (PC=8): halt              -> JUMP #8     -> 16'h9008
// =============================================================

module instr_memory (
    input      [7:0]  addr,
    output reg [15:0] instr
);

always @(*) begin
    case (addr)
        8'd0:  instr = 16'b0111_000_0_00000000; // I1: LOAD R0, mem[0]
        8'd1:  instr = 16'b0111_001_0_00000001; // I2: LOAD R1, mem[1]
        8'd2:  instr = 16'b1011_000_0_00000101; // I3: BNZ R0, I6 (PC=5)
        8'd3:  instr = 16'b0111_010_0_00000010; // I4: LOAD R2, mem[2]
        8'd4:  instr = 16'b1001_000_0_00000110; // I5: JUMP I7 (PC=6)
        8'd5:  instr = 16'b0111_010_0_00000011; // I6: LOAD R2, mem[3]
        8'd6:  instr = 16'b0000_001_010_000000; // I7: ADD R1, R2
        8'd7:  instr = 16'b1000_001_0_00000100; // I8: STOR R1, mem[4]
        8'd8:  instr = 16'b1001_000_0_00001000; // Halt (JUMP to self)
        default: instr = 16'h0000;
    endcase
end

//always @(*) begin
//    case (addr)
//        8'd0:  instr = 16'b0111_000_0_00001010; // I1: LOAD R0, mem[10]
//        8'd1:  instr = 16'b0111_001_0_00001011; // I2: LOAD R1, mem[11]
//        8'd2:  instr = 16'b0111_010_0_00001100; // I3: LOAD R2, mem[12]
        
//        8'd3:  instr = 16'b0111_011_0_00001010; // I4: LOAD R3, mem[10]
//        8'd4:  instr = 16'b0000_011_000_000000; // I5: ADD R3, R0
        
//        8'd5:  instr = 16'b0000_000_001_000000; // I6: ADD R0, R1
        
//        8'd6:  instr = 16'b0111_010_0_00001010; // I7: LOAD R2, mem[10]
//        8'd7:  instr = 16'b0000_010_011_000000; // I8: ADD R2, R3
        
//        8'd8:  instr = 16'b0111_011_0_00001011; // I9: LOAD R3, mem[11]
//        8'd9:  instr = 16'b0001_010_011_000000; // I10: SUB R2, R3
//        8'd10: instr = 16'b1011_000_0_00000011; // I11 BNZ R0, I4 (PC=3)
        
//        8'd11:  instr = 16'b1000_000_0_00001101; // I12: STOR R0, mem[13]
        
//        8'd12:  instr = 16'b1001_000_0_00001000; // Halt (JUMP to self)
        
//        default: instr = 16'h0000;
//    endcase
//end

endmodule