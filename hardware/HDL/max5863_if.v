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

module max5863_if(input            clk,
                  input            enable,
                  /* fifo related */
                  input            tx_fifo_rdy,
                  output           tx_fifo_rd,
                  input            rx_fifo_rdy,
                  output reg       rx_fifo_wr,
                  input [7:0]      tx_data,
                  output reg [7:0] rx_data,
                  /* max5863 stuff */
                  input            tx_i_en,
                  input            tx_q_en,
                  input            rx_i_en,
                  input            rx_q_en,
                  output           max_clk_o,
                  output reg [9:0] dac_o,
                  input [7:0]      adc_i
                  );
   

   reg [2:0]                       phase;
   reg [1:0]                       tx_bytes;
   reg                             rx_bytes;
   reg                             max_clk;

   reg [7:0]                       dac_msb;
   reg [7:0]                       dac_lsb;
   
                                   
   always @ (posedge clk)
     if (~enable)
       phase <= 0;
     else if (phase == 5)
       phase <= 0;
     else
       phase <= phase + 1'b1;

   always @ (posedge clk)
     if (~enable)
       max_clk <= 0;
     else if(phase == 0)
       max_clk <= ~max_clk;
   
   reg                             max_clk_d;
   always @ (posedge clk) begin
      max_clk_d <= max_clk;
   end
   assign max_clk_o = max_clk_d;
   
      
   assign tx_i_pulse = tx_i_en & (max_clk_d & ~max_clk);
   assign tx_q_pulse = tx_q_en & (~max_clk_d & max_clk);
   assign tx_byte_inc = (tx_i_pulse | tx_q_pulse);
   
   always @ (posedge clk)
     if (~enable)
       tx_bytes <= 2'b11;
     else if (tx_byte_inc)
       tx_bytes <= tx_bytes + 2'b01;
   
   reg [2:0] rd_cnt;
   reg       tx_en;
   
   always @ (posedge clk)
     if (tx_bytes == 2'b11)
       tx_en <= tx_fifo_rdy;
   
   always @ (posedge clk)
     if (tx_bytes == 2'b11)
        rd_cnt <= 0;
     else if (tx_fifo_rd)
       rd_cnt <= rd_cnt + 1;
   
   assign tx_fifo_rd = (tx_bytes != 2'b11) & tx_en & (rd_cnt < 5);
   
   reg tx_data_valid;
   reg [2:0] valid_cnt;
   reg [7:0] dac0_lsb;
   reg [7:0] dac1_lsb;
   reg [7:0] dac2_lsb;
   reg [7:0] dac3_lsb;
   
   
   always @ (posedge clk) begin
      tx_data_valid <= tx_fifo_rd;
      if (tx_data_valid)
        valid_cnt <= valid_cnt + 1;
      else
        valid_cnt <= 0;
   end
   
   always @ (posedge clk)
     if (tx_data_valid)
       case (valid_cnt)
         0:        dac_msb  <= tx_data;
         1:        dac0_lsb <= tx_data;
         2:        dac1_lsb <= tx_data;
         3:        dac2_lsb <= tx_data;
         4:        dac3_lsb <= tx_data;
       endcase // case (valid_cnt)


   always @ (posedge clk)
       case (tx_bytes)
         2'b00: dac_o <= {dac_msb[1:0], dac0_lsb};
         2'b01: dac_o <= {dac_msb[3:2], dac1_lsb};
         2'b10: dac_o <= {dac_msb[5:4], dac2_lsb};
         2'b11: dac_o <= {dac_msb[7:6], dac3_lsb};
       endcase // case (tx_bytes)


   reg max_clk_o_d;
   always @ (posedge clk)
     max_clk_o_d <= max_clk_o;
   
   assign rx_i_pulse = rx_i_en & (~max_clk_o_d & max_clk_o);
   assign rx_q_pulse = rx_q_en & (max_clk_o_d & ~max_clk_o);

   /* give 2 clocks for adc data to be latched */
   reg latch_adc;
   always @ (posedge clk)
     if (rx_fifo_rdy & (rx_i_pulse | rx_q_pulse))
       latch_adc <= 1;
     else
       latch_adc <= 0;
   
   always @ (posedge clk)
     if (latch_adc) begin
        rx_fifo_wr <= 1;
        rx_data <= adc_i;
     end
     else begin
        rx_fifo_wr <= 0;
     end // else: !if(tx_data_valid)

endmodule // max5863_if

  
