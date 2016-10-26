#include <ioport.h>
#include <inttypes.h>
#include <stdio.h>
#include <desc.h>

extern uint64_t table [];

struct idt_entry
{
uint16_t base_low;
uint16_t CS;
uint8_t zeros;
uint8_t flags;
uint16_t base_high;
uint32_t base_64;
uint32_t zeros2;
}__attribute__((packed));

struct idt_entry desc_table[256];
unsigned long long createMask(unsigned short a, unsigned short b)
{
   unsigned long long r = 0;
   for (unsigned short i=a; i<=b; i++)
       r |= 1 << i;

   return r;
}

void create_entry(unsigned char i) {
	desc_table[i].CS = 0x08;
	desc_table[i].flags = 142; /*10001110 0x8E*/
	desc_table[i].base_low = table[i];
	desc_table[i].base_high = (table[i] & (createMask(16, 31)) ) >> 16;
	desc_table[i].base_64 = (table[i] & (createMask(32, 63))) >> 32;
}

void idtsetup (void) {
	for (int i = 0; i < 256; i ++) {
		create_entry(i);
	}
	struct desc_table_ptr ptr = {sizeof (desc_table) - 1, (uintptr_t) &desc_table};
	write_idtr(&ptr); 
}


