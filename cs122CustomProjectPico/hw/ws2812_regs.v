module ws2812_regs(
    input  wire         clk,
    input  wire         rst,

    // Write port
    input  wire         wr_en,
    input  wire [1:0]   wr_bank,
    input  wire [5:0]   wr_index,
    input  wire [23:0]  wr_data,

    // Read port (simple request -> one-cycle valid response)
    input  wire         rd_req,
    input  wire [1:0]   rd_bank,
    input  wire [5:0]   rd_index,
    output reg  [23:0]  rd_data,
    output reg          rd_valid
);

    // Flat memory: 3 banks * 60 pixels = 180 entries
    localparam BANKS = 3;
    localparam PER_BANK = 60;
    localparam TOTAL = BANKS * PER_BANK;

    reg [23:0] mem [0:TOTAL-1];

    // Compute addresses
    wire [8:0] wr_addr = {wr_bank, 6'b0} + wr_index; // bank*64 + index (safe)
    wire [8:0] rd_addr = {rd_bank, 6'b0} + rd_index;

    integer i;
    always @(posedge clk) begin
        if (rst) begin
            for (i = 0; i < TOTAL; i = i + 1) begin
                mem[i] <= 24'h000000;
            end
            rd_data <= 0;
            rd_valid <= 0;
        end else begin
            // write side
            if (wr_en) begin
                if (wr_index < PER_BANK && wr_bank < BANKS) begin
                    mem[wr_addr] <= wr_data;
                end
            end

            // read side: simple synchronous response
            if (rd_req) begin
                if (rd_index < PER_BANK && rd_bank < BANKS) begin
                    rd_data <= mem[rd_addr];
                end else begin
                    rd_data <= 24'h000000;
                end
                rd_valid <= 1'b1;
            end else begin
                rd_valid <= 1'b0;
            end
        end
    end

endmodule
