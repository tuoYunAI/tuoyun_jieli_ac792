#include "generic/log.h"
#include "device/gpio.h"

static const u16 gpio_uuid_table[][2] = {
    { IO_PORTA_00, 0xc2e1 },
    { IO_PORTA_01, 0xc2e2 },
    { IO_PORTA_02, 0xc2e3 },
    { IO_PORTA_03, 0xc2e4 },
    { IO_PORTA_04, 0xc2e5 },
    { IO_PORTA_05, 0xc2e6 },
    { IO_PORTA_06, 0xc2e7 },
    { IO_PORTA_07, 0xc2e8 },
    { IO_PORTA_08, 0xc2e9 },
    { IO_PORTA_09, 0xc2ea },
    { IO_PORTA_10, 0xc302 },
    { IO_PORTA_11, 0xc303 },
    { IO_PORTA_12, 0xc304 },
    { IO_PORTA_13, 0xc305 },
    { IO_PORTA_14, 0xc306 },
    { IO_PORTA_15, 0xc307 },

    { IO_PORTB_00, 0x4f42 },
    { IO_PORTB_01, 0x4f43 },
    { IO_PORTB_02, 0x4f44 },
    { IO_PORTB_03, 0x4f45 },
    { IO_PORTB_04, 0x4f46 },
    { IO_PORTB_05, 0x4f47 },
    { IO_PORTB_06, 0x4f48 },
    { IO_PORTB_07, 0x4f49 },
    { IO_PORTB_08, 0x4f4a },
    { IO_PORTB_09, 0x4f4b },
    { IO_PORTB_10, 0x4f63 },
    { IO_PORTB_11, 0x4f64 },
    { IO_PORTB_12, 0x4f65 },
    { IO_PORTB_13, 0x4f66 },
    { IO_PORTB_14, 0x4f67 },
    { IO_PORTB_15, 0x4f68 },

    { IO_PORTC_00, 0xdba3 },
    { IO_PORTC_01, 0xdba4 },
    { IO_PORTC_02, 0xdba5 },
    { IO_PORTC_03, 0xdba6 },
    { IO_PORTC_04, 0xdba7 },
    { IO_PORTC_05, 0xdba8 },
    { IO_PORTC_06, 0xdba9 },
    { IO_PORTC_07, 0xdbaa },
    { IO_PORTC_08, 0xdbab },
    { IO_PORTC_09, 0xdbac },
    { IO_PORTC_10, 0xdbc4 },
    { IO_PORTC_11, 0xdbc5 },
    { IO_PORTC_12, 0xdbc6 },
    { IO_PORTC_13, 0xdbc7 },
#ifdef IO_PORTC_14
    { IO_PORTC_14, 0xdbc8 },
#endif
#ifdef IO_PORTC_15
    { IO_PORTC_15, 0xdbc9 },
#endif

    { IO_PORTD_00, 0x6804 },
    { IO_PORTD_01, 0x6805 },
    { IO_PORTD_02, 0x6806 },
    { IO_PORTD_03, 0x6807 },
    { IO_PORTD_04, 0x6808 },
    { IO_PORTD_05, 0x6809 },
    { IO_PORTD_06, 0x680a },
    { IO_PORTD_07, 0x680b },
    { IO_PORTD_08, 0x680c },
    { IO_PORTD_09, 0x680d },
    { IO_PORTD_10, 0x6825 },
    { IO_PORTD_11, 0x6826 },
    { IO_PORTD_12, 0x6827 },
    { IO_PORTD_13, 0x6828 },
    { IO_PORTD_14, 0x6829 },
    { IO_PORTD_15, 0x682a },

    { IO_PORTE_00, 0xf465 },
    { IO_PORTE_01, 0xf466 },
    { IO_PORTE_02, 0xf467 },
    { IO_PORTE_03, 0xf468 },
    { IO_PORTE_04, 0xf469 },
    { IO_PORTE_05, 0xf46a },
    { IO_PORTE_06, 0xf46b },
    { IO_PORTE_07, 0xf46c },
    { IO_PORTE_08, 0xf46d },
    { IO_PORTE_09, 0xf46e },
    { IO_PORTE_10, 0xf486 },
    { IO_PORTE_11, 0xf487 },
    { IO_PORTE_12, 0xf488 },
    { IO_PORTE_13, 0xf489 },
    { IO_PORTE_14, 0xf48a },
    { IO_PORTE_15, 0xf48b },

    { IO_PORTF_00, 0x80c6 },
    { IO_PORTF_01, 0x80c7 },
    { IO_PORTF_02, 0x80c8 },
    { IO_PORTF_03, 0x80c9 },
    { IO_PORTF_04, 0x80ca },
    { IO_PORTF_05, 0x80cb },

    { IO_PORT_USB_DPA, 0x691e },
    { IO_PORT_USB_DMA, 0x68bb },
    { IO_PORT_USB_DPB, 0x691f },
    { IO_PORT_USB_DMB, 0x68bc },

    { IO_PORT_PR_00, 0xc5a1 },
    { IO_PORT_PR_01, 0xc5a2 },

    { IO_PORT_PV_00, 0xf725 },
    { IO_PORT_PV_01, 0xf726 },
};

u8 uuid2gpio(u16 uuid)
{
    for (int i = 0; i < ARRAY_SIZE(gpio_uuid_table); i++) {
        if (gpio_uuid_table[i][1] == uuid) {
            return gpio_uuid_table[i][0];
        }
    }
    return 0xff;
}
