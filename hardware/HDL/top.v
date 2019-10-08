/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2019  Ning Wang, All rights reserved          ////
////                     nwang.cooper@gmail.com                  ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////


`ifdef _IVERILOG_SIM
 `define FIFO_AW       6
 `define SOF_PER_SEC   2
`else
 `define FIFO_AW       12
 `define SOF_PER_SEC   8000
`endif

 
module top
  (
   input         clk,
   input         rst_n,
   //EZUSB fifo interface 
   input         ep2out_empty_n,
   input         ep6in_fullm2_n,
   input         ep6in_full_n,
   output        ez_rd_n,
   output        ez_wr_n,
   output        ez_oe_n,
   output        ez_pktend,
   output [1:0]  ez_fifoaddr,
   inout [7:0]   ezusb_data,
   input         usb_sof,
   // EZUSB spi interface
   input         spi_clk_i,
   input         spi_ncs_i,
   input         spi_mosi,
   output        spi_miso,
   // Max5863
   output        max_clk_o,
   output [9:0]  dac_o,
   input [7:0]   adc_i,
   // GPIO/LED status
   output        tx_rxn,
   output [1:0]  sys_led,
   output [15:0] gpio
   );


   wire         tx_i_en, tx_q_en, rx_i_en, rx_q_en;
   wire         tx_rxn, sys_en;
   wire [6:0]   sys_cdiv;
   wire [3:0]   sys_ctrl;
   wire [7:0]   data_out_ezusb;
   wire         ezusb_data_en;
   // ezusb -> dac_fifo -> max5863
   wire [7:0]   dac_fifo_in_data;
   wire [7:0]   dac_fifo_out_data;
   wire         dac_fifo_wr;
   wire         dac_fifo_rd;
   wire         dac_fifo_full;
   wire         dac_fifo_empty;

   // ezusb <- adc_fifo <- max5863
   wire [7:0]   adc_fifo_in_data;
   wire [7:0]   adc_fifo_out_data;
   wire         adc_fifo_wr;
   wire         adc_fifo_rd;
   wire         adc_fifo_full;
   wire         adc_fifo_empty;

   reg          adc_fifo_overflow;
   reg          dac_fifo_underflow;
   reg          dac_fifo_overflow;
   
   wire [5:0]   adc_fifo_level;
   wire [5:0]   dac_fifo_level;

   reg [5:0]   adc_fifo_level_q;
   reg [5:0]   dac_fifo_level_q;
   reg [31:0]  sync_word;
   
   
   assign tx_q_en = sys_ctrl[3];
   assign tx_i_en = sys_ctrl[2];
   assign rx_q_en = sys_ctrl[1];
   assign rx_i_en = sys_ctrl[0];

   assign tx_rxn = (tx_i_en | tx_q_en);
   assign sys_led[0] = dac_fifo_underflow | (usb_sof & sys_en);
   assign sys_led[1] = dac_fifo_overflow;
   
   assign ezusb_data = ezusb_data_en ? data_out_ezusb : 8'hzz;   //inout
   

   /*
   assign gpio[0] = clk;
   assign gpio[1] = dac_fifo_full;
   assign gpio[2] = dac_fifo_empty;
   assign gpio[3] = adc_fifo_full;
   assign gpio[4] = adc_fifo_empty;

   assign gpio[5] = dac_fifo_wr;
   assign gpio[6] = dac_fifo_rd;

   assign gpio[7] = adc_fifo_wr;
   assign gpio[8] = adc_fifo_rd;

   assign gpio[11] = ez_rd_n;
   assign gpio[10] = ep2out_empty_n;

   assign gpio[15] = max_clk_o;

   assign gpio[9] =  ep6in_fullm2_n;   
   assign gpio[14] = ez_wr_n;
   assign gpio[13] = ep6in_full_n;
   assign gpio[12] = adc_fifo_level[0];
    */   
   
   
   spi_if  spi0(.rst_n(rst_n),
	        .spi_clk_i(spi_clk_i),
	        .spi_ncs_i(spi_ncs_i),
                .spi_mosi(spi_mosi),
                .spi_miso(spi_miso),
                .cdiv(sys_cdiv),
                .ctrl(sys_ctrl),
                .sys_en(sys_en),
                .dac_fifo_level(dac_fifo_level_q),                
                .adc_fifo_level(adc_fifo_level_q),
                .sync_word(sync_word),
                .gpio(gpio)
	        );
   
`ifdef _IVERILOG_SIM
   assign adc_clr = 1'b0;
   assign dac_clr = 1'b0;
