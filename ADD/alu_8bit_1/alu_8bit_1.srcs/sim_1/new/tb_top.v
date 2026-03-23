`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 01:02:28 PM
// Design Name: 
// Module Name: tb_top
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


module tb_top;

// Inputs
reg clk;

reg reg_we;
reg [1:0] raddr1, raddr2, waddr;

reg [2:0] alu_op;

reg mem_we;
reg [2:0] mem_addr;

// Output
wire [7:0] alu_out;

// Instantiate DUT (Device Under Test)
top uut (
    .clk(clk),
    .reg_we(reg_we),
    .raddr1(raddr1),
    .raddr2(raddr2),
    .waddr(waddr),
    .alu_op(alu_op),
    .mem_we(mem_we),
    .mem_addr(mem_addr),
    .alu_out(alu_out)
);

// Clock generation (10ns period)
always #5 clk = ~clk;

initial begin
    // Initialize signals
    clk = 0;
    reg_we = 0;
    mem_we = 0;
    alu_op = 3'b000;
    raddr1 = 0;
    raddr2 = 0;
    waddr = 0;
    mem_addr = 0;

    // Wait a bit
    #10;

    // ------------------------------------
    // STEP 1: Initialize Registers manually
    // ------------------------------------
    // We simulate writing constants by forcing ALU inputs

    // Write 10 to R0
    force uut.rf.registers[0] = 8'd10;
    force uut.rf.registers[1] = 8'd5;
    force uut.rf.registers[2] = 8'd20;
    force uut.rf.registers[3] = 8'd3;

    #10;

    release uut.rf.registers[0];
    release uut.rf.registers[1];
    release uut.rf.registers[2];
    release uut.rf.registers[3];

    // ------------------------------------
    // STEP 2: ADD R0 + R1 → R2
    // ------------------------------------
    raddr1 = 2'b00; // R0
    raddr2 = 2'b01; // R1
    waddr  = 2'b10; // R2
    alu_op = 3'b000; // ADD
    reg_we = 1;

    #10;
    reg_we = 0;

    // ------------------------------------
    // STEP 3: SUB R2 - R3 → R0
    // ------------------------------------
    raddr1 = 2'b10; // R2
    raddr2 = 2'b11; // R3
    waddr  = 2'b00; // R0
    alu_op = 3'b001; // SUB
    reg_we = 1;

    #10;
    reg_we = 0;

    // ------------------------------------
    // STEP 4: AND R0 & R1 → R1
    // ------------------------------------
    raddr1 = 2'b00;
    raddr2 = 2'b01;
    waddr  = 2'b01;
    alu_op = 3'b010; // AND
    reg_we = 1;

    #10;
    reg_we = 0;

    // ------------------------------------
    // STEP 5: Store ALU result to memory
    // ------------------------------------
    mem_addr = 3'b001;
    mem_we = 1;

    #10;
    mem_we = 0;

    // ------------------------------------
    // STEP 6: OR operation
    // ------------------------------------
    raddr1 = 2'b01;
    raddr2 = 2'b11;
    waddr  = 2'b10;
    alu_op = 3'b011; // OR
    reg_we = 1;

    #10;
    reg_we = 0;

    // ------------------------------------
    // STEP 7: SLT (Set Less Than)
    // ------------------------------------
    raddr1 = 2'b11;
    raddr2 = 2'b10;
    waddr  = 2'b01;
    alu_op = 3'b100; // SLT
    reg_we = 1;

    #10;
    reg_we = 0;

    // Finish simulation
    #20;
    $finish;

end

endmodule
