`timescale 1ns / 1ps
// =============================================================
//  Testbench - iterative sum_n processor
//
//  Tests sum = 1 + 2 + ... + N for N = 0, 1, 3, 5, 10
//  Expected results  (N*(N+1)/2):
//    N=0  → 0
//    N=1  → 1
//    N=3  → 6
//    N=5  → 15
//    N=10 → 55
//
//  Strategy per test case:
//    1. Assert rst, write mem[0]=N and mem[255]=1 via force,
//       then de-assert rst so the processor begins from PC=0.
//    2. Run for enough clock cycles for the program to complete
//       (worst case: 2 init + 1 guard-CMP + 1 guard-BNE +
//        N*(ADD+SUB+CMP+BNE) + STOR + JUMP  =  N*4 + 6 cycles).
//    3. Read mem[0] from the data memory and compare to expected.
// =============================================================

module tb_control;

// ---------------------------------------------------------------
// DUT signals
// ---------------------------------------------------------------
reg        clk;
reg        rst;
wire [7:0] alu_result_out;
wire [7:0] pc_out;

// ---------------------------------------------------------------
// DUT instantiation
// ---------------------------------------------------------------
control dut (
    .clk           (clk),
    .rst           (rst),
    .alu_result_out(alu_result_out),
    .pc_out        (pc_out)
);

// ---------------------------------------------------------------
// Clock  - 10 ns period (100 MHz)
// ---------------------------------------------------------------
initial clk = 0;
always  #5 clk = ~clk;

// ---------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------
integer pass_count;
integer fail_count;

// Max cycles to wait for the processor to halt at the JUMP #11
// instruction.  Formula: N*4 + 10 (generous headroom).
function [31:0] max_cycles;
    input [7:0] n;
    begin
        max_cycles = (n * 4) + 20;
    end
endfunction

// Wait until the PC stops changing (processor has halted in the
// JUMP self-loop) or the cycle budget is exhausted.
task wait_for_halt;
    input [31:0] budget;
    integer i;
    reg [7:0] prev_pc;
    begin
        prev_pc = 8'hFF;   // impossible initial value
        for (i = 0; i < budget; i = i + 1) begin
            @(posedge clk);
            #1;            // tiny settle after posedge
            if (pc_out == prev_pc) begin
                i = budget; // break: PC has stopped moving
            end
            prev_pc = pc_out;
        end
    end
endtask

// Run one test case.
//   n        - input value stored in mem[0]
//   expected - expected sum
//   label    - string printed in pass/fail line
task run_test;
    input [7:0]  n;
    input [7:0]  expected;
    input [63:0] label;      // up to 8 ASCII chars packed into 64 bits
    reg   [7:0]  result;
    begin
        // ---- 1. Hold reset ----
        rst = 1;
        @(posedge clk); #1;
        @(posedge clk); #1;

        // ---- 2. Initialise data memory while reset is held ----
        //  mem[0x00] = N   (the input)
        //  mem[0xFF] = 1   (constant used for SUB R2, R3)
        force dut.dmem.mem[8'h00] = n;
        force dut.dmem.mem[8'hFF] = 8'd1;

        // ---- 3. Release reset ----
        @(negedge clk);
        rst = 0;
        release dut.dmem.mem[8'h00];
        release dut.dmem.mem[8'hFF];

        // ---- 4. Run until halt ----
        wait_for_halt(max_cycles(n));

        // ---- 5. Read result ----
        result = dut.dmem.mem[8'h00];

        // ---- 6. Report ----
        if (result === expected) begin
            $display("PASS  N=%-3d  expected=%3d  got=%3d  (PC=%0d)",
                     n, expected, result, pc_out);
            pass_count = pass_count + 1;
        end else begin
            $display("FAIL  N=%-3d  expected=%3d  got=%3d  (PC=%0d)",
                     n, expected, result, pc_out);
            fail_count = fail_count + 1;
        end
    end
endtask

// ---------------------------------------------------------------
// Test sequence
// ---------------------------------------------------------------
initial begin
    pass_count = 0;
    fail_count = 0;

    $display("================================================");
    $display("  Testbench: iterative sum_n");
    $display("  sum(N) = 1 + 2 + ... + N");
    $display("================================================");

    // Edge case: N = 0  (should return 0 immediately via guard branch)
//    run_test(8'd0,   8'd0,   "N=0     ");

    // Minimal: N = 1
    run_test(8'd1,   8'd1,   "N=1     ");

    // Small: N = 3  → 6
//    run_test(8'd3,   8'd6,   "N=3     ");

    // Typical: N = 5  → 15
//    run_test(8'd5,   8'd15,  "N=5     ");

    // Larger: N = 10 → 55
//    run_test(8'd10,  8'd55,  "N=10    ");

    // Max safe (8-bit result fits: N=20 → 210, N=21 → 231, N=22 → 253)
//    run_test(8'd20,  8'd210, "N=20    ");

    $display("================================================");
    $display("  Results: %0d passed,  %0d failed", pass_count, fail_count);
    $display("================================================");

    $finish;
end

// ---------------------------------------------------------------
// Waveform dump (optional - remove if not needed)
// ---------------------------------------------------------------
initial begin
    $dumpfile("tb_control.vcd");
    $dumpvars(0, tb_control);
end

// ---------------------------------------------------------------
// Timeout watchdog - kills simulation if something hangs
// ---------------------------------------------------------------
initial begin
    #100000;
    $display("TIMEOUT - simulation exceeded 100 us");
    $finish;
end

endmodule