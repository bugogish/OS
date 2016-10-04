#include <stdint.h>
#include <ioport.h>
#define UNUSED(x) (void)(x)

struct frame {
uint64_t r15;
uint64_t r14;
uint64_t r13;
uint64_t r12;
uint64_t r11;
uint64_t r10;
uint64_t r9;
uint64_t rsi;
uint64_t rdi;
uint64_t rbp;
uint64_t rdx;
uint64_t rcx;
uint64_t rbx;
uint64_t rax;
uint64_t intno;
uint64_t error; 
uint64_t rip;
uint64_t cs;
uint64_t rflags;
uint64_t rsp;
uint64_t ss; 
}__attribute__((packed));

char* interrupts[] = {
"Integer Divide-by-Zero Exception",
"Debug Exception",
"Non-Maskable-Interrupt",
"Breakpoint Exception (INT 3)",
"Overflow Exception (INTO instruction)",
"Bound-Range Exception (BOUND instruction)",
"Invalid-Opcode Exception",
"Device-Not-Available Exception",
"Double-Fault Exception",
"Reserved9",
"Invalid-TSS Exception",
"Segment-Not-Present Exception",
"Stack Exception",
"General-Protection Exception",
"Page-Fault Exception",
"(Reserved)",
"x87 Floating-Point Exception",
"Alignment-Check Exception",
"Machine-Check Exception",
"SIMD Floating-Point Exception"
};

void PIC_handler(struct frame* frame_state) {
	if (frame_state->intno == 32) {
		send_info("PIT interrupt!\n");
}
		
	const uint8_t command = 0x60;
	if (frame_state->intno < 40) {
		out8(0x20, command + frame_state->intno - 32);
}
	else {
		out8(0x20, command + 2);/*00000000*/
		out8(0xA0, command + frame_state->intno - 40);
}
	
}



void handle(struct frame* frame_state) {
if (frame_state->intno >= 32) {
		PIC_handler(frame_state);
}
else {
	if (frame_state->intno  <= 19) {
		send_info(interrupts[frame_state->intno]);
		send_info(" occured!");
	}
	else
		send_info("ERROR");
	if (frame_state->error != 0) {
		send_info("\nThe error code is ");
		send_int_info(frame_state->error);
		send_info("\n");
	}
	while (1);
}
	
	return;
}

