#include "../inc/memory_management.h"
#include "../inc/print.h"
#include <stdint.h>
#include <stdlib.h>


const int MY_PAGE_SIZE = 4096;


const int NUMBER_OF_REGIONS = 8;
const unsigned BUDDY_LEVELS = 30;


extern char data_phys_begin[];
extern char data_phys_end[];

struct level *blocks_list;

struct block {
    unsigned long address;
};

struct level {
    struct block blocks[32540];
    unsigned number_of_blocks;
};


unsigned long log2(unsigned long v) {
    static const unsigned long b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
                                      0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF00000000};
    register unsigned long r = (v & b[0]) != 0;
    for (int i = 5; i > 0; i--) {
        r |= ((v & b[i]) != 0) << i;
    }

    return r;
}


void init_buddy(multiboot_memory_map_t *memory_info) {
    for (int i = 0; i < NUMBER_OF_REGIONS; i++) {
        /*allocate memory for buddy levels list*/
        if ((memory_info[i].type == 1) && (memory_info[i].len > sizeof(struct level) * BUDDY_LEVELS)) {
            blocks_list = (struct level *) memory_info[i].addr;
            memory_info[i].addr = memory_info[i].addr + sizeof(struct level) * BUDDY_LEVELS;
            memory_info[i].len -= sizeof(struct level) * BUDDY_LEVELS;
        }
    }


    for (int j = 0; j < NUMBER_OF_REGIONS; j++) {
        /*add unreserved memory chunk to list as block*/
        if (memory_info[j].type == 1) {
            multiboot_uint64_t size = memory_info[j].len / MY_PAGE_SIZE;

            //round to the lowest power of two
            while ((size & (size - 1)) != 0)
                size -= 1;

            unsigned no_level = log2(size);

            /*put chunk of memory in list on the level log2(size) as single block*/
            blocks_list[no_level].blocks[blocks_list[no_level].number_of_blocks].address = memory_info[j].addr;
            blocks_list[no_level].number_of_blocks += 1;
        }
    }

}

unsigned long pow2(unsigned order) {
    unsigned long result = 1;
    for (unsigned i = 0; i < order; i++) {
        result *= 2;
    }
    return result;
}


void merge(unsigned order, unsigned long address) {

    for (unsigned j = 0; j < blocks_list[order].number_of_blocks; j++) {
        if (abs(blocks_list[order].blocks[j].address - address) == pow2(order) * MY_PAGE_SIZE) {
            unsigned long result_block;
            if (blocks_list[order].blocks[j].address > address)
                result_block = address;
            else
                result_block = blocks_list[order].blocks[j].address;
            blocks_list[order + 1].blocks[blocks_list[order + 1].number_of_blocks].address = result_block;
            blocks_list[order + 1].number_of_blocks += 1;

            blocks_list[order].number_of_blocks -= 1;

            for (unsigned i = j; i < blocks_list[order].number_of_blocks - 1; i++) {
                blocks_list[order].blocks[j] = blocks_list[order].blocks[j + 1];
            }

            blocks_list[order].number_of_blocks -= 1;


            merge(order + 1, result_block);
        }
    }

}


unsigned long buddy_allocate(unsigned order) {
    if (blocks_list[order].number_of_blocks != 0) {
        blocks_list[order].number_of_blocks -= 1;
        return blocks_list[order].blocks[blocks_list[order].number_of_blocks].address;
    }

    int min_availiable_order = 0;
    for (unsigned i = order; i < BUDDY_LEVELS; i++) {
        if (blocks_list[i].number_of_blocks != 0) {
            min_availiable_order = i;
            break;
        }
    }

    unsigned current = min_availiable_order;
    
    while (current > order) {

        struct block temp = blocks_list[current].blocks[blocks_list[current].number_of_blocks - 1];
        /* splitting i - level block into two i - 1 - level blocks */
        blocks_list[current - 1].blocks[blocks_list[current - 1].number_of_blocks].address = temp.address;

        blocks_list[current - 1].blocks[blocks_list[current - 1].number_of_blocks + 1].address =
                temp.address + MY_PAGE_SIZE * pow2(current - 1);

        blocks_list[current - 1].number_of_blocks += 2;

        /*forget about last i block, because we splitted it*/
        blocks_list[current].number_of_blocks -= 1;


        current--;
    }

    /*return suitable block, delete it from free blocks list*/
    blocks_list[current].number_of_blocks -= 1;
    return blocks_list[current].blocks[blocks_list[current].number_of_blocks].address;

}


void buddy_free(unsigned order, unsigned long address) {
    blocks_list[order].blocks[blocks_list[order].number_of_blocks].address = address;
    blocks_list[order].number_of_blocks += 1;

    merge(order, address);
}

void read_memory_map(struct multiboot_info *mb_info) {

    multiboot_memory_map_t memory_info[8];

    uintptr_t kernel_begin = (uintptr_t) data_phys_begin;
    uintptr_t kernel_end = (uintptr_t) data_phys_end;

    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *) (uintptr_t) mb_info->mmap_addr;

    int i = 0;
    while (i < NUMBER_OF_REGIONS) {
        memory_info[i].addr = mmap->addr;
        memory_info[i].size = mmap->size;
        memory_info[i].type = mmap->type;
        memory_info[i].len = mmap->len;

        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                           + mmap->size + sizeof(mmap->size));

        if (mmap->type == 1)
            break;
        i += 1;
    }
    i += 1;
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

    mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                       + mmap->size + sizeof(mmap->size));
    i += 3;
    while (i != NUMBER_OF_REGIONS) {
        memory_info[i].addr = mmap->addr;
        memory_info[i].size = mmap->size;
        memory_info[i].type = mmap->type;
        memory_info[i].len = mmap->len;

        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                           + mmap->size + sizeof(mmap->size));
        i += 1;
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
