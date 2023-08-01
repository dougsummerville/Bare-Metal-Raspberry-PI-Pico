#include "usbcdc.h"

int main()
{
	char c;
	configure_usbcdc();
	while(1)
	{
		if( usbcdc_getchar(&c))
			usbcdc_putchar(c);
	}
	return 0;
}
