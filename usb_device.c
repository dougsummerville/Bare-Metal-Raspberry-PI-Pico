#include <MKL25Z4.h>
#include <stdbool.h>
#include "usb_device.h"

#define DISABLE_ERROR_IRQ
#define DISABLE_STALL_IRQ
#define DISABLE_SLEEP_IRQ

#define min(A,B) ( ((A)<(B))?(A):(B))

#define BD_BC_SHIFT                     16
#define BD_OWN_MASK                     (1 << 7)
#define BD_DATA1_MASK                   (1 << 6)
#define BD_KEEP_MASK                    (1 << 5)
#define BD_NINC_MASK                    (1 << 4)
#define BD_DTS_MASK                     (1 << 3)
#define BD_STALL_MASK                   (1 << 2)

uint8_t application_data[HID_REPORT_SIZE];
_Bool there_is_application_data_to_send;

static const uint8_t language_string_descriptor[4] =
{ 0x04, 0x03, 0x09, 0x04 };// US English

static const uint8_t manufacturer_string_descriptor[18] =
{   sizeof(manufacturer_string_descriptor),0x03,
    'B',0,
    'U',0,
    ' ',0,
    'E',0,
    'E',0,
    'C',0,
    'E',0,
    '.',0
};
static const uint8_t configuration_string_descriptor[18] =
{   sizeof(configuration_string_descriptor),0x03,
    'C',0,
    'O',0,
    'N',0,
    'F',0,
    'i',0,
    'G',0,
    '?',0,
    '.',0
};

#define NUM_STRING_DESCRIPTORS 3
const uint8_t *string_table[NUM_STRING_DESCRIPTORS] = {
    language_string_descriptor,
    manufacturer_string_descriptor,
    configuration_string_descriptor
};
const uint8_t string_table_sizes[NUM_STRING_DESCRIPTORS]= {
    sizeof(language_string_descriptor),
    sizeof(manufacturer_string_descriptor),
    sizeof(configuration_string_descriptor)
};

/***********HID REPORT***********/
static const uint8_t report_descriptor[23] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x95, 0x01,       //     Report Count (1)
    0x75, 0x08,       //     Report Size (8)
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x65,       //     Logical Maximum (101)
    0x05, 0x07,       //     Usage Page (Key codes)
    0x19, 0x00,       //     Usage Minimum (0)
    0x29, 0x65,       //     Usage Maximum (101)
    0x81, 0x00,       //     Input (Data, Array) Key array(1 byte)
    0xC0              // End Collection (Application)
};
static const uint8_t device_descriptor[18] = {
    sizeof(device_descriptor), // Length of this descriptor, in Bytes
    0x01, // Descriptor type = 1 findicating device descriptor
    0x10, // USB version (in BCD, lsB)
    0x01, // USB version (in BCD, msB)
    0x00, // Device class (00: class info in interface descriptors,
    //   0xFF: vendor specified (any use))
    0x00, // Device subclass
    0x00, // Device Protocol
    MAX_PACKET_SIZE, // Maximum packet size target can process
    0xad, // Vendor ID (lsB)
    0xde, // Vendor ID (msB)
    0xef, // Product ID (lsb)
    0xbe, // Product ID (msb)
    0x01, // Device Version (lsB)
    0x00, // Device Version (msB)
    0x01, // Index of Manufacturer string descriptor
    0x00, // Index of Device string descriptor
    0x00, // Index of Serial Number string descriptor
    0x01, // Number of configurations
};

static const uint8_t configuration_and_interface_descriptors[34] = {
    0x09, // Size of configuration descriptor (first 9 bytes)
    0x02, // Descriptor Type 2 (config. desc.)
    sizeof(configuration_and_interface_descriptors), // Total Length of descriptors (lsB)
    0x00, // Total Length of descriptors (msB)
    0x01, // Number of interfaces
    0x01, // Which configuration this describes
    0x02, // configuration string index
    0x80, // bmAttributes
    0xfa, // bMaxPower

    0x09, // bLength (*** Interface ***
    0x04, // bDescriptorType
    0x00, // iInterfaceNumber
    0x00, // bAlternateSetting
    0x01, // bNumEndpoints
    0x03, // bInterfaceClass
    0x00, // bInterfaceSubClass  was 01
    0x01, // bInterfaceProtocol
    0x00, // iInterface

    0x09, // bLength (*** HID-Descriptor ***
    0x21, // bDescriptorType
    0x10, // bcdHid (lo)
    0x01, // bcdHid (hi)
    0x00, // bCountryCode
    0x01, // bNumDescriptors
    0x22, // bDescriptorType
    sizeof(report_descriptor), // wItemLength (lo)
    0x00, // wItemLength (hi)

    0x07, // bLength (*** Endpoint ***)
    0x05, // bDescriptorType
    0x81, // bEndpointAddr
    0x03, // bmAttributes
    HID_REPORT_SIZE, // wMaxPacketSize (lo)
    0x00, // wMaxPacketSize (hi)
    0xFF, // bInterval
};

