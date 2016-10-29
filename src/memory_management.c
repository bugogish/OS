#include "../inc/memory_management.h"
#include "../inc/print.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


const int MY_PAGE_SIZE = 4096;
int NUMBER_OF_REGIONS;
const unsigned int NUM_LEVELS = 25;
unsigned long TOTAL_MEMORY_AVAILIABLE;


multiboot_memory_map_t *memory_info;


extern char text_phys_begin[];
extern char bss_phys_end[];


struct descriptor
{
    struct descriptor* prev;
    struct descriptor* next;
    bool is_free;
    unsigned order;
};


struct descriptor* descriptors;
struct descriptor* free_blocks [25];


unsigned long log2(unsigned long v) {
    static const unsigned long b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
                                      0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF00000000};
    register unsigned long r = (v & b[0]) != 0;
    for (int i = 5; i > 0; i--) {
        r |= ((v & b[i]) != 0) << i;
    }

    return r;
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


void remove_from_list (struct descriptor *buddy){
    if (buddy->prev != NULL)
    {
        if (buddy->next != NULL)
        {
            buddy->prev->next = buddy->next;
            buddy->next->prev = buddy->prev;
        }
        else
        {
            buddy->prev->next = NULL;
        }
    }
    else
    {
        if (buddy->next != NULL)
        {
            buddy->next->prev = NULL;
        }
        else
        {
            buddy = NULL;
        }
    }
}


void pop_from_list(unsigned order)
{

    if (free_blocks[order] != NULL)
    {
        if (free_blocks[order]->next != NULL)
        {
            free_blocks[order] = free_blocks[order]->next;
            free_blocks[order]->prev = NULL;
        }
        else
            free_blocks[order] = NULL;

    }
    else
        free_blocks[order] = NULL;
}


void init_buddy(multiboot_memory_map_t *memory_info) {

    //allocate memory for TOTAL_MEMORY_AVAILABLE / PAGE_SIZE descriptors

    TOTAL_MEMORY_AVAILIABLE = memory_info[NUMBER_OF_REGIONS - 1].addr + memory_info[NUMBER_OF_REGIONS - 1].len - memory_info[0].addr;
    
    const unsigned long num_descriptors = TOTAL_MEMORY_AVAILIABLE / MY_PAGE_SIZE;
    bool has_found = false;

    for (int i = 0; i < NUMBER_OF_REGIONS; i++)
    {

        if ((memory_info[i].type == 1) && (memory_info[i].addr + memory_info[i].len < 0xFFFFFFFF)  && (memory_info[i].len - 1 > sizeof(struct descriptor) * num_descriptors))
        {
            if (memory_info[i].addr == 0x0)
                memory_info[i].addr += 1;

            descriptors = (struct descriptor*) memory_info[i].addr;
            memory_info[i].addr = memory_info[i].addr + sizeof(struct descriptor) * num_descriptors;
            memory_info[i].len -= sizeof(struct descriptor) * num_descriptors;
            has_found = true;
            break;
        }
    }

    if (!has_found)
    {
        printf("%s\n", "No available memory to init buddy allocator.");
    }
    else
    {
        for (unsigned long i = 0; i < num_descriptors; i ++)
        {
            descriptors[i].is_free = false;
            descriptors[i].order = 0;
        }


        for (int j = 0; j < NUMBER_OF_REGIONS; j++) {

            unsigned long address = memory_info[j].addr;
            unsigned long shift_right = MY_PAGE_SIZE - address % MY_PAGE_SIZE;
            unsigned long shift_left = (address + memory_info[j].len) % MY_PAGE_SIZE;

            unsigned long left_border;
            unsigned long right_border;

         
            left_border = address + shift_right;
            right_border = (address + memory_info[j].len) - shift_left;

            if (left_border < right_border)
            {
                if (memory_info[j].type == 1)
                {
                    unsigned long region_size = (right_border - left_border) / MY_PAGE_SIZE;
                   
                    
                    for (unsigned long i = 0; i < region_size; i ++)
                    {
                        buddy_free(left_border + i * MY_PAGE_SIZE);
                    }
                   
                }
            }
        }

    }
}


