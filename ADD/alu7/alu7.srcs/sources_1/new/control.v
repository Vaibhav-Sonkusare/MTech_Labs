`timescale 1ns / 1ps
// =============================================================
//  Control - top-level processor (single-cycle)
//  Modified to support direct memory addressing & reg conditions
// =============================================================

module control (
    input        clk,
    input        rst,
    output [7:0] alu_result_out,
    output [7:0] pc_out
);

localparam ADD  = 4'h0,
           SUB  = 4'h1,
           LOAD = 4'h7,
           STOR = 4'h8,
           JUMP = 4'h9,
           BNZ  = 4'hB;

reg  [7:0] pc;
wire [7:0] next_pc;

// Instruction fetch
wire [15:0] instr;
instr_memory imem (
    .addr  (pc),
    .instr (instr)
);

// Instruction decode
wire [3:0] opcode = instr[15:12];
wire [2:0] rd     = instr[11:9];
wire [2:0] rs     = instr[8:6];
wire [7:0] cf_target = instr[7:0]; // Used as address for LOAD/STOR and target for branch

reg        reg_we;     
reg        mem_we;       
reg        mem_to_reg;   
reg [2:0]  alu_op;       

always @(*) begin
    reg_we       = 1'b0;
    mem_we       = 1'b0;
    mem_to_reg   = 1'b0;
    alu_op       = 3'b000;

    case (opcode)
        ADD:  begin reg_we = 1'b1; alu_op = 3'b000; end
        SUB:  begin reg_we = 1'b1; alu_op = 3'b001; end
        LOAD: begin reg_we = 1'b1; mem_to_reg = 1'b1; end
        STOR: begin mem_we = 1'b1; end
        JUMP: begin end
        BNZ:  begin end
        default: begin end
    endcase
end

// Register file
wire [7:0] reg_data_rd;  
wire [7:0] reg_data_rs;  
wire [7:0] mem_rdata;    
wire [7:0] alu_result;

wire [7:0] wb_data = mem_to_reg ? mem_rdata : alu_result;

register_file rf (
    .clk        (clk),
    .rst        (rst),
    .we         (reg_we),
    .raddr1     (rd),
    .raddr2     (rs),
    .waddr      (rd),
    .write_data (wb_data),
    .read_data1 (reg_data_rd),
    .read_data2 (reg_data_rs)
);

// ALU
wire alu_zero;
wire alu_negative;

alu alu_unit (
    .a        (reg_data_rd),
    .b        (reg_data_rs),
    .alu_op   (alu_op),
    .result   (alu_result),
    .zero     (alu_zero),
    .negative (alu_negative)
);

// Data memory (Absolute Addressing)
wire is_mem_op = (opcode == LOAD) || (opcode == STOR);
wire [7:0] mem_addr = is_mem_op ? cf_target : 8'd0;

memory dmem (
    .clk        (clk),
    .rst        (rst),
    .we         (mem_we),
    .addr       (mem_addr),
    .write_data (reg_data_rd), // STOR writes Rd to memory
    .read_data  (mem_rdata)
);

// PC logic (Checks Register directly instead of flags)
assign next_pc =
    (opcode == JUMP)                                ? cf_target :
    (opcode == BNZ && reg_data_rd != 8'd0)          ? cf_target :
    pc + 8'd1;

always @(posedge clk) begin
    if (rst) pc <= 8'd0;
    else     pc <= next_pc;
end

assign alu_result_out = alu_result;
assign pc_out         = pc;

endmodule