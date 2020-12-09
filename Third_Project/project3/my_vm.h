 #ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4096

// Maximum size of your memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024 //4GB

#define MEMSIZE 1024*1024*1024

// Represents a page table entry
typedef void* pte_t;

// Represents a page directory entry
typedef pte_t* pde_t;

//Physical memory
void * physical_memory;

//Number of Physical pages
unsigned int physical_pages_num;

//Number of Virtual pages
unsigned int virtual_pages_num;

//Physical bitmap
int * physical_bitmap;

//Virtual bitmap;
int * virtual_bitmap;

//Start of Page Directory
pde_t * page_directory;

//Bits for PG, PT, offset
int offset_bits;
int page_directory_bits;
int page_table_bits;

//Data for hits and misses
unsigned long long hits;
unsigned long long misses;


#define TLB_SIZE 120

//Structure to represents TLB
struct tlb {

    //Assume your TLB is a direct mapped TLB of TBL_SIZE (entries)
    // You must also define wth TBL_SIZE in this file.
    //Assume each bucket to be 4 bytes
    void * virtual_address;
    void * physical_address;
    struct tlb * next;
};
struct tlb * tlb_store;


void SetPhysicalMem();
pte_t* Translate(pde_t *pgdir, void *va);
int PageMap(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *myalloc(unsigned int num_bytes);
void myfree(void *va, int size);
void PutVal(void *va, void *val, int size);
void GetVal(void *va, void *val, int size);
void MatMult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();

#endif
