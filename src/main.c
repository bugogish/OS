#include <ints.h>
#include <ioport.h>
#include <desc.h>
#include <PIT.h>
#include <multiboot_header.h>
#include <memory_management.h>
#include <print.h>

#define UNUSED(x) (void)(x)

void foo(const int i) {
    UNUSED(i);
}


static void qemu_gdb_hang(void) {
#ifdef DEBUG
    static volatile int wait = 1;

    while (wait);
#endif
}


void main(struct multiboot_info *trash, struct multiboot_info *mbi) {
    qemu_gdb_hang();
    UNUSED (trash);
        
    setup();
    idtsetup();
    PIC_setup();
    enable_ints();
    
    read_memory_map(mbi);

    unsigned long test = buddy_allocate(0);
    buddy_free(test);
    printf("%d",  test);

    //PIT_setup();


}