struct Endpoint_State {
    uint8_t pingpong;
    uint8_t data01;
} endpoint_state[2];

typedef struct {
    volatile uint32_t desc;
    volatile void* volatile addr;
} buffer_descriptor_t;

static volatile uint8_t endpoint_0_rx_buf[2][MAX_PACKET_SIZE];
static volatile uint8_t endpoint_1_rx_buf[2][MAX_PACKET_SIZE];

__attribute((aligned(512))) static buffer_descriptor_t buf_desc_table[8] = {
    {0,endpoint_0_rx_buf[0]},{0,endpoint_0_rx_buf[1]},
    {0,0},{0,0},
    {0,endpoint_1_rx_buf[0]},{0,endpoint_1_rx_buf[1]},
    {0,0},{0,0}
};

void initialize_usb(void) {
    //1: Select clock source
    SIM->SOPT2 |= SIM_SOPT2_USBSRC_MASK | SIM_SOPT2_PLLFLLSEL_MASK;

    //2: Gate USB clock
    SIM->SCGC4 |= SIM_SCGC4_USBOTG_MASK;

    //3: Software USB module reset
    USB0->USBTRC0 |= USB_USBTRC0_USBRESET_MASK;
    while (USB0->USBTRC0 & USB_USBTRC0_USBRESET_MASK);

    //4: Set BDT base registers
    USB0->BDTPAGE1 = ((uint32_t) buf_desc_table) >> 8;  //bits 15-9
    USB0->BDTPAGE2 = ((uint32_t) buf_desc_table) >> 16; //bits 23-16
    USB0->BDTPAGE3 = ((uint32_t) buf_desc_table) >> 24; //bits 31-24

    //Clear flags nd enable weak pull downs
    //USB0->ISTAT = 0xFF;
//#ifndef DISABLE_ERROR_IRQ
    //USB0->ERRSTAT = 0xFF;
//#endif
    //USB0->OTGISTAT = 0xFF;
    //USB0->USBTRC0 |= 0x40;

    //enable USB module
    USB0->CTL = USB_CTL_USBENSOFEN_MASK;
    //take usb module out of suspend and disable weak pull-downs
    USB0->USBCTRL = 0;
    //enable usb reset interrupt (others enabled in IRQ)
    USB0->INTEN |= USB_INTEN_USBRSTEN_MASK;
    NVIC_EnableIRQ(USB0_IRQn);

    //pull-up D+ for Full speed
    USB0->CONTROL = USB_CONTROL_DPPULLUPNONOTG_MASK;
}

static void endpoint_1_handler(uint8_t tok, buffer_descriptor_t* buf_desc) {

    switch (tok) {

    case 0x09:// TOK IN
        break;

    case 0x01://TOK OUT
        break;
    }
}