`else
   assign adc_clr = ~((rx_i_en | rx_q_en) & sys_en);
   assign dac_clr = ~((tx_i_en | tx_q_en) & sys_en);
`endif

   generic_fifo_dc #(.aw(`FIFO_AW), .n(3))  adc_fifo0
     (.rd_clk(clk),
      .wr_clk(clk),
      .rst_n(rst_n), 
      .clr(adc_clr), 
      .din(adc_fifo_in_data), 
      .we(adc_fifo_wr), 
      .dout(adc_fifo_out_data), 
      .re(adc_fifo_rd),
      .full(adc_fifo_full),
      .empty(),
      .full_n(), 
      .empty_n(adc_fifo_empty), 
      .level(adc_fifo_level)
      );

   generic_fifo_dc #(.aw(`FIFO_AW), .n(3)) dac_fifo0
     (.rd_clk(clk),
      .wr_clk(clk),
      .rst_n(rst_n), 
      .clr(dac_clr), 
      .din(dac_fifo_in_data), 
      .we(dac_fifo_wr), 
      .dout(dac_fifo_out_data), 
      .re(dac_fifo_rd),
      .full(),
      .empty(dac_fifo_empty),
      .full_n(dac_fifo_full), 
      .empty_n(), 
      .level(dac_fifo_level)
      );

   ezusb_if ez0( 
                 .clk(clk),
                 .rst(~rst_n),
                 .rx_fifo_fullm1(dac_fifo_full),
                 .rx_fifo_wr(dac_fifo_wr),
                 .wdata(dac_fifo_in_data),
                 .tx_fifo_emptyp1(adc_fifo_empty),
                 .tx_fifo_rd(adc_fifo_rd),
                 .rdata(adc_fifo_out_data),
                 .data_i(ezusb_data),
                 .data_o(data_out_ezusb),
                 .data_en(ezusb_data_en),
                 .ep2out_emptyp1_n(ep2out_empty_n),
                 .ep6in_fullm2_n(ep6in_fullm2_n),
                 .ep6in_full_n(ep6in_full_n),
                 .rd_n(ez_rd_n),
                 .wr_n(ez_wr_n),
                 .oe_n(ez_oe_n),
                 .fifoaddr(ez_fifoaddr)
                 );

   frontend max0(.clk(clk),
                 .enable(sys_en),
                 .dac_fifo_rdy(~dac_fifo_empty), /*tx to max5863 */
                 .dac_fifo_read(dac_fifo_rd),
                 .adc_fifo_rdy(~adc_fifo_full),    /*rx to max5863 */
                 .adc_fifo_write(adc_fifo_wr),
                 .dac_fifo_data(dac_fifo_out_data),
                 .adc_fifo_data(adc_fifo_in_data),
                 .tx_i_en(tx_i_en),
                 .tx_q_en(tx_q_en),
                 .rx_i_en(rx_i_en),
                 .rx_q_en(rx_q_en),
                 .clk_div(sys_cdiv),
                 .max_clk_o(max_clk_o),
                 .dac_o(dac_o),
                 .adc_i(adc_i)
                 );


   /* pktend: whenever ep6in is availabel, and when adc_clr asserted 
              assert ptkend for one clock 
    */
   reg         _adc_clr;
   reg         _adc_clr2;
   always @ (posedge clk) begin
      _adc_clr <= adc_clr;
      _adc_clr2 <= _adc_clr;
   end
   assign ez_pktend = ~(_adc_clr & ~_adc_clr2 & ep6in_full_n);
   
     


   /* record overflow and under flow  */
   reg          dac_fifo_filled;
   always @ (posedge clk)
     if (~sys_en) begin
        dac_fifo_underflow <= 1'b0;
        dac_fifo_filled <= 1'b0;
        dac_fifo_overflow <= 1'b0;
     end
     else begin
        if (~dac_fifo_empty)
          dac_fifo_filled <= 1'b1;
        if (dac_fifo_filled & dac_fifo_empty & (tx_i_en | tx_q_en))
          dac_fifo_underflow <= 1'b1;
        if (dac_fifo_full & (tx_i_en | tx_q_en))
          dac_fifo_overflow <= 1'b1;
     end
      

   always @ (posedge clk)
     if (~sys_en)
       adc_fifo_overflow <= 1'b0;
     else if (adc_fifo_full & (rx_q_en | rx_i_en) )
       adc_fifo_overflow <= 1'b1;


   /* sampling dac fifo level at the rising edge of usb_sof */
   /* sof will come as a 125 ms pulse sync to USB SOF */
   reg          usb_sof_q;
   reg          _usb_sof;
   reg [31:0]   clk_counter;
   reg [2:0]    sec_counter;
   
   always @ (posedge clk) begin
      usb_sof_q <= usb_sof;
      _usb_sof <= usb_sof_q;
   end
   assign sof  =  usb_sof_q & ~_usb_sof;

   always @ (posedge clk)
      if (sof) begin
         adc_fifo_level_q <= adc_fifo_level;
         dac_fifo_level_q <= dac_fifo_level;
      end

   always @ (posedge clk)
     if (sof)
       sec_counter <= sec_counter + 1;

   assign sec_pulse = (sec_counter == 0);
   
   always @ (posedge clk)
     if (sof & sec_pulse) begin
        sync_word <= clk_counter;
        clk_counter <= 0;
     end
     else
       clk_counter <= clk_counter + 1;
   
       
   
endmodule // top
