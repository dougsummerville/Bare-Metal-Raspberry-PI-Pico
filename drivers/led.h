#ifndef LED_H
#define LED_H

#include <rp2040/resets.h>
#include <rp2040/sio.h>
#include <rp2040/io_bank0.h>
#include <rp2040/clocks.h>

void configure_led( void );
void turn_on_led();
void turn_off_led();
void toggle_led();

#endif
