`timescale 1ns/1ns

module t_spi;
   
   parameter tp=16;   
   reg           clk_i;
   reg 		 master_spi_ncs;
   wire 	 master_spi_miso;
   reg 		 master_spi_mosi;
   reg           spi_en;
   reg [23:0]    in_data;
   wire [6:0]    cdiv;
   wire [3:0]    ctrl;
   reg [5:0]     adc_level;
   reg [5:0]     dac_level;
   reg [31:0]    sync_word;
    
   reg [1:0]     rx_level;
   wire          sys_en;
   wire [15:0]    gpio;
   reg            rst_n;
   
   
   initial begin
      spi_en = 0;
      rst_n = 0;
      
      master_spi_ncs = 1;
      adc_level = 6'b101010;
      dac_level = 6'b010101;
      sync_word = 32'hDDDDAAAA;
      #20 rst_n = 1;
   end

   always
     if (spi_en)
       clk_i = #(tp/2) ~clk_i;
     else
       clk_i = #(tp/2) 1;
   
   task write_spi;
      input [23:0] spi_data;
      input [7:0]  spi_bytes;
      
      integer     spi_cnt;
      integer     bits;


      begin
         bits = spi_bytes * 8;         
	 master_spi_ncs = 0;
         spi_en = 1;
	 for(spi_cnt = 0; spi_cnt <bits; spi_cnt= spi_cnt+1) begin
	    @ (negedge clk_i) begin
	       master_spi_mosi = spi_data[bits - 1 -spi_cnt];
	    end
	 end
         #2 spi_en = 0;
	 #20 master_spi_ncs = 1;
   
      end
   endtask // write_spi

   always @ (posedge clk_i)
     if (~master_spi_ncs) begin
	in_data[23:1] <= in_data[22:0];
	in_data[0] <= master_spi_miso;
     end

   always
     if (spi_en)
       clk_i = #(tp/2) ~clk_i;
     else
       clk_i = #(tp/2) 1;

   spi_if u0(.rst_n(rst_n),
	     .spi_clk_i(clk_i),
	     .spi_ncs_i(master_spi_ncs),
             .spi_mosi(master_spi_mosi),
             .spi_miso(master_spi_miso),
             .cdiv(cdiv),
             .ctrl(ctrl),
             .sys_en(sys_en),
             .adc_fifo_level(adc_level),
             .dac_fifo_level(dac_level),
             .sync_word(sync_word),
             .gpio(gpio)
	     );
   
   
   initial begin
      #200 write_spi(16'h80FF, 2);
      #100;
      #200 write_spi(16'hA00A, 2);
      #100
      #200 write_spi(16'hC0FF, 2);
      #100
      #200 write_spi(16'hE0F0, 2);
      #100
      #200 write_spi(24'h000000, 3);
      #100
      #200 write_spi(24'h200000, 3);
      #100
      #200 write_spi(24'h400000, 3);      
      #10 $finish;
   end
   
   initial begin
      $dumpfile("spi.vcd");
      $dumpvars;
   end
endmodule // t_spi
