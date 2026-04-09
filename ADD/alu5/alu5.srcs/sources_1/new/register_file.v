`timescale 1ns / 1ps

module register_file (
    input            clk,
    input            rst,
    input            we,
    input      [2:0] raddr1,      // read port 1
    input      [2:0] raddr2,      // read port 2
    input      [2:0] waddr,       // write port
    input      [7:0] write_data,
    output     [7:0] read_data1,
    output     [7:0] read_data2
);

reg [7:0] regs [7:0];

// Reset initialises the calling convention state.
// Everything else starts at 0;
// Link Register (sotring the return address);
// SP starts at 0xFF (top of 256-byte memory);
integer i;
always @(posedge clk) begin
    if (rst) begin
        for (i = 0; i < 6; i = i + 1)
            regs[i] <= 8'd0;
        regs[6] <= 8'd0;    // LR = 0
        regs[7] <= 8'hFF;   // SP = 255
    end else if (we) begin
        regs[waddr] <= write_data;
    end
end

// Asynchronous read - reflects latest value including same-cycle write
assign read_data1 = regs[raddr1];
assign read_data2 = regs[raddr2];

endmodule
