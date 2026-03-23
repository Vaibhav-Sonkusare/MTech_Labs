`timescale 1ns / 1ps
// =============================================================
//  Testbench — tb_control
//
//  Strategy (no force/release — not reliable in Vivado XSim):
//    1. Assert reset for several cycles so all state clears.
//    2. De-assert reset. On the VERY NEXT negedge (between clock
//       edges), directly write n into regs[0] and set regs[6]=30
//       (LR = halt address) using hierarchical path assignment.
//       Because register_file uses synchronous writes, a direct
//       procedural assignment to the underlying reg array takes
//       effect immediately in simulation without needing we=1.
//    3. Also write regs[7]=8'hFF (SP) in case a previous test
//       left SP in a dirty state.
//    4. Wait for PC == 30 (the JUMP #30 halt sentinel).
//    5. Read regs[0] — that is the return value.
//
//  fib() entry is at addr 0. PC resets to 0. The testbench sets
//  LR=30 before execution begins so after the outermost RET the
//  CPU lands at the halt loop at addr 30.
//
//  Expected values:
//    fib(1)=1  fib(2)=1  fib(3)=2  fib(4)=3
//    fib(5)=5  fib(6)=8  fib(7)=13 fib(8)=21
// =============================================================
module tb_control;

// ---------------------------------------------------------------
// DUT
// ---------------------------------------------------------------
reg        clk;
reg        rst;
wire [7:0] alu_result_out;
wire [7:0] pc_out;

control uut (
    .clk            (clk),
    .rst            (rst),
    .alu_result_out (alu_result_out),
    .pc_out         (pc_out)
);

// ---------------------------------------------------------------
// Clock — 10 ns period
// ---------------------------------------------------------------
initial clk = 0;
always  #5 clk = ~clk;

// ---------------------------------------------------------------
// Test counters
// ---------------------------------------------------------------
integer pass_count;
integer fail_count;
integer cycle_count;

// ---------------------------------------------------------------
// Golden reference — pure combinational Fibonacci in Verilog
// ---------------------------------------------------------------
function [7:0] fib_ref;
    input [7:0] n;
    integer a, b, tmp, i;
    begin
        a = 0; b = 1;
        if (n == 0) fib_ref = 8'd0;
        else begin
            for (i = 1; i < n; i = i + 1) begin
                tmp = a + b; a = b; b = tmp;
            end
            fib_ref = b[7:0];
        end
    end
endfunction

// ---------------------------------------------------------------
// Task — run one test case
//   n        : Fibonacci argument (1-8)
//   expected : golden answer
// ---------------------------------------------------------------
task run_test;
    input [7:0] n;
    input [7:0] expected;
    reg   [7:0] got;
    begin

        // ---- 1. Reset the CPU ----
        rst = 1'b1;
        repeat (4) @(posedge clk);
        @(negedge clk);          // settle just after rising edge

        // ---- 2. Inject state directly into the register file ----
        //  regs[0] = n          (argument)
        //  regs[6] = 30         (LR = halt addr; outermost RET -> addr 30)
        //  regs[7] = 8'hFF      (SP = top of memory)
        uut.rf.regs[0] = n;
        uut.rf.regs[6] = 8'd30;
        uut.rf.regs[7] = 8'hFF;

        // ---- 3. Release reset ----
        rst = 1'b0;
        @(posedge clk);          // first real execution cycle

        // ---- 4. Run until halt (PC==30) or timeout ----
        cycle_count = 0;
        while (uut.pc_out !== 8'd30 && cycle_count < 5000) begin
            @(posedge clk);
            cycle_count = cycle_count + 1;
        end

        // One extra negedge for last writeback to settle
        @(negedge clk);
        got = uut.rf.regs[0];

        // ---- 5. Check ----
        if (cycle_count >= 5000) begin
            $display("  [TIMEOUT] fib(%0d) -- no halt after 5000 cycles", n);
            fail_count = fail_count + 1;
        end else if (got === expected) begin
            $display("  [PASS]    fib(%0d) = %0d  (%0d cycles)", n, got, cycle_count);
            pass_count = pass_count + 1;
        end else begin
            $display("  [FAIL]    fib(%0d) : got %0d, expected %0d  (%0d cycles)",
                     n, got, expected, cycle_count);
            fail_count = fail_count + 1;
        end
    end
endtask

// ---------------------------------------------------------------
// Main test sequence
// ---------------------------------------------------------------
initial begin
    $dumpfile("tb_control.vcd");
    $dumpvars(0, tb_control);

    pass_count = 0;
    fail_count = 0;
    rst        = 1'b1;

    $display("");
    $display("==============================================");
    $display("  Recursive Fibonacci CPU  --  Test Suite");
    $display("==============================================");

    run_test(8'd1, fib_ref(1));   // 1
    run_test(8'd2, fib_ref(2));   // 1
    run_test(8'd3, fib_ref(3));   // 2
    run_test(8'd4, fib_ref(4));   // 3
    run_test(8'd5, fib_ref(5));   // 5
    run_test(8'd6, fib_ref(6));   // 8
    run_test(8'd7, fib_ref(7));   // 13
    run_test(8'd8, fib_ref(8));   // 21

    $display("----------------------------------------------");
    $display("  Results : %0d passed,  %0d failed",
             pass_count, fail_count);
    $display("==============================================");
    $display("");

    if (fail_count == 0)
        $display("  ALL TESTS PASSED");
    else
        $display("  FAILURES -- open tb_control.vcd for waveform debug");

    $display("");
    $finish;
end

// ---------------------------------------------------------------
// Verbose trace — uncomment to print every cycle
// ---------------------------------------------------------------
// always @(posedge clk) begin
//     if (!rst)
//         $display("  t=%0t  PC=%0d  instr=%04h  R0=%0d  R1=%0d  LR=%0d  SP=%0d  ALU=%0d",
//                  $time,
//                  uut.pc_out,
//                  uut.imem.mem[uut.pc_out],
//                  uut.rf.regs[0],
//                  uut.rf.regs[1],
//                  uut.rf.regs[6],
//                  uut.rf.regs[7],
//                  uut.alu_result_out);
// end

endmodule