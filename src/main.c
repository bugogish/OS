static void qemu_gdb_hang(void)
{
#ifdef DEBUG
	static volatile int wait = 1;

	while (wait);
#endif
}

#include <ints.h>
#include <ioport.h>
#include <desc.h>
#include <PIT.h>
#define UNUSED(x) (void)(x)
void main(void)
{
	qemu_gdb_hang();
	setup();
	idtsetup();
	PIC_setup();
__asm__ __volatile__ ("sti");

	PIT_setup();
while(1);
	
}
