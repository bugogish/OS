#include "../inc/memory_management.h"
#include "../inc/print.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))


const int MY_PAGE_SIZE = 4096;


int NUMBER_OF_REGIONS;
const unsigned int NUM_LEVELS = 25;
unsigned long TOTAL_MEMORY_AVAILIABLE;

multiboot_memory_map_t *memory_info;
struct descriptor* free_blocks [25];

extern char data_phys_begin[];
extern char data_phys_end[];


struct descriptor
{
    struct descriptor* prev;
    struct descriptor* next;
    bool is_free;
    unsigned order;
    unsigned long no;
    unsigned long address;
};

struct descriptor* descriptors;

unsigned long log2(unsigned long v) {
    static const unsigned long b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
                                      0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF00000000};
    register unsigned long r = (v & b[0]) != 0;
    for (int i = 5; i > 0; i--) {
        r |= ((v & b[i]) != 0) << i;
    }

    return r;
}


unsigned long descr_no (unsigned long address)
{
    return address / MY_PAGE_SIZE;

}

unsigned long desc_addr (unsigned long no)
{
    return no * MY_PAGE_SIZE;
}


void append_to_list_head (struct descriptor* descr, unsigned order)
{
    if (free_blocks[order] == NULL)
        free_blocks[order] = descr;
    else
    {
        free_blocks[order]->prev = descr;
        descr->next = free_blocks[order];
        descr->prev = NULL;
        free_blocks[order] = descr;
    }
}

void traverse_list(unsigned order)
{
    struct descriptor * temp = free_blocks[order];
    if (temp != 0)
    while (temp -> next != 0) {
        printf("%s%d%s%x\n", "LEVEL NO ", order, " adress of block ", free_blocks[order]->address);
        temp = temp -> next;
    }
}

void init_buddy(multiboot_memory_map_t *memory_info) {

    //allocate memory for TOTAL_MEMORY_AVAILABLE / PAGE_SIZE descriptors

    TOTAL_MEMORY_AVAILIABLE = memory_info[NUMBER_OF_REGIONS - 1].addr + memory_info[NUMBER_OF_REGIONS - 1].len - memory_info[0].addr;


    for (int i = 0; i < NUMBER_OF_REGIONS; i++)
    {
        if ((memory_info[i].type == 1) && (memory_info[i].len > sizeof(struct descriptor) * (TOTAL_MEMORY_AVAILIABLE / MY_PAGE_SIZE)))
        {
            descriptors = (struct descriptor*) memory_info[i].addr;
            memory_info[i].addr = memory_info[i].addr + sizeof(struct descriptor) * (TOTAL_MEMORY_AVAILIABLE / MY_PAGE_SIZE);
            memory_info[i].len -= sizeof(struct descriptor) * (TOTAL_MEMORY_AVAILIABLE / MY_PAGE_SIZE);
        }
    }

//(TOTAL_MEMORY_AVAILIABLE / MY_PAGE_SIZE)

    for (int j = 0; j < NUMBER_OF_REGIONS; j++) {
        /*add unreserved memory chunk to list as block*/

        multiboot_uint64_t size = memory_info[j].len / MY_PAGE_SIZE;

        //round to the lowest power of two
        while ((size & (size - 1)) != 0)
            size -= 1;

        unsigned no_level = log2(size);

        if (memory_info[j].type == 1) {

            descriptors[descr_no(memory_info[j].addr)].is_free = true;
            descriptors[descr_no(memory_info[j].addr)].prev = NULL;
            descriptors[descr_no(memory_info[j].addr)].order = no_level;
            descriptors[descr_no(memory_info[j].addr)].no = descr_no(memory_info[j].addr);
            descriptors[descr_no(memory_info[j].addr)].address = memory_info[j].addr;

            if (free_blocks[no_level] == NULL)
            {
                descriptors[descr_no(memory_info[j].addr)].next = NULL;
            }
            else
            {
                descriptors[descr_no(memory_info[j].addr)].next = free_blocks[no_level];
                free_blocks[no_level]->prev = &descriptors[descr_no(memory_info[j].addr)];
            }


            free_blocks[no_level] = &descriptors[descr_no(memory_info[j].addr)];


            /*put chunk of memory in list on the level log2(size) as single block*/
        }

        if (memory_info[j].type == 2)
        {
            descriptors[descr_no(memory_info[j].addr)].is_free = false;
            descriptors[descr_no(memory_info[j].addr)].next = NULL;
            descriptors[descr_no(memory_info[j].addr)].prev = NULL;
            descriptors[descr_no(memory_info[j].addr)].order = no_level;
        }
    }



    for (unsigned i = 0; i < NUM_LEVELS; i ++)
    {
        traverse_list(i);
    }

}

