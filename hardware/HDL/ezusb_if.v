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



module ezusb_if
  ( 
    input            clk,
    input            rst,
    //internal fifo interface, rx_fifo is for dac
    input            rx_fifo_fullm1, /* one data to full */
    output           rx_fifo_wr,
    output [7:0]     wdata,
    // tx_fifo is for adc
    input            tx_fifo_emptyp1, /* one data to empty */
    output           tx_fifo_rd,
    input [7:0]      rdata,
  
    //EZUSB sync FIFO interface
    input [7:0]      data_i,
    output [7:0]     data_o,
    output           data_en,
    input            ep2out_emptyp1_n, /* one data to empty */
    input            ep6in_fullm2_n, /* two data to full */
    input            ep6in_full_n,  /* full */   
    output           rd_n,
    output reg       wr_n,
    output           oe_n,
    output reg [1:0] fifoaddr
);
    
   reg           wr_s;
   reg           rd_s;
   reg           oe_s;
   
   reg [2:0]     state;

   assign rd_n     = ~rd_s;
   assign oe_n     = ~oe_s;
   assign data_en  = ~oe_s;
   assign data_o   = rdata;
   assign wdata    = data_i;
   assign tx_fifo_rd = wr_s;
   assign rx_fifo_wr = rd_s;

   /* this is necessary as the internal fifo is a sync read fifo
    * the address is read on the clock, which means 1 clock latency */
   always @ (posedge clk)
     wr_n <= ~wr_s;
   
   localparam  IDLE = 0, ADDR_RD = 1,  READ = 2 , WRITE = 3, ADDR_WR=4;

   wire          ez_pending_write  = (ep6in_full_n && ~tx_fifo_emptyp1);
   wire          ez_cont_write = (ep6in_fullm2_n && ~tx_fifo_emptyp1);
   wire          ez_pending_read   = (ep2out_emptyp1_n && ~rx_fifo_fullm1);
   
   always@(posedge clk, posedge rst)
     if(rst) begin
        state   <=  IDLE;
        fifoaddr <= 2'b00;
     end
     else 
        case(state) 
          IDLE :
            if (ez_pending_read ) begin
               state <= #1 ADDR_RD;
               fifoaddr <= #1 2'b00;
            end
            else if (ez_pending_write) begin
               state <= #1 ADDR_WR;
               fifoaddr <= #1 2'b10;
            end
            else begin
               state <= #1 IDLE;
            end
          ADDR_RD : state <= #1 ez_pending_read ? READ: IDLE;
          ADDR_WR : state <= #1 ez_pending_write ? WRITE: IDLE;
          READ : state <= #1 ez_pending_read ? READ : IDLE;
          WRITE: state <= #1 ez_cont_write ? WRITE : IDLE;
        endcase // case (state)

   always @ (*)
     case (state)
       default: begin
          oe_s = 1'b0;
          rd_s = 1'b0;
          wr_s = 1'b0;
       end
       ADDR_RD: begin
          oe_s = 1'b1;
          rd_s = 1'b0;
          wr_s = 1'b0;
       end
       ADDR_WR: begin
          oe_s = 1'b0;
          rd_s = 1'b0;
          wr_s = 1'b0;
       end
       READ: begin
          oe_s = 1'b1;
          rd_s = 1'b1;
          wr_s = 1'b0;
       end
       WRITE: begin
          oe_s = 1'b0;
          rd_s = 1'b0;
          wr_s = 1'b1;
       end
     endcase // case (state)
endmodule


