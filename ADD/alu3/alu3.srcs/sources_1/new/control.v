`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/18/2026 01:10:38 PM
// Design Name: 
// Module Name: control
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


module control(
    input clk,
    input rst,
    output [7:0] alu_out
);

// ============================
// PROGRAM COUNTER
// ============================
reg [7:0] pc;
wire [7:0] next_pc;

// ============================
// INSTRUCTION MEMORY
// ============================
wire [15:0] instr;

instr_memory imem (
    .addr(pc),
    .instr(instr)
);

// ============================
// DECODE
// ============================
wire [2:0] opcode = instr[15:13];
wire [1:0] raddr1 = instr[12:11];
wire [1:0] raddr2 = instr[10:9];
wire [1:0] waddr  = instr[8:7];
wire [2:0] mem_addr = instr[6:4];

// ============================
// CONTROL SIGNALS
// ============================
reg reg_we, mem_we, mem_to_reg;
reg [2:0] alu_op;

// ============================
// DECODER
// ============================
always @(*) begin
    reg_we = 0;
    mem_we = 0;
    mem_to_reg = 0;
    alu_op = 3'b000;

    case (opcode)
        3'b000: begin alu_op = 3'b000; reg_we = 1; end // ADD
        3'b001: begin alu_op = 3'b001; reg_we = 1; end // COMPARE
        3'b010: begin alu_op = 3'b010; end             // BEQZ
        3'b011: begin reg_we = 1; mem_to_reg = 1; end  // LOAD
        3'b100: begin mem_we = 1; end                  // STORE
        3'b101: begin end                              // JUMP handled in PC
    endcase
end

// ============================
// DATAPATH
// ============================
wire [7:0] reg_data1, reg_data2;
wire [7:0] alu_result;
wire [7:0] mem_data;
wire [7:0] write_back;

// Register File
register_file rf (
    .clk(clk),
    .rst(rst),
    .we(reg_we),
    .raddr1(raddr1),
    .raddr2(raddr2),
    .waddr(waddr),
    .write_data(write_back),
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
    .rst(rst),
    .we(mem_we),
    .addr(mem_addr),
    .write_data(reg_data1),
    .read_data(mem_data)
);

// Writeback
assign write_back = (mem_to_reg) ? mem_data : alu_result;
assign alu_out = alu_result;

// ============================
// PC UPDATE LOGIC
// ============================
assign next_pc = (opcode == 3'b101) ? instr[7:0] : // JUMP
                 (opcode == 3'b010 && alu_result == 0) ? instr[7:0] : // BEQZ
                 pc + 1;

always @(posedge clk) begin
    if (rst)
        pc <= 0;
    else
        pc <= next_pc;
end

endmodule