module t_loopback;

   parameter tp=16;
   reg           clk_i;
   reg 		 rst_n;
   reg 		 ep2out_empty_n;
   reg 		 ep6in_full_n;
   reg [7:0] 	 adc_data;
   reg [7:0] 	 ez_data_out;
   wire [7:0] 	 ez_data_in;
   wire [7:0] 	 ez_data;
   wire 	 ez_nrd;
   wire 	 ez_nwr;
   wire 	 ez_noe;
   wire [1:0]    ez_addr;
   integer       tx_int;
   integer       rx_int;

   assign ez_data_in = ez_data;
   assign ez_data = ~ez_noe ? ez_data_out : 8'hzz;
   
   initial begin
      rst_n = 0;
      clk_i = 0;
      ep2out_empty_n = 0;
      ep6in_full_n = 1;
      tx_int = 0;
      rx_int = 0;
      #100 rst_n = 1;
      #500 ep2out_empty_n = 1;
   end

   
   always
     clk_i = #(tp/2) ~clk_i;

   always @ (posedge clk_i)
     if (ez_noe & ~ez_nwr) begin
        rx_int <= rx_int + 1;
        $display("rx: %h", ez_data);
     end

   always @ (*)
     ez_data_out <= tx_int[7:0];
   
   always @ (posedge clk_i)
     if (~ez_noe & ~ez_nrd) begin
        tx_int <= tx_int + 1;
     end

   always @ (*)
     if ((tx_int & 8'hFF) == 8'h0F) begin
        ep2out_empty_n = 0;
        #200 ep2out_empty_n = 1;
     end

   always @ (*)
     if ((rx_int & 8'hFF) == 8'h0F) begin
        ep6in_full_n = 0;
        #200 ep6in_full_n = 1;
     end
   

   
   test_ezusb u0
     (
      .clk(clk_i),
      .rst_n(rst_n),
      .ep2out_empty_n(ep2out_empty_n),
      .ep6in_full_n(ep6in_full_n),
      .ez_rd_n(ez_nrd),
      .ez_wr_n(ez_nwr),
      .ez_oe_n(ez_noe),
      .ez_pktend(),
      .ez_fifoaddr(),
      .ezusb_data(ez_data),
      .sys_led()
   );



   initial begin
      #10000 $finish;
   end
   
   initial begin
      $dumpfile("loopback.vcd");
      $dumpvars;
   end
   
endmodule // t_loopback
