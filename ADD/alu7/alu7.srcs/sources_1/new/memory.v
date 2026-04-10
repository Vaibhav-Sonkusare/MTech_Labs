`timescale 1ns / 1ps

module memory (
    input            clk,
    input            rst,
    input            we,
    input      [7:0] addr,
    input      [7:0] write_data,
    output     [7:0] read_data
);

reg [7:0] mem [255:0];  // 2^8

integer i;
always @(posedge clk) begin
    if (rst) begin
        for (i = 0; i < 256; i = i + 1)
            mem[i] <= 8'd0;
        // Test values
        mem[0] <= 0;
        mem[1] <= 5;
        mem[2] <= 6;
        mem[3] <= 4;
//        mem[10] <= 0;
//        mem[11] <= 1;
//        mem[12] <= 2;
    end else if (we) begin
        mem[addr] <= write_data;
    end
end

// Asynchronous read so LOAD result is available the same cycle
assign read_data = mem[addr];

endmodule
