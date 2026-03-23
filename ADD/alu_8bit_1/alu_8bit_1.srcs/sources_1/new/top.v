`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/17/2026 12:57:31 PM
// Design Name: 
// Module Name: top
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


module top (
    input clk,

    // Register control
    input reg_we,
    input [1:0] raddr1,
    input [1:0] raddr2,
    input [1:0] waddr,

    // ALU control
    input [2:0] alu_op,

    // Memory control
    input mem_we,
    input [2:0] mem_addr,

    output [7:0] alu_out
);

wire [7:0] reg_data1, reg_data2;
wire [7:0] alu_result;
wire [7:0] mem_data;

// Register File
register_file rf (
    .clk(clk),
    .we(reg_we),
    .read_addr1(raddr1),
    .read_addr2(raddr2),
    .write_addr(waddr),
    .write_data(alu_result),
    .read_data1(reg_data1),
    .read_data2(reg_data2)
);

// ALU
alu alu_unit (
    .a(reg_data1),
    .b(reg_data2),
    .alu_op(alu_op),
    .result(alu_result)
);

// Memory
memory mem (
    .clk(clk),
    .we(mem_we),
    .addr(mem_addr),
    .write_data(alu_result),
    .read_data(mem_data)
);

assign alu_out = alu_result;

endmodule