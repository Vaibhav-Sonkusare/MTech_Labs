`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/18/2026 01:25:01 PM
// Design Name: 
// Module Name: instr_memory
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


module instr_memory(
    input [7:0] addr,
    output [15:0] instr
);

reg [15:0] mem [255:0];

initial begin
    // Program starts here

    // R2 = R0 + R1  → 10 + 5 = 15
    mem[0] = 16'b000_00_01_10_000_0000; 

    // STORE R2 → MEM[3]
    mem[1] = 16'b100_10_00_00_011_0000;

    // LOAD MEM[3] → R1
    mem[2] = 16'b011_00_00_01_011_0000;

    // COMPARE R0 and R1
    mem[3] = 16'b001_00_01_11_000_0000;

    // Infinite loop (stay here)
    mem[4] = 16'b101_00000000; // JUMP to 0
end

assign instr = mem[addr];

endmodule
