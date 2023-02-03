#include <rp2040/resets.h>
#include <rp2040/sio.h>
#include <rp2040/io_bank0.h>
#include <rp2040/uart.h>
#include <rp2040/clocks.h>

#include <stdint.h>


int main ( void )
{
    resets -> clr_reset = RESETS_RESET_IO_BANK0_MASK;
    while(! (resets -> reset_done & RESETS_RESET_IO_BANK0_MASK))
    	continue;
    //TODO GPIO clock at reset is clk_sys
    io_bank0 -> gpio25_ctrl = 
	IO_BANK0_GPIO25_CTRL_IRQOVER(0) |
	IO_BANK0_GPIO25_CTRL_INOVER(0)  |
	IO_BANK0_GPIO25_CTRL_OEOVER(0)  |
	IO_BANK0_GPIO25_CTRL_OUTOVER(0) |
	IO_BANK0_GPIO25_CTRL_FUNCSEL(5);
    sio -> gpio_oe_set = (1<<25);

    //UART
    /*
    resets -> clr_reset = RESETS_RESET_UART0_MASK;
    while(! (resets -> reset_done & RESETS_RESET_UART0_MASK))
    clocks -> set_clk_peri_ctrl = CLOCKS_CLK_PERI_CTRL_ENABLE_MASK;
    uart0 -> uartibrd = 67;
    uart0 -> uartfbrd = 52;
    uart0 -> uartlcr_h = UART_UARTLCR_H_WLEN(3);
    uart0 -> uartcr = 
	     UART_UARTCR_RXE_MASK
	    |UART_UARTCR_TXE_MASK
	    |UART_UARTCR_UARTEN_MASK;
    uart0 -> uartdr='0';
    */
    long unsigned i;
    sio -> gpio_out_set = (1<<25);
    while(1)
    {
	    for( i=0; i<1000000; i++)
		    ;
	sio->gpio_out_xor = (1<<25);
    }
    return(0);
}