static void endpoint_0_handler(uint8_t tok, buffer_descriptor_t* buf_desc) {

#define setup_pkt ((uint16_t *) buf_desc->addr)
    static uint16_t my_usb_address=0;
    static uint16_t descriptor_length = 0;
    static uint8_t const* descriptor_ptr = 0;
    uint8_t const* tx_data = 0;
    uint16_t tx_size = 0;
    _Bool send_stall = false;
    _Bool send_zero_length_data_packet = false;

    switch (tok) {
    case 0x0d: //SETUP Token- Begin Control Transfer
        buf_desc_table[2].desc = 0;
        buf_desc_table[3].desc = 0;
        //new token starts data0, we respond data1
        endpoint_state[0].data01 = BD_DATA1_MASK;
        descriptor_length=0;

        switch (setup_pkt[0]) {

        case 0x0500:
            //setting of address occurs on subsequent IN
            //send a zero len data1 to ack
            my_usb_address=setup_pkt[1];
            send_zero_length_data_packet=true;
            break;
        case 0x0900:
            //set config: we have one so just ack
            send_zero_length_data_packet=true;
            break;

        case 0x0680: //get descriptor
            switch( setup_pkt[1] >> 8 )
            {
            case 0x03: //string
                if( (setup_pkt[1] & 0xFF) >= NUM_STRING_DESCRIPTORS )
                {
                    send_stall=true;
                    break;
                }
                descriptor_ptr = (uint8_t *)string_table[setup_pkt[1] & 0xFF];
                descriptor_length = string_table_sizes[setup_pkt[1] & 0xFF];
                break;
            case 0x01: //device descriptor
                descriptor_ptr = device_descriptor;
                descriptor_length = sizeof(device_descriptor);
                break;
            case 0x02: //config descriptor
                descriptor_ptr = configuration_and_interface_descriptors;
                descriptor_length = sizeof(configuration_and_interface_descriptors);
                break;
            default:
                send_stall = true;
            }
            break;
        case 0x0681:
            descriptor_ptr = report_descriptor;
            descriptor_length = sizeof(report_descriptor);
            break;

        default:
            send_stall = true;
        }
        // must unfreeze after every setup packet
        USB0->CTL = USB_CTL_USBENSOFEN_MASK;
        if (send_stall) {
            USB0->ENDPOINT[0].ENDPT = USB_ENDPT_EPSTALL_MASK
                                      | USB_ENDPT_EPRXEN_MASK
                                      | USB_ENDPT_EPTXEN_MASK
                                      | USB_ENDPT_EPHSHK_MASK;
        }
        else {
            //truncate if we have more data than requested
            descriptor_length=min(descriptor_length,setup_pkt[3]);
        }

        break;
    //intentionally fall through to process another chunk

    case 0x09: //IN token
        // setting of address after stall: assumes 0 never assigned?
        if( my_usb_address != 0 )
        {
            USB0->ADDR = my_usb_address;
            my_usb_address=0;
        }
        break;

    case 0x01://out token
        break;

    case 0x05:
        break;
    }
    //Queue up responses to IN packets
    while(descriptor_length || send_zero_length_data_packet )
    {
        send_zero_length_data_packet=false;
        tx_size=min(descriptor_length,MAX_PACKET_SIZE);
        tx_data = descriptor_ptr;

        buffer_descriptor_t* bd = &buf_desc_table[2+endpoint_state[0].pingpong];
        if( bd->desc & BD_OWN_MASK ) //no more free BD
            break;
        bd->addr = (void *)tx_data;
        bd->desc = ((tx_size << BD_BC_SHIFT) | BD_OWN_MASK | endpoint_state[0].data01 | BD_DTS_MASK);
        descriptor_ptr += tx_size;
        descriptor_length -=tx_size;
        endpoint_state[0].data01 ^= BD_DATA1_MASK;
        endpoint_state[0].pingpong ^= 1;
    }
}

