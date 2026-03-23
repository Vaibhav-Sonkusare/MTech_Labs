`timescale 1ns / 1ps
// =============================================================
//  Control Unit (top-level CPU core)
//
//  Integrates: PC, instr_memory, register_file, ALU, data memory
//
//  Instruction encoding (decoded here):
//    [15:12] = opcode (4 bits)
//    [11:9]  = rd     (3 bits) - destination or address register
//    [8:6]   = rs     (3 bits) - source register
//    [5:0]   = imm6   (6 bits, sign-extended to 8 for arithmetic)
//
//  Branch/jump/call target: {instr[11:6], imm6} gives 12-bit addr
//  but we only use [7:0] - full 8-bit target packed as {4'b0, addr[7:0]}
//  where [11:8]=4'b0 and [7:0]=target. Control unit extracts: target=instr[7:0]
//
//  Datapath summary per opcode:
//    ADD/SUB/MOV/AND/OR:  alu(regA,regB) → reg[rd]
//    ADDI/SUBI:           alu(regA,imm8) → reg[rd]
//    CMP/CMPI:            alu(regA,regB_or_imm) → flags only (reg_we=0)
//    LOAD:                mem[reg[rs]] → reg[rd]
//    STORE:               reg[rs] → mem[reg[rd]]
//    JUMP:                PC ← imm8
//    BEQ/BNE/BLT/BGE:     conditional PC ← imm8
//    CALL:                LR ← PC+1, PC ← imm8
//    RET:                 PC ← LR
//
//  All instructions execute in 1 clock cycle (single-cycle design).
//  Register writes and PC update happen on posedge clk.
//  Memory write happens on posedge clk.
//  All reads (register file, instruction memory, data memory) are async.
//
//  Outputs exposed for testbench inspection:
//    alu_result - live ALU output
//    pc_out     - current program counter
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
           MOV  = 4'h2,
           ADDI = 4'h3,
           SUBI = 4'h4,
           CMP  = 4'h5,
           CMPI = 4'h6,
           LOAD = 4'h7,
           STOR = 4'h8,
           JUMP = 4'h9,
           BEQ  = 4'hA,
           BNE  = 4'hB,
           BLT  = 4'hC,
           BGE  = 4'hD,
           CALL = 4'hE,
           RET  = 4'hF;

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
wire [5:0] imm6   = instr[5:0];

// Sign-extend imm6 → 8 bits
wire signed [7:0] imm8 = {{2{imm6[5]}}, imm6};

// Branch/jump/call target uses lower 8 bits of instruction
wire [7:0] cf_target = instr[7:0];

// ---------------------------------------------------------------
// Control signals (combinational decode)
// ---------------------------------------------------------------
reg        reg_we;       // 1 = write ALU/mem result to reg[rd]
reg        mem_we;       // 1 = write to data memory
reg        mem_to_reg;   // 1 = writeback comes from data memory
reg        use_imm;      // 1 = ALU operand B is imm8
reg        use_mem_addr; // 1 = memory address is reg[rd] (register-indirect)
reg        is_call;      // 1 = save PC+1 to LR, jump to target
reg        is_ret;       // 1 = PC ← LR
reg        wr_lr;        // 1 = write PC+1 to LR (part of CALL)
reg [2:0]  alu_op;       // 3-bit ALU opcode

always @(*) begin
    // Safe defaults
    reg_we       = 1'b0;
    mem_we       = 1'b0;
    mem_to_reg   = 1'b0;
    use_imm      = 1'b0;
    use_mem_addr = 1'b0;
    is_call      = 1'b0;
    is_ret       = 1'b0;
    wr_lr        = 1'b0;
    alu_op       = 3'b000;

    case (opcode)
        ADD:  begin reg_we=1; alu_op=3'b000; end
        SUB:  begin reg_we=1; alu_op=3'b001; end
        MOV:  begin reg_we=1; alu_op=3'b010; end
        ADDI: begin reg_we=1; alu_op=3'b101; use_imm=1; end
        SUBI: begin reg_we=1; alu_op=3'b011; use_imm=1; end
        CMP:  begin           alu_op=3'b100; end          // no reg_we
        CMPI: begin           alu_op=3'b100; use_imm=1; end
        LOAD: begin reg_we=1; mem_to_reg=1; use_mem_addr=1; end
        STOR: begin mem_we=1; use_mem_addr=1; end
        JUMP: begin end
        BEQ:  begin end
        BNE:  begin end
        BLT:  begin end
        BGE:  begin end
        CALL: begin is_call=1; wr_lr=1; end
        RET:  begin is_ret=1; end
        default: begin end
    endcase
end

// ---------------------------------------------------------------
// Register file
// ---------------------------------------------------------------
wire [7:0] reg_data_rd;   // value in reg[rd]  (used as ALU A and mem addr)
wire [7:0] reg_data_rs;   // value in reg[rs]  (used as ALU B and store data)

// Declare mem_rdata here so wb_data can reference it before the memory
// instantiation block appears further down in the file.
wire [7:0] mem_rdata;

// What gets written back to the register file?
// CALL writes PC+1 to LR (reg 6); everything else writes ALU/mem result to rd
wire [2:0] wb_addr   = wr_lr ? 3'd6 : rd;
wire [7:0] wb_data   = wr_lr ? (pc + 8'd1) :
                       mem_to_reg ? mem_rdata : alu_result;
wire       wb_en     = reg_we | wr_lr;

register_file rf (
    .clk        (clk),
    .rst        (rst),
    .we         (wb_en),
    .raddr1     (rd),
    .raddr2     (rs),
    .waddr      (wb_addr),
    .write_data (wb_data),
    .read_data1 (reg_data_rd),
    .read_data2 (reg_data_rs)
);

// RET reads LR via reg_data_rs because instr_memory encodes RET with rs=R6(LR).

// ---------------------------------------------------------------
// ALU
// ---------------------------------------------------------------
wire [7:0] operand_b   = use_imm ? imm8 : reg_data_rs;
wire [7:0] alu_result;
wire       alu_zero;
wire       alu_negative;

alu alu_unit (
    .a        (reg_data_rd),
    .b        (operand_b),
    .alu_op   (alu_op),
    .result   (alu_result),
    .zero     (alu_zero),
    .negative (alu_negative)
);

// ---------------------------------------------------------------
// Flag register
// Flags are set by CMP / CMPI and held until the next CMP/CMPI.
// Branch instructions read the REGISTERED flags, not the live
// ALU output - this is essential because a branch instruction
// has opcode=BLT/BEQ etc., so the ALU inputs are driven by the
// branch instruction's rd/rs fields (which are 0 for CF-format
// instructions), not by the preceding CMP's operands.
// ---------------------------------------------------------------
reg flag_zero;
reg flag_negative;

// A flag-setting instruction is CMP or CMPI
wire is_cmp = (opcode == CMP) || (opcode == CMPI);

always @(posedge clk) begin
    if (rst) begin
        flag_zero     <= 1'b0;
        flag_negative <= 1'b0;
    end else if (is_cmp) begin
        flag_zero     <= alu_zero;
        flag_negative <= alu_negative;
    end
end

// ---------------------------------------------------------------
// Data memory (stack + data)
// ---------------------------------------------------------------
// LOAD  rd, [rs]  → address = reg[rs] = reg_data_rs
// STORE rs, [rd]  → address = reg[rd] = reg_data_rd
// The is_load signal selects between the two for register-indirect ops.
wire       is_load   = (opcode == LOAD);
wire [7:0] mem_addr  = use_mem_addr ? (is_load ? reg_data_rs : reg_data_rd) : imm8;
// mem_rdata declared above near wb_data to avoid forward-reference warning

memory dmem (
    .clk        (clk),
    .rst        (rst),
    .we         (mem_we),
    .addr       (mem_addr),
    .write_data (reg_data_rs),
    .read_data  (mem_rdata)
);

// ---------------------------------------------------------------
// PC / branch logic  - branches use registered flags
// ---------------------------------------------------------------
wire [7:0] ret_target = reg_data_rs;

assign next_pc =
    is_ret                                 ? ret_target :
    is_call                                ? cf_target  :
    (opcode == JUMP)                       ? cf_target  :
    (opcode == BEQ  &&  flag_zero)         ? cf_target  :
    (opcode == BNE  && !flag_zero)         ? cf_target  :
    (opcode == BLT  &&  flag_negative)     ? cf_target  :
    (opcode == BGE  && !flag_negative)     ? cf_target  :
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