unsigned long pow2(unsigned order) {
    unsigned long result = 1;
    for (unsigned i = 0; i < order; i++) {
        result *= 2;
    }
    return result;
}

unsigned long xor(unsigned long x, unsigned long y)
{
    unsigned long a = x & y;
    unsigned long b = ~x & ~y;
    unsigned long z = ~a & ~b;
    return z;
}


void merge(unsigned order, unsigned long address) {

    for (unsigned i = 0; i < NUM_LEVELS; i ++)
    {
        //traverse_list(i);
    }


    unsigned long block_no = descr_no(address);
    unsigned long buddy_no = xor(block_no, pow2(order));

    if (descriptors[buddy_no].is_free == true)
    {
        if (block_no > buddy_no)
        {
            unsigned long temp = block_no;
            block_no = buddy_no;
            buddy_no = temp;
        }

        descriptors[block_no].order += 1;
        descriptors[block_no].prev = descriptors[block_no].next;
        descriptors[block_no].next = descriptors[block_no].prev;

        append_to_list_head(&descriptors[block_no], order + 1);

        merge (order + 1, desc_addr(block_no));
    }


}






void pop_from_list(unsigned order)
{

    if (free_blocks[order] != NULL)
    {
        if (free_blocks[order]->next != NULL)
        {
            free_blocks[order] = free_blocks[order]->next;
            free_blocks[order]->next->prev = free_blocks[order]->prev;
        }
        else
            free_blocks[order] = NULL;

    }
    else
        free_blocks[order] = NULL;
}



unsigned long buddy_allocate(unsigned order)
{

    if (free_blocks[order] != NULL)
    {
        free_blocks[order]->is_free = false;
        if (free_blocks[order]->next != NULL)
        {
            free_blocks[order] = free_blocks[order]->next;
            free_blocks[order]->next->prev = free_blocks[order]->prev;
        }
        else
            free_blocks[order] = NULL;

        return free_blocks[order]->address;
    }


    unsigned min_availiable_order = 0;
    for (unsigned i = order; i < NUM_LEVELS; i++) {
        if (free_blocks[i] != NULL) {
            min_availiable_order = i;
            break;
        }
    }

    unsigned current = min_availiable_order;
    while (current > order) {
        for (unsigned i = 0; i < NUM_LEVELS; i ++)
        {
            //traverse_list(i);
        }


        descriptors[free_blocks[current]->no].order -= 1;
        append_to_list_head(&descriptors[free_blocks[current]->no], current - 1);

        descriptors[descr_no(free_blocks[current]->address + pow2(current - 1) * MY_PAGE_SIZE)].is_free = true;
        descriptors[descr_no(free_blocks[current]->address + pow2(current - 1) * MY_PAGE_SIZE)].no = descr_no(free_blocks[current]->address + pow2(current - 1) * MY_PAGE_SIZE);
        descriptors[descr_no(free_blocks[current]->address + pow2(current - 1) * MY_PAGE_SIZE)].address = free_blocks[current]->address + pow2(current - 1) * MY_PAGE_SIZE;
        descriptors[descr_no(free_blocks[current]->address + pow2(current - 1) * MY_PAGE_SIZE)].order = current - 1;
        append_to_list_head(&descriptors[descr_no(free_blocks[current]->address + pow2(current - 1) * MY_PAGE_SIZE)], current - 1);

        pop_from_list(current);


        current--;
    }

    /*return suitable block, delete it from free blocks list*/

    unsigned long return_value = free_blocks[current]->address;

    if (free_blocks[order] != NULL)
    {
        free_blocks[order]->is_free = false;
        if (free_blocks[order]->next != NULL)
        {
            free_blocks[order] = free_blocks[order]->next;
            free_blocks[order]->next->prev = free_blocks[order]->prev;
        }
        else
            free_blocks[order] = NULL;

        return free_blocks[order]->address;
    }
    return return_value;

}


