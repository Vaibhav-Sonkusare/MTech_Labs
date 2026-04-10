`timescale 1ns / 1ps
// =============================================================
//  Control - top-level processor (single-cycle)
//
//  Opcodes used by the sum_n program (all others removed):
//    ADD   0x0  -  R0 = R0 + R1            (combine result + saved a)
//    SUB   0x1  -  R0 = R0 - R0 = 0        (base-case zero-out)
//    ADDI  0x3  -  Rx = Rx + imm            (SP adjustments)
//    SUBI  0x4  -  Rx = Rx - imm            (SP adjustments, a-1)
//    CMPI  0x6  -  flags ← R0 - imm         (compare a with 0)
//    LOAD  0x7  -  Rd = mem[Rs]             (pop saved a / LR)
//    STOR  0x8  -  mem[Rd] = Rs             (push LR / a)
//    BEQ   0xA  -  branch if Z == 1
//    BLT   0xC  -  branch if N == 1
//    CALL  0xE  -  LR = PC+1; PC = target
//    RET   0xF  -  PC = LR
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
           ADDI = 4'h3,
           SUBI = 4'h4,
           CMPI = 4'h6,
           LOAD = 4'h7,
           STOR = 4'h8,
           BEQ  = 4'hA,
           BLT  = 4'hC,
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

// Branch / call / jump target is the lower 8 bits
wire [7:0] cf_target = instr[7:0];

// ---------------------------------------------------------------
// Control signals (combinational decode)
// ---------------------------------------------------------------
reg        reg_we;       // 1 = write ALU/mem result to reg[rd]
reg        mem_we;       // 1 = write to data memory
reg        mem_to_reg;   // 1 = writeback comes from data memory
reg        use_imm;      // 1 = ALU operand B is sign-extended imm8
reg        use_mem_addr; // 1 = memory address comes from register
reg        is_call;      // 1 = save PC+1 to LR, jump to target
reg        is_ret;       // 1 = PC ← LR
reg        wr_lr;        // 1 = write PC+1 to LR  (CALL)
reg [2:0]  alu_op;       // 3-bit ALU opcode

always @(*) begin
    // Safe defaults - every signal explicitly driven
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
        // ---- Data-processing ----
        ADD:  begin reg_we = 1'b1; alu_op = 3'b000;             end  // R0 = R0 + R1
        SUB:  begin reg_we = 1'b1; alu_op = 3'b001;             end  // R0 = R0 - R0 = 0
        ADDI: begin reg_we = 1'b1; alu_op = 3'b000; use_imm=1;  end  // SP++
        SUBI: begin reg_we = 1'b1; alu_op = 3'b001; use_imm=1;  end  // SP--, a-1

        // ---- Compare (flags only, no writeback) ----
        CMPI: begin             alu_op = 3'b001; use_imm=1;  end  // flags ← a - 0

        // ---- Memory ----
        LOAD: begin reg_we=1; mem_to_reg=1; use_mem_addr=1;   end  // Rd = mem[Rs]
        STOR: begin mem_we=1;               use_mem_addr=1;   end  // mem[Rd] = Rs

        // ---- Branches (PC update logic handles these) ----
        BEQ:  begin end
        BLT:  begin end

        // ---- Call / Return ----
        CALL: begin is_call=1; wr_lr=1; end  // LR←PC+1, PC←target
        RET:  begin is_ret=1;           end  // PC←LR

        default: begin end
    endcase
end

// ---------------------------------------------------------------
// Register file
// ---------------------------------------------------------------
wire [7:0] reg_data_rd;   // value read from reg[rd]  (ALU A, mem addr for STOR)
wire [7:0] reg_data_rs;   // value read from reg[rs]  (ALU B, store data, RET target)

// Forward-declared so wb_data can reference it before the memory block
wire [7:0] mem_rdata;

// Writeback mux:
//   CALL  → wb_addr = R6 (LR),  wb_data = PC+1
//   LOAD  → wb_addr = rd,       wb_data = mem_rdata
//   other → wb_addr = rd,       wb_data = alu_result
wire [2:0] wb_addr = wr_lr    ? 3'd6       : rd;
wire [7:0] wb_data = wr_lr    ? (pc + 8'd1) :
                     mem_to_reg ? mem_rdata  : alu_result;
wire       wb_en   = reg_we | wr_lr;

register_file rf (
    .clk        (clk),
    .rst        (rst),
    .we         (wb_en),
    .raddr1     (rd),           // port 1: rd  → reg_data_rd
    .raddr2     (rs),           // port 2: rs  → reg_data_rs
    .waddr      (wb_addr),
    .write_data (wb_data),
    .read_data1 (reg_data_rd),
    .read_data2 (reg_data_rs)
);
// RET: instr_memory encodes RET with rs = R6 (LR), so ret_target = reg_data_rs = LR.

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
// Flags are set only by CMPI and held until the next CMPI.
// Branches read the REGISTERED flags - not the live ALU output -
// so the flags from the preceding CMPI are stable during the
// branch instruction itself.
// ---------------------------------------------------------------
reg flag_zero;
reg flag_negative;

wire is_cmp = (opcode == CMPI);

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
// Data memory
// LOAD  rd, [rs]  →  address = reg[rs] = reg_data_rs
// STOR  rs, [rd]  →  address = reg[rd] = reg_data_rd
// ---------------------------------------------------------------
wire       is_load  = (opcode == LOAD);
wire [7:0] mem_addr = use_mem_addr ? (is_load ? reg_data_rs : reg_data_rd) : imm8;

memory dmem (
    .clk        (clk),
    .rst        (rst),
    .we         (mem_we),
    .addr       (mem_addr),
    .write_data (reg_data_rs),
    .read_data  (mem_rdata)
);

// ---------------------------------------------------------------
// PC / branch logic  (uses registered flags)
// ---------------------------------------------------------------
wire [7:0] ret_target = reg_data_rs;   // rs field encodes R6 for RET

assign next_pc =
    is_ret                         ? ret_target :
    is_call                        ? cf_target  :
    (opcode == BEQ &&  flag_zero)  ? cf_target  :
    (opcode == BLT &&  flag_negative) ? cf_target :
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