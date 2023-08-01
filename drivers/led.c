#include <rp2040/reset.h>
#include <rp2040/sio.h>
#include <rp2040/io_bank0.h>
#include <rp2040/clocks.h>

#include <stdint.h>


void configure_led( void )
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
}
void turn_on_led()
{
    sio -> gpio_out_set = (1<<25);
}
void turn_off_led()
{
    sio -> gpio_out_clr = (1<<25);
}
void toggle_led()
{
    sio -> gpio_out_xor = (1<<25);
}