void USBOTG_IRQHandler(void) {

    if (USB0->ISTAT & USB_ISTAT_USBRST_MASK) {
        //USB reset
        USB0->CTL |= USB_CTL_ODDRST_MASK;
        //initialize endpoint 0 ping-pong buffers
        endpoint_state[0].pingpong = 0;
        endpoint_state[0].data01 = 0;
        buf_desc_table[0].desc = ((MAX_PACKET_SIZE << BD_BC_SHIFT) | BD_OWN_MASK | BD_DTS_MASK);
        buf_desc_table[1].desc = ((MAX_PACKET_SIZE << BD_BC_SHIFT) | BD_OWN_MASK | BD_DATA1_MASK | BD_DTS_MASK);

        buf_desc_table[2].desc = 0;
        buf_desc_table[3].desc = 0;
        USB0->ENDPOINT[0].ENDPT = USB_ENDPT_EPRXEN_MASK
                                  | USB_ENDPT_EPTXEN_MASK
                                  | USB_ENDPT_EPHSHK_MASK;

        // initialize endpoint 1
        endpoint_state[1].pingpong = 0;
        endpoint_state[1].data01 = 0;
        buf_desc_table[4].desc = ((MAX_PACKET_SIZE << BD_BC_SHIFT) | BD_OWN_MASK | BD_DTS_MASK);
        buf_desc_table[5].desc = ((MAX_PACKET_SIZE << BD_BC_SHIFT) | BD_OWN_MASK | BD_DATA1_MASK | BD_DTS_MASK);
        buf_desc_table[6].desc = 0;
        buf_desc_table[7].desc = 0;
        USB0->ENDPOINT[1].ENDPT = USB_ENDPT_EPRXEN_MASK
                                  | USB_ENDPT_EPTXEN_MASK
                                  | USB_ENDPT_EPHSHK_MASK;

        //clear old pending interrupts
#ifndef DISABLE_ERROR_IRQ
        USB0->ERRSTAT = 0xff;
#endif
        USB0->ISTAT = 0xff;

        //address 0 after reset
        USB0->ADDR = 0;

#ifndef DISABLE_ERROR_IRQ
        USB0->ERREN = 0xFF;
#endif
        USB0->INTEN =     USB_INTEN_USBRSTEN_MASK 
			| USB_INTEN_SOFTOKEN_MASK 
			| USB_INTEN_TOKDNEEN_MASK
#ifndef DISABLE_ERROR_IRQ
			| USB_INTEN_ERROREN_MASK
#endif
#ifndef DISABLE_SLEEP_IRQ
			| USB_INTEN_SLEEPEN_MASK 
#endif
#ifndef DISABLE_STALL_IRQ
			| USB_INTEN_STALLEN_MASK
#endif
	;
        return;
    }

#ifndef DISABLE_ERROR_IRQ
    if (USB0->ISTAT & USB_ISTAT_ERROR_MASK) {
        USB0->ERRSTAT = USB0->ERRSTAT;
        USB0->ISTAT = USB_ISTAT_ERROR_MASK;
    }
#endif

    if (USB0->ISTAT & USB_ISTAT_SOFTOK_MASK) {
        //start of frame
        USB0->ISTAT = USB_ISTAT_SOFTOK_MASK;
        //todo after setup?
	if( there_is_application_data_to_send )
	{
		buffer_descriptor_t* bd = &buf_desc_table[6+endpoint_state[1].pingpong];
		if( (bd->desc & BD_OWN_MASK) == 0 )
		{
			bd->addr = application_data;
			bd->desc = ((HID_REPORT_SIZE << BD_BC_SHIFT) 
				| BD_OWN_MASK 
				| endpoint_state[1].data01 
				| BD_DTS_MASK);
			endpoint_state[1].data01 ^= BD_DATA1_MASK;
			endpoint_state[1].pingpong ^= 1;
			there_is_application_data_to_send=0;
		}
	}
    }
    if (USB0->ISTAT & USB_ISTAT_TOKDNE_MASK) {

        buffer_descriptor_t* buf_desc = &buf_desc_table[(USB0->STAT)>>2];

        // determine which token has been processed
        uint8_t tok = ((buf_desc->desc >> 2) & 0xF);

        if ((USB0->STAT & 0xF0) == 0) {
            endpoint_0_handler(tok, buf_desc);
        } else{ 
            endpoint_1_handler(tok, buf_desc);
        }
	//If this was RX reset descriptor
        if (!(USB0->STAT & USB_STAT_TX_MASK)) {
            buf_desc->desc = ((MAX_PACKET_SIZE << BD_BC_SHIFT) | BD_OWN_MASK | (buf_desc->desc & BD_DATA1_MASK) | BD_DTS_MASK);
        }

        USB0->ISTAT = USB_ISTAT_TOKDNE_MASK;
    }

#ifndef DISABLE_SLEEP_IRQ
    if (USB0->ISTAT & USB_ISTAT_SLEEP_MASK) {
        //handle USB sleep
        USB0->ISTAT = USB_ISTAT_SLEEP_MASK;
    }
#endif

#ifndef DISABLE_STALL_IRQ
    if (USB0->ISTAT & USB_ISTAT_STALL_MASK) {
        //handle usb stall
        USB0->ISTAT = USB_ISTAT_STALL_MASK;
    }
#endif
}
