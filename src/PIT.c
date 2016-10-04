#include <ioport.h>
#include "PIT.h"

void PIT_setup() {
	uint16_t Freq_out = 0;
	out8(0x43, 0x34);/*00110010 ch 00 div 11 mode 010 BCD 0*/
	out8(0x40, Freq_out & 0xFF);
	out8(0x40, Freq_out >> 8);
	/*unmask PIC interrupt from PIT*/
	out8(0x21, 0xFE);
}