unsigned long buddy_allocate(unsigned order)
{
    unsigned current = 0;
    bool has_found = false;

    for (unsigned i = order; i < NUM_LEVELS; i++) {
        if (free_blocks[i] != NULL) {
            current = i;
            has_found = true;
            break;
        }
    }

    if (!has_found)
    {
        printf("%s", "No appropriate memory available.");
        return 0;
    }
    else
    {
        while (current > order) {
            struct descriptor *page = free_blocks[current];
            unsigned long page_no = page - descriptors;
            unsigned long buddy_no = page_no ^ (1ul << (current - 1));
            struct descriptor *buddy = &descriptors[buddy_no];

            pop_from_list(current);
            append_to_list_head(buddy, current - 1);
            append_to_list_head(page, current - 1);

            buddy->order = current - 1;
            buddy->is_free = 1;
            page->order = current - 1;
            --current;
        }

        struct descriptor *page = free_blocks[order];

        pop_from_list(order);
        page->is_free = 0;
        return (page - descriptors) * MY_PAGE_SIZE;
    }

}


void buddy_free(unsigned long address)
{
    const unsigned long count = TOTAL_MEMORY_AVAILIABLE / MY_PAGE_SIZE;
    struct descriptor *page = &descriptors[address / MY_PAGE_SIZE];
    unsigned long page_no = page - descriptors;

    while (page -> order < NUM_LEVELS - 1) {
        unsigned long buddy_no = page_no ^ (1 << page -> order);
        
        struct descriptor *buddy = &descriptors[buddy_no];

  printf("%s %d\n", "Freed", page_no);
        if (buddy_no >= count)
            break;

        if (!buddy->is_free || buddy->order != page -> order)
            break;

        remove_from_list(buddy);
    
        
  
        if (page_no > buddy_no) {
            page = buddy;
            page_no = buddy_no;
        }
        page -> order += 1;
    }
    page->is_free = 1;
    append_to_list_head(page, page -> order);
}

void read_memory_map(struct multiboot_info *mb_info) {

    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *) (uintptr_t) mb_info->mmap_addr;
    NUMBER_OF_REGIONS = mb_info -> mmap_length / (mmap->size + sizeof (mmap -> size));
    printf("%s%d\n", "NUMBER_OF_REGIONS ", NUMBER_OF_REGIONS);

    uint64_t kernel_begin = (uint64_t) text_phys_begin;
    uint64_t kernel_end = (uint64_t) bss_phys_end;

    // allocating memory for memory_info table
    for (int i = 0; i < NUMBER_OF_REGIONS; i ++)
    {
        if ((mmap -> type == 1) && (mmap -> len > (NUMBER_OF_REGIONS + 3) * sizeof (multiboot_memory_map_t)) && ((!(mmap -> addr <= kernel_begin) && (!(mmap -> addr + mmap -> len >= kernel_end )))) )
        {
            memory_info = (multiboot_memory_map_t *) mmap -> addr;
            break;
        }
        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                           + mmap->size + sizeof(mmap->size));
    }



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
            memory_info[i].len = mmap -> len - NUMBER_OF_REGIONS * sizeof (multiboot_memory_map_t);
            memory_info[i].size = mmap -> size;

            i += 1;
        }

        else
        {
            if ((mmap -> type == 1) && ((mmap -> addr <= kernel_begin) && (mmap -> addr + mmap -> len >= kernel_end )))
            {
                memory_info[i].addr = mmap->addr;
                memory_info[i].size = mmap->size;
                memory_info[i].len = kernel_begin - mmap->addr;
                memory_info[i].type = 1;

                memory_info[i + 1].addr = kernel_begin;
                memory_info[i + 1].size = mmap->size;
                memory_info[i + 1].len = kernel_end - kernel_begin + 1;
                memory_info[i + 1].type = 2;

                memory_info[i + 2].addr = kernel_end + 1;
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
