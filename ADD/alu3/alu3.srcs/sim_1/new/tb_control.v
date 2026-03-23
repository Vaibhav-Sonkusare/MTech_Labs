`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/18/2026 01:11:17 PM
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
wire [7:0] alu_out;

control uut (
    .clk(clk),
    .rst(rst),
    .alu_out(alu_out)
);

// Clock
always #5 clk = ~clk;

initial begin
    clk = 0;

    // Reset system
    rst = 1;
    #10;
    rst = 0;

    // Let program run
    #10; // after ADD
    if (uut.rf.register[2] == 15) $display("PASS: ADD");
    
    #10; // after STORE
    if (uut.mem.mem[3] == 15) $display("PASS: STORE");
    
    #10; // after LOAD
    if (uut.rf.register[1] == 15) $display("PASS: LOAD");
    
    #10; // after COMPARE
    $display("COMPARE result = %d", uut.rf.register[3]);

//    #100
//    // Check results
//    if (uut.rf.register[2] == 8'd15)
//        $display("PASS: ADD");

//    if (uut.mem.mem[3] == 8'd15)
//        $display("PASS: STORE");

//    if (uut.rf.register[1] == 8'd15)
//        $display("PASS: LOAD");

//    if (uut.rf.register[3] == 8'd1)
//        $display("PASS: COMPARE");

    $display("Final R1 = %d", uut.rf.register[1]);
    $display("Memory[3] = %d", uut.mem.mem[3]);

    $finish;
end

endmodule