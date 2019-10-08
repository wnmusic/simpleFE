module t_ezusb;

   parameter tp=16;   
   reg           clk;
   reg           rst;
   wire           rx_fifo_full;
   wire          rx_fifo_wr;
   wire [7:0]    wdata;
   wire          tx_fifo_empty;
   wire          tx_fifo_rd;
   reg [7:0]     rdata;
   
   //EZUSB sync FIFO interface
   reg [7:0]     data_i;
   wire [7:0]    data_o;
   wire          data_en;
   reg          ep2out_empty_n;
   reg          ep6in_full_n;
   reg          ep6in_fullm2_n;
   wire          rd_n;
   wire          wr_n;
   wire          oe_n;
   wire [1:0]    fifoaddr;
   
   integer     tx_fifo_cnt;
   integer     rx_fifo_cnt;

   integer     ft_out_fifo_cnt;
   integer     ft_in_fifo_cnt;
   
   
   ezusb_if u0  ( 
                  .clk(clk),
                  .rst(rst),
                  .rx_fifo_fullm1(rx_fifo_full),
                  .rx_fifo_wr(rx_fifo_wr),
                  .wdata(wdata),
                  .tx_fifo_emptyp1(tx_fifo_empty),
                  .tx_fifo_rd(tx_fifo_rd),
                  .rdata(rdata),
                  .data_i(data_i),
                  .data_o(data_o),
                  .data_en(data_en),
                  .ep2out_emptyp1_n(ep2out_empty_n),
                  .ep6in_fullm2_n(ep6in_fullm2_n),
                  .ep6in_full_n(ep6in_full_n),
                  .rd_n(rd_n),
                  .wr_n(wr_n),
                  .oe_n(oe_n),
                  .fifoaddr(fifoaddr)
                  );

   initial begin
      rst = 1;
      clk = 0;
      tx_fifo_cnt = 4'hF;
      rx_fifo_cnt = 4'h0;
      ft_out_fifo_cnt = 3'h7;
      ft_in_fifo_cnt = 3'h0;
      data_i = 0;
      rdata = 0;
      #200 rst = 0;
   end // initial begin
   
   always
     clk = #(tp/2) ~clk;
   
   always @ (posedge clk)
     if(~rd_n & ~oe_n) 
        if (ft_out_fifo_cnt >= 0)begin
           ft_out_fifo_cnt <= ft_out_fifo_cnt -1;
           data_i <= data_i + 1;
        end
        else begin
           ft_out_fifo_cnt <= -1;
           $display("%m WARNING: Reading while fifo is empty (%t)",$time);
        end
   always @ (posedge clk)
     ep2out_empty_n <= (ft_out_fifo_cnt > 1); //empty plus 1 
   
   always @ (*)
     if (~ep2out_empty_n)
       #200 ft_out_fifo_cnt = 3'h7;
   
   always @ (posedge clk)
     if(~wr_n & oe_n)
        if(ft_in_fifo_cnt <= 7)  begin
          ft_in_fifo_cnt <= ft_in_fifo_cnt + 1;
          $display("ft recieve: %h(%b)", data_o, data_en);
        end
        else begin
           $display("%m WARNING: Writing while fifo is full (%t)",$time);
           ft_in_fifo_cnt <= 8;
        end

   always @ (posedge clk) begin
      ep6in_full_n <=  (ft_in_fifo_cnt < 3'h7);
      ep6in_fullm2_n <=  (ft_in_fifo_cnt < 3'h5);
   end
   
   
   always @ (*)
     if (~ep6in_full_n)
       #300 ft_in_fifo_cnt = 3'h2;

   always @ (posedge clk)
     if ((rx_fifo_cnt < 15) & rx_fifo_wr) begin
        rx_fifo_cnt <= rx_fifo_cnt + 1;
        $display("rxfifo recv: %h", wdata);
     end
   
   assign rx_fifo_full = (rx_fifo_cnt >= 4'hE);
   always @ (*)
     if (rx_fifo_full)
       #300 rx_fifo_cnt = 'h3;

   always @ (posedge clk)
     if ((tx_fifo_cnt >0) & tx_fifo_rd) begin
        tx_fifo_cnt <= tx_fifo_cnt -1;
        rdata <= rdata + 1;
     end
   assign tx_fifo_empty = (tx_fifo_cnt <= 4'h1);
   always @ (*)
     if (tx_fifo_empty)
       #300 tx_fifo_cnt = 'hE;
   
   
   initial begin
      #10000 $finish;
   end
   
   initial begin
      $dumpfile("ezusb.vcd");
      $dumpvars;
   end


endmodule // t_ezusb

   
     
