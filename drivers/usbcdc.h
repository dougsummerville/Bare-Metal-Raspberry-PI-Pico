#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdbool.h>

void configure_usbcdc();
_Bool usbcdc_getchar( char *c);
_Bool usbcdc_putchar( char c);

#endif
