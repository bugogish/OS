#ifndef __MEMORY_MANAGEMENT_H__
#define __MEMORY_MANAGEMENT_H__


#include <multiboot_header.h>
void read_memory_map(struct multiboot_info* mbi);

void buddy_free (unsigned order, unsigned long address);

unsigned long buddy_allocate (unsigned order);

#endif
