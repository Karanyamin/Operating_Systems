#include "my_vm.h"

//Implicit Declarations
bool enough_virtual_pages(int pages, int* start_index, int* end_index);

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*
Function responsible for allocating and setting your physical memory
*/
void SetPhysicalMem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating


    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

    //Allocate and zero out Physical Memory
    physical_memory = malloc(MEMSIZE);
    if (physical_memory == NULL){
        handle_error("Unable to allocate physical memory");
    }
    memset(physical_memory, 0, MEMSIZE);

    //Calculate number of physical pages
    physical_pages_num = MEMSIZE / PGSIZE;

    //Calculate number of virtual pages
    virtual_pages_num = MAX_MEMSIZE / PGSIZE;

    //Initalize physical and virtual bitmaps
    physical_bitmap = malloc(sizeof(int) * physical_pages_num);
    virtual_bitmap = malloc(sizeof(int) * virtual_pages_num);
    memset(physical_bitmap, 0, sizeof(int) * physical_pages_num);
    memset(virtual_bitmap, 0, sizeof(int) * virtual_pages_num);

    //Get number of bits for offset
    offset_bits = (int)log2(PGSIZE);

    //Get number of entries in Page Directory and Page Table
    page_directory_numOfEntries = (int)log2(virtual_pages_num) / 2;
    page_table_numOfEntries = 32 - offset_bits - page_directory_numOfEntries;

    /*  Set up page directory where each element of this array is a reference
        to the start of a page table. Zero out page directory
    */
    page_directory = malloc(sizeof(pde_t) * pow(2, page_directory_numOfEntries));
    memset(page_directory, 0, sizeof(pde_t) * pow(2, page_directory_numOfEntries));

}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;

    /*Part 2 Code here to calculate and print the TLB miss rate*/




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}


/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * Translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address


    //If translation not successfull
    return NULL;
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
PageMap(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to Translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    return -1;
}


/*Function that gets the next available page
*/
void *get_next_avail_virtual(int num_pages) {
    /*
    //HACK virtual bitmap
    virtual_bitmap = malloc(sizeof(int) * 10);
    virtual_bitmap[0] = 0;
    virtual_bitmap[1] = 1;
    virtual_bitmap[2] = 0;
    virtual_bitmap[3] = 1;
    virtual_bitmap[4] = 0;
    virtual_bitmap[5] = 0;
    virtual_bitmap[6] = 1;
    virtual_bitmap[7] = 0;
    virtual_bitmap[8] = 0;
    virtual_bitmap[9] = 0;
    virtual_pages_num = 10;
    printf("================Start=================\n");
    int i;
    for (int i = 0; i < virtual_pages_num; i++){
        printf("%d ", virtual_bitmap[i]);
    }
    printf("\n================END=================\n");
    */
    printf("Total virtual pages is %d\n", virtual_pages_num);
    //Use virtual address bitmap to find the next free page (Contiguous)
    int start_index;
    int end_index;
    if (enough_virtual_pages(num_pages, &start_index, &end_index)){
        printf("Start of block [%d], end of block [%d]\n", start_index, end_index);
        return malloc(10);
    } else {
        return NULL;
    }
}

//Checks if there is enough virtual pages to satisfy request
bool enough_virtual_pages(int pages, int* start_index, int* end_index){
    //Edge case
    if (pages == 0) return true;

    int temp = pages;

    int i;
    for (i = 0; i < virtual_pages_num; i++){
        if (virtual_bitmap[i] == 0){
            *start_index = i;
            temp--;
            i++;
            //Check if there is a contiguous chunk of temp - 1 more pages ahead
            while (i < virtual_pages_num && temp > 0 && virtual_bitmap[i] == 0){
                temp--;
                i++;
            }
            if (temp == 0){
                //Enough virtual pages to satisfy request
                *end_index = i-1; //Inclusive
                return true;
            } else if (i == virtual_pages_num){
                //Reached end of virtual bitmap and there are still more virtual pages left to be allocated
                return false;
            } else {
                //Theres a empty chunk but its not contiguous
                temp = pages;
            }
        }
    }
    return false;
}


//Checks if there is enough physical pages to satisfy request
bool enough_physical_pages(int pages){
    /*
    //Edge Case when pages == 0
    if (pages == 0) return true;

    char * memory = (char*)physical_memory; //Just so we can do pointer arthemetic

    int i;
    for (i = 0; i < MEMSIZE; i += PGSIZE){
        if (memory[i] == 0) pages--;

        if (pages == 0) return true; //All pages can be allocated
    }

    return false; //Not enough pages to satisfy request
    */
    if (pages == 0) return true;
    int i;
    for (i = 0; i < physical_pages_num; i++){
        if (physical_bitmap[i] == 0) pages--;

        if (pages == 0) return true;
    }
    return false;
}


//Get the address of the next avaiable physical page that is free
void *get_next_avail_physical() {

    //Use physical address bitmap to find the next free page


}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *myalloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will
   have to mark which physical pages are used. */

    //Check if physical memory has been intialized
    if (physical_memory == NULL){
        SetPhysicalMem();
    }

    //Get number of pages that need to be allocated
    unsigned int unallocated_pages = (num_bytes / PGSIZE);
    if ((num_bytes % PGSIZE) != 0){
        //Theres some leftover bytes
        unallocated_pages++;
    }

    if (enough_physical_pages(unallocated_pages)){
        //Theres enough pages in physical memory to accomodate this request
        printf("ENough pages!\n");
        if (get_next_avail_virtual(unallocated_pages) != NULL){
            printf("There's enough continguous Virtual pages\n");
        } else {
            printf("Not enough contiguous virtual pages\n");
            return NULL;
        }
    } else {
        printf("Not enough pages :(\n");
        return NULL;
    }

    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */


}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void MatMult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to
    getting the values from two matrices, you will perform multiplication and
    store the result to the "answer array"*/


}
