`timescale 1ns / 1ps
// =============================================================
//  Control - top-level processor (single-cycle)
//
//  Opcodes active in this build:
//    ADD   0x0  -  Rd = Rd + Rs
//    SUB   0x1  -  Rd = Rd - Rs
//    CMP   0x5  -  flags ← Rd - Rs  (no writeback)
//    LOAD  0x7  -  Rd = mem[Rs]
//    STOR  0x8  -  mem[Rd] = Rs
//    BNE   0xB  -  if Z==0: PC ← target
//    JUMP  0x9  -  PC ← target
// =============================================================

module control (
    input        clk,
    input        rst,
    output [7:0] alu_result_out,
    output [7:0] pc_out
);

// ---------------------------------------------------------------
// Opcode parameters (must match instr_memory.v)
// ---------------------------------------------------------------
localparam ADD  = 4'h0,
           SUB  = 4'h1,
           CMP  = 4'h5,
           LOAD = 4'h7,
           STOR = 4'h8,
           JUMP = 4'h9,
           BNE  = 4'hB;

// ---------------------------------------------------------------
// Program counter
// ---------------------------------------------------------------
reg  [7:0] pc;
wire [7:0] next_pc;

// ---------------------------------------------------------------
// Instruction fetch
// ---------------------------------------------------------------
wire [15:0] instr;
instr_memory imem (
    .addr  (pc),
    .instr (instr)
);

// ---------------------------------------------------------------
// Instruction decode
// ---------------------------------------------------------------
wire [3:0] opcode = instr[15:12];
wire [2:0] rd     = instr[11:9];
wire [2:0] rs     = instr[8:6];

// Branch / jump target occupies lower 8 bits
wire [7:0] cf_target = instr[7:0];

// ---------------------------------------------------------------
// Control signals (combinational decode)
// ---------------------------------------------------------------
reg        reg_we;       // 1 = write ALU result to reg[rd]
reg        mem_we;       // 1 = write to data memory
reg        mem_to_reg;   // 1 = writeback from data memory (LOAD)
reg        use_mem_addr; // 1 = memory address is a register value
reg [2:0]  alu_op;       // ALU operation select

always @(*) begin
    reg_we       = 1'b0;
    mem_we       = 1'b0;
    mem_to_reg   = 1'b0;
    use_mem_addr = 1'b0;
    alu_op       = 3'b000;

    case (opcode)
        ADD:  begin reg_we = 1'b1; alu_op = 3'b000; end
        SUB:  begin reg_we = 1'b1; alu_op = 3'b001; end
        CMP:  begin               alu_op = 3'b001; end  // flags only, no reg_we
        LOAD: begin reg_we = 1'b1; mem_to_reg = 1'b1; use_mem_addr = 1'b1; end
        STOR: begin mem_we = 1'b1;                     use_mem_addr = 1'b1; end
        JUMP: begin end
        BNE:  begin end
        default: begin end
    endcase
end

// ---------------------------------------------------------------
// Register file
// ---------------------------------------------------------------
wire [7:0] reg_data_rd;  // value of reg[rd]  - ALU input A, mem addr for STOR
wire [7:0] reg_data_rs;  // value of reg[rs]  - ALU input B, store data, LOAD addr

wire [7:0] mem_rdata;    // forward declaration (defined at memory block below)

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

// ---------------------------------------------------------------
// ALU  (operand B is always a register for this program)
// ---------------------------------------------------------------
wire [7:0] alu_result;
wire       alu_zero;
wire       alu_negative;

alu alu_unit (
    .a        (reg_data_rd),
    .b        (reg_data_rs),
    .alu_op   (alu_op),
    .result   (alu_result),
    .zero     (alu_zero),
    .negative (alu_negative)
);

// ---------------------------------------------------------------
// Flag register
// Updated only by CMP; held stable during branch instructions.
// ---------------------------------------------------------------
reg flag_zero;

always @(posedge clk) begin
    if (rst)
        flag_zero <= 1'b0;
    else if (opcode == CMP)
        flag_zero <= alu_zero;
end

// ---------------------------------------------------------------
// Data memory
//   LOAD rd,[rs]  →  address = reg[rs] = reg_data_rs
//   STOR rs,[rd]  →  address = reg[rd] = reg_data_rd
// ---------------------------------------------------------------
wire is_load = (opcode == LOAD);
wire [7:0] mem_addr = use_mem_addr ? (is_load ? reg_data_rs : reg_data_rd) : 8'd0;

memory dmem (
    .clk        (clk),
    .rst        (rst),
    .we         (mem_we),
    .addr       (mem_addr),
    .write_data (reg_data_rs),
    .read_data  (mem_rdata)
);

// ---------------------------------------------------------------
// PC / branch logic  (reads registered flag_zero)
// ---------------------------------------------------------------
assign next_pc =
    (opcode == JUMP)               ? cf_target :
    (opcode == BNE && !flag_zero)  ? cf_target :
    pc + 8'd1;

always @(posedge clk) begin
    if (rst) pc <= 8'd0;
    else     pc <= next_pc;
end

// ---------------------------------------------------------------
// Debug outputs
// ---------------------------------------------------------------
assign alu_result_out = alu_result;
assign pc_out         = pc;

endmodule