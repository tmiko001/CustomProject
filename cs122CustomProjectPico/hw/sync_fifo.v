module sync_fifo #(
    parameter DATA_WIDTH = 24,
    parameter ADDR_WIDTH = 4
)(
    input  wire                  clk,
    input  wire                  reset,
    input  wire                  wr_en,
    input  wire [DATA_WIDTH-1:0] din,
    output reg  [DATA_WIDTH-1:0] dout,
    output wire                  full,
    output wire                  empty,
    input  wire                  rd_en
);

    localparam DEPTH = 1 << ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] mem [0:DEPTH-1];
    reg [ADDR_WIDTH:0]   wr_ptr;
    reg [ADDR_WIDTH:0]   rd_ptr;

    wire [ADDR_WIDTH-1:0] wr_addr = wr_ptr[ADDR_WIDTH-1:0];
    wire [ADDR_WIDTH-1:0] rd_addr = rd_ptr[ADDR_WIDTH-1:0];

    assign full  = (wr_ptr[ADDR_WIDTH] != rd_ptr[ADDR_WIDTH]) && (wr_addr == rd_addr);
    assign empty = (wr_ptr == rd_ptr);

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            wr_ptr <= 0;
        end else if (wr_en && !full) begin
            wr_ptr <= wr_ptr + 1'b1;
        end
    end

    always @(posedge clk) begin
        if (wr_en && !full) begin
            mem[wr_addr] <= din;
        end
    end

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            rd_ptr <= 0;
            dout   <= {DATA_WIDTH{1'b0}};
        end else if (rd_en && !empty) begin
            rd_ptr <= rd_ptr + 1'b1;
            dout   <= mem[rd_addr];
        end
    end

endmodule
