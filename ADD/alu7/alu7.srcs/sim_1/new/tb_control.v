`timescale 1ns / 1ps
// =============================================================
//  Testbench - Hardcoded Memory
//
//  This testbench assumes that the initial values for the test
//  (mem[0], mem[1], mem[2], mem[3]) are hardcoded directly 
//  inside the memory.v module's reset block.
// =============================================================

module tb_control;

reg clk;
reg rst;
wire [7:0] alu_result_out;
wire [7:0] pc_out;

reg [7:0] result;
integer i;

control dut (
    .clk(clk),
    .rst(rst),
    .alu_result_out(alu_result_out),
    .pc_out(pc_out)
);

// Clock generation
initial clk = 0;
always #5 clk = ~clk;

initial begin
    $display("================================================");
    $display("  Testbench: Conditional Execution Program");
    $display("================================================");

    // 1. Assert reset
    rst = 1;
    @(posedge clk); #1;
    @(posedge clk); #1;
    
    // 2. Release reset to start processor execution
    @(negedge clk);
    rst = 0;

    // 3. Wait until PC reaches the halt instruction (PC=8)
    for (i = 0; i < 20; i = i + 1) begin
        @(posedge clk);
        #1;
        if (pc_out == 8) i = 20; // break loop
    end

    // 4. Extract result and display
    result = dut.dmem.mem[8'd4];
//    result = dut.dmem.mem[8'd13];
    
    $display("Execution Halted at PC = %0d", pc_out);
    $display("Result stored at mem[4] = %0d", result);
//    $display("Result stored at mem[13] = %0d", result);
    
    $display("================================================");
    $finish;
end

// Timeout watchdog
initial begin
    #10000;
    $display("TIMEOUT");
    $finish;
end

endmodule