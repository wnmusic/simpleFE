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

module frontend(input            clk,
                input            enable,
                /* fifo related */
                input            dac_fifo_rdy,
                output           dac_fifo_read,
                input            adc_fifo_rdy,
                output           adc_fifo_write,
                input [7:0]      dac_fifo_data,
                output [7:0]     adc_fifo_data,
                /* logic control */
                input [6:0]      clk_div,
                input            tx_i_en,
                input            tx_q_en,
                input            rx_i_en,
                input            rx_q_en,
                /* max5863 io*/
                output           max_clk_o,
                output reg [9:0] dac_o,
                input [7:0]      adc_i
                  );
   



   
   reg [6:0]                     idle_cnt;
   reg                           max_clk;
                    
   reg [7:0]                     dac_msb;
   reg [7:0]                     dac_lsb;
   reg [2:0]                     tx_data_cnt;
   reg [7:0]                     adc_latch;
   reg                           dac_fifo_read_d;
   reg                           adc_sample;
   
   

   localparam S0=0, S1=1, SW=2, SRST=3;
   wire                          add_delay;
   wire                          tx_en, rx_en;

   
   
   assign tx_en = tx_i_en | tx_q_en;
   assign rx_en = (rx_i_en | rx_q_en) & enable;
   
   assign add_delay = |clk_div;

`define NEW_DAC_COUNTER
   
`ifdef  NEW_DAC_COUNTER
   
   reg [2:0]                     state;
   reg [2:0]                     frame_cnt;
   
   localparam S_RST = 0, S_MSB=1, S_I=2, S_WI =3, S_Q=4, S_WQ=5;
   assign dac_rdy = ~tx_en | dac_fifo_rdy;
   
   assign dac_fifo_read = dac_fifo_rdy & ((state == S_I & tx_i_en) || (state == S_Q & tx_q_en) || (state == S_MSB & tx_en));
   
   always @ (posedge clk)
     if (~enable) begin
        state <= S_RST;
        max_clk <= 1;
        idle_cnt <= 0;
        frame_cnt <= 0;
     end
     else
       case (state)
         S_RST: state <= dac_rdy ? S_MSB : S_RST;
         S_MSB: begin
            frame_cnt <= 0;            
            state <= dac_rdy ? S_I : S_MSB;
         end
         S_I: begin
            if (tx_i_en & dac_fifo_rdy)
              frame_cnt <= frame_cnt + 1;

            if (dac_rdy) begin
               state <= S_WI;
               max_clk <= 0;
            end
         end
         S_WI: begin
            // see if addition wait is needed 
            if (idle_cnt == clk_div) begin
               state <= S_Q;
               idle_cnt <= 0;
            end
            else
              idle_cnt <= idle_cnt + 1'b1;
         end // case: S_WI

         S_Q: begin
            if (tx_q_en & dac_fifo_rdy)
               frame_cnt <= frame_cnt + 1;
               
            if (dac_rdy) begin
               if ((frame_cnt == 3'b011 & tx_q_en) || (frame_cnt == 3'b100 ))
                 if (add_delay) begin
                    idle_cnt <= 1'b1;
                    state <= S_WQ;
                 end
                 else
                   state <= S_MSB;
               else
                 state <= S_WQ;
               
               max_clk <= 1;
            end
         end // case: S_Q
         S_WQ: begin
            if (idle_cnt == clk_div) begin
               state <= (frame_cnt == 3'b100) ? S_MSB : S_I;               
               idle_cnt <= 0;
            end
            else
              idle_cnt <= idle_cnt + 1'b1;
         end // case: S_WQ
       endcase // case (state)

`else
   reg [1:0]                     state;
   assign dac_msb_rdy = (tx_data_cnt==3'b000)  & dac_fifo_rdy; //
   /* DAC sync to internal clock */
   assign dac_I_rdy = max_clk & dac_fifo_rdy;
   assign dac_Q_rdy = ~max_clk & dac_fifo_rdy;
   
   assign dac_fifo_read = (((tx_i_en & dac_I_rdy) | (tx_q_en & dac_Q_rdy)) & (state == S0)) |
                        ((tx_en & dac_msb_rdy) & (state == S1));

   assign S1_en = (~tx_en | (tx_data_cnt != 3'b000) | dac_fifo_rdy );

   always @ (posedge clk)
     if(~enable) begin
        state <= SRST;
        max_clk <= 0;
        idle_cnt <= 1;
     end
     else
       case (state)
         SRST:
           state <= S1;
         S1: 
            if (S1_en) begin
               state <= S0;
               max_clk <= ~max_clk;
            end
            else
              state <= S1;
         S0: begin
            if (~tx_en | dac_I_rdy | dac_Q_rdy)
              state <=  add_delay ? SW : S1;
         end
         SW:
           if (idle_cnt == clk_div) begin
              state <= S1;
              idle_cnt <= 1;
           end
           else
             idle_cnt <= idle_cnt + 1'b1;
       endcase
`endif
      
   
   always @ (posedge clk)
     if(~enable)
       tx_data_cnt <= 3'b000;
     else if(dac_fifo_read_d) begin
        if (tx_data_cnt == 3'b100)
          tx_data_cnt <= 3'b000;
        else
          tx_data_cnt <= tx_data_cnt + 3'b001;
     end

   reg [7:0] dac_msb_d;
   
   always @ (posedge clk) begin
      dac_fifo_read_d <= dac_fifo_read;
      dac_msb_d <= dac_msb;
      if (dac_fifo_read_d)
        if (tx_data_cnt == 3'b000)
          dac_msb <= dac_fifo_data;
        else
          dac_lsb <= dac_fifo_data;
   end
      
   always @ (*)
     case (tx_data_cnt)
       3'b010: dac_o = #1 {dac_msb_d[1:0], dac_lsb};
       3'b011: dac_o = #1 {dac_msb_d[3:2], dac_lsb};
       3'b100: dac_o = #1 {dac_msb_d[5:4], dac_lsb};
       3'b000, 3'b001: dac_o = #1 {dac_msb_d[7:6], dac_lsb};
       default: dac_o = 0;
     endcase 

   always @ (posedge clk)
     if(max_clk_d2 ^ max_clk_d3)
       adc_latch <= adc_i;
   
   assign adc_fifo_data = adc_latch;

   reg max_clk_d, max_clk_d2, max_clk_d3;
   always @ (posedge clk) begin
      max_clk_d <= max_clk;
      max_clk_d2 <= max_clk_d;
      max_clk_d3 <= max_clk_d2;
   end

   assign max_clk_o = max_clk_d2;


   /* ADC should sync to output clock */
   reg [1:0]  adc_state;
   localparam ADC_RST=0, ADC_I=1, ADC_Q = 2;
   
   assign adc_I_sample = max_clk_d2 & ~max_clk_d3;
   assign adc_Q_sample = ~max_clk_d2 & max_clk_d3;
   assign adc_fifo_write = adc_sample & (((adc_state == ADC_I) & rx_i_en) | ((adc_state == ADC_Q) & rx_q_en));
   
   always @ (posedge clk)
     adc_sample <= adc_I_sample | adc_Q_sample;
   
   always @ (posedge clk)
     if (~rx_en)
       adc_state <= ADC_RST;
     else
       case(adc_state)
         ADC_RST : adc_state <= adc_I_sample ? ADC_I : ADC_RST;
         ADC_I: adc_state <= (adc_Q_sample) ? ADC_Q : ADC_I;
         ADC_Q: adc_state <= (adc_I_sample) ? ADC_I : ADC_Q;
       endcase // case (adc_state)

   
endmodule // max5863_if

  
