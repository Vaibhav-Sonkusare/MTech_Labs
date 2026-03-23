`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/18/2026 08:41:39 AM
// Design Name: 
// Module Name: tb_control
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

module tb_control;

reg clk, rst;
reg [15:0] instr;

wire [7:0] alu_out;

control uut (
    .clk(clk),
    .rst(rst),
    .instr(instr),
    .alu_out(alu_out)
);

// Clock
always #5 clk = ~clk;

// Execute task
task execute;
    input [15:0] i;
    begin
        instr = i;
        #10;
    end
endtask

initial begin
    clk = 0;
    instr = 0;

    // ============================
    // RESET SYSTEM
    // ============================
    rst = 1;
    #10;
    rst = 0;

    // ============================
    // TEST 1: ADD
    // R2 = R0 + R1 (both 0 → result 0)
    // ============================
    execute(16'b000_00_01_10_000_0000);

    // ============================
    // TEST 2: STORE R2 → MEM[3]
    // ============================
    execute(16'b100_10_00_00_011_0000);

    // ============================
    // TEST 3: LOAD R1 ← MEM[3]
    // ============================
    execute(16'b011_00_00_01_011_0000);

    // ============================
    // VERIFY
    // ============================
    #1;
    if (uut.rf.register[1] == uut.mem.mem[3])
        $display("PASS: LOAD/STORE verified");
    else
        $display("FAIL: LOAD/STORE mismatch");

    #20;
    $finish;
end

endmodule