
`timescale 1ns/1ns

module t_top;
   
   parameter tp=16;
   reg           clk_i;
   reg 		 rst;
   reg 		 ep2out_empty_n;
   reg 		 ep6in_full_n;
   reg 		 ep6in_fullm2_n;
   reg [7:0] 	 adc_data;
   reg [7:0] 	 ez_data_out;
   wire [7:0] 	 ez_data_in;
   wire [7:0] 	 ez_data;
   wire 	 ez_nrd;
   wire 	 ez_nwr;
   wire 	 ez_noe;
   wire [1:0]    ez_addr;
   
   reg           spi_clk;
   reg           spi_mosi;
   wire 	 spi_miso;
   reg           spi_ncs;
   reg           spi_en;
   
   wire [9:0] 	 dac_data;
   wire 	 max5853_clk;
   wire 	 tx_rx;
   
   // original data
   reg [7:0] 	 tx_data_mem[0:4096];
   reg [7:0] 	 adc_i_data_mem[0:4096];
   reg [7:0]     adc_q_data_mem[0:4096];
   reg           usb_sof;
   
   integer rx_file, dac_i_file, dac_q_file, param_file;
   integer tx_cnt;
   integer rx_int;
   integer tx_ctrl, rx_ctrl, clkdiv;
   
   reg [15:0] spi_cfg_data;
   
   integer fres;

   initial begin
      param_file = $fopen("t_max5863_param.txt", "r");
      rx_file= $fopen("rx_out.txt");
      dac_i_file = $fopen("dac_i_out.txt");
      dac_q_file = $fopen("dac_q_out.txt");
      fres = $fscanf( param_file, "tx_cnt: %d, tx_ctrl: %d, rx_ctrl: %d, cdiv: %d\n", tx_cnt, tx_ctrl, rx_ctrl, clkdiv);
      $display("Total tx count: %d, tx_ctrl: %d, rx_ctrl %d, cdiv: %d", tx_cnt, tx_ctrl, rx_ctrl, clkdiv);
      $fclose(param_file);
   end
   
   assign ez_data_in = ez_data;
   assign ez_data = ~ez_noe ? ez_data_out : 8'hzz;
   
   top u0(
	  .rst_n(~rst),
	  .clk(clk_i),
	  .ep2out_empty_n(ep2out_empty_n),
          .ep6in_fullm2_n(ep6in_fullm2_n),
	  .ep6in_full_n(ep6in_full_n),
	  .ezusb_data(ez_data),
	  .ez_rd_n(ez_nrd),
	  .ez_wr_n(ez_nwr),
	  .ez_oe_n(ez_noe),
          .ez_pktend(),
          .ez_fifoaddr(ez_addr),
          .usb_sof(usb_sof),
	  .spi_ncs_i(spi_ncs),
	  .spi_clk_i(spi_clk),
          .spi_mosi(spi_mosi),
          .spi_miso(spi_miso),
          .max_clk_o(max5863_clk),  
	  .dac_o(dac_data),
	  .adc_i(adc_data),
	  .tx_rxn(tx_rx),
          .sys_led(),
          .gpio()
	  );

   integer adc_i_int, adc_q_int, tx_int;

   initial begin
      rst = 0;
      clk_i = 0;
      ep2out_empty_n = 1;
      ep6in_full_n = 1;
      ep6in_fullm2_n = 1;
      adc_i_int = 0;
      adc_q_int = 0;
      tx_int = 0;
      rx_int = 0;
      spi_en = 0;
      spi_ncs = 1;
      usb_sof = 0;
      #30 rst = 1;
      #100 rst = 0;
   end

   initial begin
      $readmemh("tx_data.txt", tx_data_mem, 0, tx_cnt+1);
      $readmemh("adc_i_data.txt", adc_i_data_mem);
      $readmemh("adc_q_data.txt", adc_q_data_mem);
   end
   
   // aezer negedge max5863 will give Q data
   always @ (negedge max5863_clk)
     begin
      adc_data <= adc_q_data_mem[adc_q_int];
      adc_q_int <= adc_q_int +1;
     end
   
   //aezer posedge max5863 will give I data,
   always @ (posedge max5863_clk)
     begin
	adc_data <= adc_i_data_mem[adc_i_int];
	adc_i_int <= adc_i_int + 1;
     end

   
   // on negedge max5863 will latch I data. 
   always @ (negedge max5863_clk)
     begin
        $display("dac I: %h", dac_data);
        $fwrite(dac_i_file, "%h\n", dac_data);
     end

   // on posedge max5863 will latch Q data
   // only valid if IQ is enabled
   always @ (posedge max5863_clk)
     begin
        $display("dac Q: %h", dac_data);
        $fwrite(dac_q_file, "%h\n", dac_data);
     end
   
   always
     clk_i = #(tp/2) ~clk_i;

   always @ (*)
     ez_data_out = tx_data_mem[tx_int][7:0];

   always @ (posedge clk_i)
     if (~ez_noe & ~ez_nrd) begin
        tx_int <= tx_int + 1;
     end

       
   always @ (posedge clk_i) begin
      if (~ez_nwr) begin
         rx_int = rx_int + 1;
         $display("EZ rx: %h", ez_data_in);
         $fwrite(rx_file, "%h\n", ez_data_in);
      end
   end


   always
     if (spi_en)
       spi_clk = #(tp/2) ~spi_clk;
     else
       spi_clk = #(tp/2) 1;
   
   task write_spi;
      input [15:0] spi_data;
      integer     spi_cnt;
      
      begin
         spi_data[15] = 1'b1;
	 spi_ncs = 0;
         spi_en = 1;
	 for(spi_cnt = 0; spi_cnt <16; spi_cnt= spi_cnt+1) begin
	    @ (negedge spi_clk) begin
	       spi_mosi = spi_data[15-spi_cnt];
	    end
	 end
         #2 spi_en = 0;
	 #20 spi_ncs = 1;
      end
   endtask // write_spi

   reg [23:0] slave_rd_data;
   task read_spi;
      integer     spi_cnt;
                begin
	 spi_ncs = 0;
         spi_en = 1;
	 for(spi_cnt = 0; spi_cnt <24; spi_cnt= spi_cnt+1) begin
	    @ (negedge spi_clk) begin
	       spi_mosi = 0;
               slave_rd_data[23-spi_cnt] = spi_miso;
	    end
	 end
         #2 spi_en = 0;
	 #20 spi_ncs = 1;
         $display("spi read: %h", slave_rd_data);
      end
   endtask // read_spi

   always @ (*)
     if ((tx_int == tx_cnt /4) || (tx_int == tx_cnt/2) || (tx_int == tx_cnt*3/4)) begin
        usb_sof = 1'b1;
        #200 usb_sof = 1'b0;
     end

   always @(*)
     if (tx_int % 16 == 16) begin
        #1 ep2out_empty_n = 0;
        #(10*tp + 2) ep2out_empty_n = 1;
     end


   /*
   always @(*) 
     if (rx_int % 16 == 13) begin
        #1 ep6in_fullm2_n = 0;
        #(2*tp) ep6in_full_n = 0;
        #(128*tp + 2) ep6in_full_n = 1;
        ep6in_fullm2_n = 1;
     end
     */   
           
        
   integer batch_count;
   integer i;
   
   initial  begin 
      #200;
      spi_cfg_data = {{1'b1, 2'b01, 5'b00000}, {1'b0, clkdiv[6:0]}};
      write_spi(spi_cfg_data);
      #200;
      spi_cfg_data = {{1'b1, 2'b00, 5'b00000}, {3'b000, tx_ctrl[1:0], rx_ctrl[1:0], 1'b1}};
      write_spi(spi_cfg_data);

      if (tx_ctrl) begin
         while (tx_int < tx_cnt) 
           # 5;
      end
      else begin
         while (rx_int < tx_cnt)
           #5;
      end

      #50
      
      read_spi;

      #50 $finish;
      $fclose(rx_file);
      $fclose(dac_i_file);
      $fclose(dac_q_file);
   end
   
   initial begin
      $dumpfile("top.vcd");
      $dumpvars;
   end
	


endmodule // t_ez232
