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



/* 
 
 SPI Write data format: 
    CMD + DATA (1/2)
 
 CMD:  bit 7  : rd/write
       bit 6-5: register addr
       bit 4-0: N/A
 
 reg_0:  system control 
         bit 7-5  : rsv
         bit 4  : tx_q_en
         bit 3  : tx_i_en
         bit 2  : rx_q_en
         bit 1  : rx_i_en
         bit 0  : sys_en

 
 reg_1:  clock divider 
         bit6-0 : cdiv
 
 reg_2:  gpio_high
         bit7-0 : gpio15-gpio8
 
 reg_3:  gpio_low
         bit7-0 : gpio7-gpio0

 read data: 

 reg_0:
         bit 15-14:    rsv
         bit 13-8:     adc_fifo_level
 
         bit 7-6:      rsv
         bit 5-0:      dac_fifo_level
 
 reg_1:
         bit15-0    sync_word_high
 
 reg_2:   
         bit15-0    sync_word_low
 
 reg_3: 
         bit 15:    rsv
         bit 14-8:  clk_div
  
         bit 7-5  : rsv
         bit 4  : tx_q_en
         bit 3  : tx_i_en
         bit 2  : rx_q_en
         bit 1  : rx_i_en
         bit 0  : sys_en
 
 */

module spi_if(rst_n, spi_clk_i, spi_ncs_i, spi_mosi, spi_miso, cdiv, ctrl,sys_en, dac_fifo_level, adc_fifo_level, sync_word, gpio);


   input                        rst_n;
   input                        spi_clk_i;
   input                        spi_ncs_i;
   input                        spi_mosi;
   output reg                   spi_miso;
   /* system control */
   output [6:0]                 cdiv;
   output [3:0]                 ctrl;
   output                       sys_en;
   /* fifo status */
   input [5:0]                  dac_fifo_level;
   input [5:0]                  adc_fifo_level;
   input [31:0]                 sync_word;
   /* GPIO */
   output [15:0]                gpio;
   
   
   
   reg [4:0]                     spi_counter;
   reg [3:0]                     ctrl_reg;
   reg [6:0]                     cdiv_reg;
   reg                           sys_en_reg;
   reg                           spi_rdn_wr;
   reg [7:0]                     gpio_reg0;
   reg [7:0]                     gpio_reg1;
   reg [1:0]                     sub_addr;

   reg [5:0]                     dac_level_q;
   reg [5:0]                     adc_level_q;
   


   assign gpio[15:8] = gpio_reg1;
   assign gpio[7:0]  = gpio_reg0;
   
    //master spi input and output stage
   always @ (posedge spi_clk_i, posedge spi_ncs_i) begin
      if(spi_ncs_i) 
        spi_counter <= 0;
      else
        spi_counter <= spi_counter + 1'b1;
   end

   always @ (posedge spi_clk_i, posedge spi_ncs_i)
     if(spi_ncs_i)
       spi_rdn_wr <= 1'b0;
     else if (spi_counter == 0)
       spi_rdn_wr <= spi_mosi;


   always @ (posedge spi_clk_i, posedge spi_ncs_i)
     if (spi_ncs_i)
        sub_addr <= 2'b00;
     else       
       case (spi_counter)
         5'b00001:  sub_addr[1] <= spi_mosi;
         5'b00010:  sub_addr[0] <= spi_mosi;
       endcase // case (spi_counter)

   always @ (posedge spi_clk_i, negedge rst_n)
     if (~rst_n) begin
        sys_en_reg <= 0;
        ctrl_reg <= 4'b0000;
        cdiv_reg <= 7'b0000000;
        gpio_reg0 <= 8'h00;
        gpio_reg1 <= 8'h00;
     end
     else if(~spi_ncs_i & spi_rdn_wr)
       case (spi_counter)
         /* second byte */
         5'b01000:
              case (sub_addr)
              2'b10: gpio_reg1[7] <= spi_mosi;
              2'b11: gpio_reg0[7] <= spi_mosi;
              endcase // case (sub_addr)
         5'b01001: 
              case (sub_addr)
              2'b01: cdiv_reg[6] <= spi_mosi;
              2'b10: gpio_reg1[6] <= spi_mosi;
              2'b11: gpio_reg0[6] <= spi_mosi;
              endcase // case (sub_addr)
         5'b01010:
              case (sub_addr)
              2'b01: cdiv_reg[5] <= spi_mosi;
              2'b10: gpio_reg1[5] <= spi_mosi;
              2'b11: gpio_reg0[5] <= spi_mosi;
              endcase // case (sub_addr)
         5'b01011:
              case (sub_addr)
              2'b00: ctrl_reg[3] <= spi_mosi;                
              2'b01: cdiv_reg[4] <= spi_mosi;
              2'b10: gpio_reg1[4] <= spi_mosi;
              2'b11: gpio_reg0[4] <= spi_mosi;
              endcase // case (sub_addr)
         5'b01100:
              case (sub_addr)
              2'b00: ctrl_reg[2] <= spi_mosi;              
              2'b01: cdiv_reg[3] <= spi_mosi;
              2'b10: gpio_reg1[3] <= spi_mosi;
              2'b11: gpio_reg0[3] <= spi_mosi;
              endcase // case (sub_addr)
         5'b01101:
              case (sub_addr)
              2'b00: ctrl_reg[1] <= spi_mosi;
              2'b01: cdiv_reg[2] <= spi_mosi;
              2'b10: gpio_reg1[2] <= spi_mosi;
              2'b11: gpio_reg0[2] <= spi_mosi;
              endcase // case (sub_addr)
         5'b01110:
              case (sub_addr)
              2'b00: ctrl_reg[0] <= spi_mosi;              
              2'b01: cdiv_reg[1] <= spi_mosi;
              2'b10: gpio_reg1[1] <= spi_mosi;
              2'b11: gpio_reg0[1] <= spi_mosi;
              endcase // case (sub_addr)
              
         5'b01111:
              case (sub_addr)
              2'b00: sys_en_reg <= spi_mosi;
              2'b01: cdiv_reg[0] <= spi_mosi;
              2'b10: gpio_reg1[0] <= spi_mosi;
              2'b11: gpio_reg0[0] <= spi_mosi;
              endcase // case (sub_addr)
       endcase // case (spi_counter)

   
   always @ (negedge spi_clk_i)
     if (~spi_ncs_i)
       if(spi_counter == 0) begin
          dac_level_q <= dac_fifo_level;
          adc_level_q <= adc_fifo_level;
       end
   
   always @ (negedge spi_clk_i)
     if(~spi_ncs_i)
       case (spi_counter)
         5'b01000:  
           case (sub_addr)
             2'b00: spi_miso <= 0;
             2'b01: spi_miso <= sync_word[31];
             2'b10: spi_miso <= sync_word[15];
             2'b11: spi_miso <= 0;
           endcase // case (sub_addr)
         5'b01001:
           case (sub_addr)
             2'b00: spi_miso <= 0;
             2'b01: spi_miso <= sync_word[30];
             2'b10: spi_miso <= sync_word[14];
             2'b11: spi_miso <= cdiv_reg[6];
           endcase // case (sub_addr)
         5'b01010: 
           case (sub_addr)
             2'b00: spi_miso <= adc_level_q[5];
             2'b01: spi_miso <= sync_word[29];
             2'b10: spi_miso <= sync_word[13];
             2'b11: spi_miso <= cdiv_reg[5];             
           endcase // case (sub_addr)
         5'b01011:  
           case (sub_addr)
             2'b00: spi_miso <= adc_level_q[4];
             2'b01: spi_miso <= sync_word[28];
             2'b10: spi_miso <= sync_word[12];
             2'b11: spi_miso <= cdiv_reg[4];             
           endcase // case (sub_addr)
         5'b01100:  
           case (sub_addr)
             2'b00: spi_miso <= adc_level_q[3];
             2'b01: spi_miso <= sync_word[27];
             2'b10: spi_miso <= sync_word[11];
             2'b11: spi_miso <= cdiv_reg[3];                          
           endcase // case (sub_addr)
         5'b01101:  
           case (sub_addr)
             2'b00: spi_miso <= adc_level_q[2];
             2'b01: spi_miso <= sync_word[26];
             2'b10: spi_miso <= sync_word[10];
             2'b11: spi_miso <= cdiv_reg[2];                                       
           endcase // case (sub_addr)
         5'b01110:  
           case (sub_addr)
             2'b00: spi_miso <= adc_level_q[1];
             2'b01: spi_miso <= sync_word[25];
             2'b10: spi_miso <= sync_word[9];
             2'b11: spi_miso <= cdiv_reg[1];                                                    
           endcase // case (sub_addr)
         5'b01111:  
           case (sub_addr)
             2'b00: spi_miso <= adc_level_q[0];
             2'b01: spi_miso <= sync_word[24];
             2'b10: spi_miso <= sync_word[8];
             2'b11: spi_miso <= cdiv_reg[0];                                                                 
           endcase // case (sub_addr)
         5'b10000:  
           case (sub_addr)
             2'b00: spi_miso <= 0;
             2'b01: spi_miso <= sync_word[23];
             2'b10: spi_miso <= sync_word[7];
             2'b11: spi_miso <= 0; 
           endcase // case (sub_addr)
         5'b10001:  
         case (sub_addr)
             2'b00: spi_miso <= 0;
             2'b01: spi_miso <= sync_word[22];
             2'b10: spi_miso <= sync_word[6];
             2'b11: spi_miso <= 0;            
           endcase // case (sub_addr)
         5'b10010:  
           case (sub_addr)
             2'b00: spi_miso <= dac_level_q[5];
             2'b01: spi_miso <= sync_word[21];
             2'b10: spi_miso <= sync_word[5];
             2'b11: spi_miso <= 0;                         
           endcase // case (sub_addr)
         5'b10011:  
           case (sub_addr)
             2'b00: spi_miso <= dac_level_q[4];
             2'b01: spi_miso <= sync_word[20];
             2'b10: spi_miso <= sync_word[4];
             2'b11: spi_miso <= ctrl_reg[3];
           endcase // case (sub_addr)
         5'b10100:  
           case (sub_addr)
             2'b00: spi_miso <= dac_level_q[3];
             2'b01: spi_miso <= sync_word[19];
             2'b10: spi_miso <= sync_word[3];
             2'b11: spi_miso <= ctrl_reg[2];             
           endcase // case (sub_addr)
         5'b10101:  
           case (sub_addr)
             2'b00: spi_miso <= dac_level_q[2];
             2'b01: spi_miso <= sync_word[18];
             2'b10: spi_miso <= sync_word[2];
             2'b11: spi_miso <= ctrl_reg[1];                          
           endcase // case (sub_addr)
         5'b10110:  
           case (sub_addr)
             2'b00: spi_miso <= dac_level_q[1];
             2'b01: spi_miso <= sync_word[17];
             2'b10: spi_miso <= sync_word[1];
             2'b11: spi_miso <= ctrl_reg[0];                                       
           endcase // case (sub_addr)
         5'b10111:  
           case (sub_addr)
             2'b00: spi_miso <= dac_level_q[0];
             2'b01: spi_miso <= sync_word[16];
             2'b10: spi_miso <= sync_word[0];
             2'b11: spi_miso <= sys_en_reg;
           endcase // case (sub_addr)
       endcase // case (spi_counter)
     else
       spi_miso <= 1'bz;
   
   /* cmd[7:4] is encoded tx/rx/i/q, cmd[3:0] is cdiv */
   assign ctrl = ctrl_reg[3:0];
   assign cdiv = cdiv_reg[6:0];
   assign sys_en = sys_en_reg;
  

endmodule // spi_if

    
                     
          
