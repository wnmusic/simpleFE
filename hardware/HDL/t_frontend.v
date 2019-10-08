
`timescale 1ns/1ns

module t_frontend;
   
   parameter tp=16;
   reg           clk_i;
   reg 		 enable;
   reg 		 tx_fifo_rdy;
   reg 		 rx_fifo_rdy;

   reg [7:0] 	 ft_data_out;
   wire [7:0] 	 ft_data_in;
   wire 	 ft_rd;
   wire 	 ft_wr;
   
   reg [7:0] 	 adc_data;   
   wire [9:0] 	 dac_data;

   reg           tx_i_en;
   reg           tx_q_en;
   reg           rx_i_en;
   reg           rx_q_en;
   reg [6:0]     clkdiv;
   
   wire 	 max5853_clk;

   
   // original data
   reg [7:0] 	 tx_data_mem[0:4096];
   reg [7:0] 	 adc_i_data_mem[0:4096];
   reg [7:0]     adc_q_data_mem[0:4096];
   
   integer param_file, rx_file, dac_i_file, dac_q_file;
   integer tx_cnt, tx_ctrl, rx_ctrl;
   integer fres;

   initial begin
      param_file = $fopen("t_max5863_param.txt", "r");
      rx_file= $fopen("rx_out.txt");
      dac_i_file = $fopen("dac_i_out.txt");
      dac_q_file = $fopen("dac_q_out.txt");
      fres = $fscanf( param_file, "tx_cnt: %d, tx_ctrl: %d, rx_ctrl: %d, cdiv: %d\n", tx_cnt, tx_ctrl, rx_ctrl, clkdiv);
      
      $display("Total tx count: %d, tx_ctrl: %d, rx_ctrl %d, cdiv: %d", tx_cnt, tx_ctrl, rx_ctrl, clkdiv);
      tx_i_en = tx_ctrl & 1;
      tx_q_en = (tx_ctrl & 2)>>1;
      rx_i_en = rx_ctrl & 1;
      rx_q_en = (rx_ctrl & 2)>>1;
      
      $fclose(param_file);
   end
   
   frontend u0(
	       .clk(clk_i),
               .enable(enable),
	       .dac_fifo_rdy(tx_fifo_rdy),
               .dac_fifo_read(ft_rd),
	       .adc_fifo_rdy(rx_fifo_rdy),
	       .adc_fifo_write(ft_wr),
               .dac_fifo_data(ft_data_out),
               .adc_fifo_data(ft_data_in),
               .clk_div(clkdiv),
               .tx_i_en(tx_i_en),
               .tx_q_en(tx_q_en),
               .rx_i_en(rx_i_en),
               .rx_q_en(rx_q_en),
               .max_clk_o(max5863_clk),
               .dac_o(dac_data),
               .adc_i(adc_data)
	       );

   integer adc_i_int, adc_q_int, tx_int, rx_int;

   initial begin
      enable = 0;
      clk_i = 0;
      tx_fifo_rdy = 1;
      rx_fifo_rdy = 1;
      adc_i_int = 0;
      adc_q_int = 0;
      tx_int = 0;
      rx_int = 0;
      #100 enable = 1;
   end

   initial begin
      $readmemh("tx_data.txt", tx_data_mem, 0, tx_cnt+1);
      $readmemh("adc_i_data.txt", adc_i_data_mem);
      $readmemh("adc_q_data.txt", adc_q_data_mem);
   end
   
   // after negedge max5863 will give Q data
   always @ (negedge max5863_clk)
     begin
      adc_data <= adc_q_data_mem[adc_q_int];
      adc_q_int <= adc_q_int +1;
     end
   
   //after posedge max5863 will give I data,
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


   always @ (posedge clk_i)
     ft_data_out <= tx_data_mem[tx_int][7:0];
   
   always @ (posedge clk_i)
     if (ft_rd) begin
        tx_int <= tx_int + 1;
     end

   always @ (posedge clk_i) begin
      if (ft_wr) begin
         $display("FT rx: %h", ft_data_in);
         $fwrite(rx_file, "%h\n", ft_data_in);
         rx_int <= rx_int + 1;
      end
   end
      
   
   initial begin 
      #200;

      while (tx_int < tx_cnt && rx_int < tx_cnt) begin
         if (tx_int == tx_cnt/2 || rx_int == tx_cnt/3) begin
            tx_fifo_rdy = 0;
            #1000 tx_fifo_rdy = 1;
            rx_fifo_rdy = 0;
            #400 rx_fifo_rdy = 1;
         end
         else
           #1;
         
      end
      
      #50 $finish;
      $fclose(rx_file);
      $fclose(dac_i_file);
      $fclose(dac_q_file);
   end
   
   initial begin
      $dumpfile("frontend.vcd");
      $dumpvars;
   end
	


endmodule 