void buddy_free(unsigned order, unsigned long address)
{
    descriptors[descr_no(address)].is_free = true;

    merge(order, address);
}

void read_memory_map(struct multiboot_info *mb_info) {

    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *) (uintptr_t) mb_info->mmap_addr;
    NUMBER_OF_REGIONS = mb_info -> mmap_length / (mmap->size + sizeof (mmap -> size));
    printf("%s%d", "NUMBER_OF_REGIONS ", NUMBER_OF_REGIONS);


    // allocating memory for memory_info table
    for (int i = 0; i < NUMBER_OF_REGIONS; i ++)
    {
        if ((mmap -> type == 1) && (mmap -> len > (NUMBER_OF_REGIONS + 3) * sizeof (multiboot_memory_map_t)) )
        {
            memory_info = (multiboot_memory_map_t *) mmap -> addr;
            break;
        }
        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                           + mmap->size + sizeof(mmap->size));
    }

    uintptr_t kernel_begin = (uintptr_t) data_phys_begin;
    uintptr_t kernel_end = (uintptr_t) data_phys_end;

    mmap = (multiboot_memory_map_t *) (uintptr_t) mb_info->mmap_addr;

    int i = 0;
    NUMBER_OF_REGIONS += 3; //one splits in two for memory_info, and another one splits in three for kernel

    while (i < NUMBER_OF_REGIONS) {

        // mark memory_info area as allocated
        if ((multiboot_memory_map_t*) mmap -> addr == memory_info)
        {
            memory_info[i].addr = mmap ->addr;
            memory_info[i].len = NUMBER_OF_REGIONS * sizeof (multiboot_memory_map_t);
            memory_info[i].size = mmap -> size;
            memory_info[i].type = 2;

            i += 1;

            memory_info[i].addr = mmap -> addr + NUMBER_OF_REGIONS * sizeof (multiboot_memory_map_t);
            memory_info[i].type = 1;
            memory_info[i].len = mmap -> len - memory_info[i].addr;
            memory_info[i].size = mmap -> size;

            i += 1;
        }

        else
        {
            if ((mmap -> type == 1) && ((mmap -> addr < kernel_begin) && (mmap -> addr + mmap -> len > kernel_end )))
            {
                memory_info[i].addr = mmap->addr;
                memory_info[i].size = mmap->size;
                memory_info[i].len = kernel_begin - mmap->addr;
                memory_info[i].type = 1;

                memory_info[i + 1].addr = kernel_begin;
                memory_info[i + 1].size = mmap->size;
                memory_info[i + 1].len = kernel_end - kernel_begin;
                memory_info[i + 1].type = 2;

                memory_info[i + 2].addr = kernel_end;
                memory_info[i + 2].size = mmap->size;
                memory_info[i + 2].len = mmap->len - memory_info[2].addr;
                memory_info[i + 2].type = mmap->type;

                i += 3;
            }
            else
            {
                memory_info[i].addr = mmap->addr;
                memory_info[i].size = mmap->size;
                memory_info[i].type = mmap->type;
                memory_info[i].len = mmap->len;

                i +=  1;
            }
        }


        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                           + mmap->size + sizeof(mmap->size));

    }

    for (int i = 0; i < NUMBER_OF_REGIONS; i++) {
        printf(" size = 0x%x, base_addr = 0x%x%x,"
                       " length = 0x%x%x, type = 0x%x\n",
               (unsigned) memory_info[i].size,
               memory_info[i].addr >> 32UL,
               memory_info[i].addr & 0xffffffff,
               memory_info[i].len >> 32,
               memory_info[i].len & 0xffffffff,
               (unsigned) memory_info[i].type);
    }

    init_buddy(memory_info);


}
