#include "my_vm.h"

//Implicit Declarations
bool enough_virtual_pages(int pages, int* start_index, int* end_index);
void *get_next_avail_physical();
unsigned createMask(unsigned a, unsigned b);

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
    page_directory_bits = (int)log2(virtual_pages_num) / 2;
    page_table_bits = 32 - offset_bits - page_directory_bits;

    /*  Set up page directory where each element of this array is a reference
        to the start of a page table. Zero out page directory
    */
    page_directory = malloc(sizeof(pde_t) * pow(2, page_directory_bits));
    memset(page_directory, 0, sizeof(pde_t) * pow(2, page_directory_bits));

    /*  Intialize TLB with capacity TLB_SIZE
        Number of entry is equal to TBL_SIZE / 8
        This is because each entry stores a 
        virtual address (32 bits) and physical
        address (32 bits) = 8 bytes for each entry
    */
    unsigned num_of_entriesTLB = TLB_SIZE / 8;

    //Create first node
    tlb_store = malloc(sizeof(struct tlb));
    tlb_store->virtual_address = NULL;
    tlb_store->physical_address = NULL;
    tlb_store->next = NULL;

    //Create the rest of the nodes
    struct tlb * ptr = tlb_store;
    int i;
    for (i = 1; i < num_of_entriesTLB; i++){
        struct tlb * node = malloc(sizeof(struct tlb));
        node->virtual_address = NULL;
        node->physical_address = NULL;
        node->next = NULL;
        ptr->next = node;
        ptr = ptr->next;
    }

    //Initalize hit and miss to 0
    hits = 0;
    misses = 0;

    //Intialize all the mutexes
    if (pthread_mutex_init(&main_mutex, NULL) != 0)
        handle_error("Couldn't intialize mutex");


}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
void
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    /*  Since va might be a address in the middle of a page
        change it so that the entry in the TBL is the address
        of the start of the page
    */
    u_int32_t virtual_address = (u_int32_t)va;
    virtual_address = (virtual_address >> offset_bits) << offset_bits;

    //Since it wasn't there before overwrite the last node in the TLB and move that node to the front
    struct tlb * ptr = tlb_store;
    struct tlb * prev = NULL;
    while (ptr->next != NULL){
        prev = ptr;
        ptr = ptr->next;
    }

    //ptr is on the last node, update entry
    ptr->virtual_address = (void *)virtual_address;
    ptr->physical_address = pa;

    //Move node to front if ptr isn't already there
    if (ptr != tlb_store){
        prev->next = NULL;
        ptr->next = tlb_store;
        tlb_store = ptr;
    }

}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
void *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */

    /*  Since va might be a address in the middle of a page
        change it so that the entry in the TBL is the address
        of the start of the page
    */
    u_int32_t virtual_address = (u_int32_t)va;
    virtual_address = (virtual_address >> offset_bits) << offset_bits;

    //Loop through TLB to see if its there
    struct tlb * ptr = tlb_store;
    struct tlb * prev = NULL;
    while (ptr != NULL){
        u_int32_t TLB_virtual_address = (u_int32_t)(ptr->virtual_address);
        if (TLB_virtual_address == virtual_address && ptr->physical_address != NULL){
            //A HIT! Move this node to the begining of list
            hits++;
            if (ptr == tlb_store){
                //First node in TLB was a hit, no need to move to front of list
                return ptr->physical_address;
            }
            prev->next = ptr->next;
            ptr->next = tlb_store;
            tlb_store = ptr;
            return ptr->physical_address;
        }
        prev = ptr;
        ptr = ptr->next;
    }

    //We reached the end of the TLB but no match was found, MISS!
    misses++;
    return NULL;

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
    
    //We already have the total number of hits and misses, calculate rate
    miss_rate = (double)(misses) / (misses + hits); 



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
    
    //First check if there's a translation in TLB
    void * TLB_physical_address = check_TLB(va);
    if (TLB_physical_address != NULL){
        //We found a translation in the TLB, perform offset and return
        u_int32_t virtual_address = (u_int32_t)va;
        u_int32_t adjusted_physical_addressTLB = (u_int32_t)TLB_physical_address;
        adjusted_physical_addressTLB += virtual_address & createMask(0, offset_bits - 1);
        return (pte_t *)adjusted_physical_addressTLB;
    }
    //No translation in TLB, proceed to find one through page directory
    
    //Find index of page directory and table and get physical address
    u_int32_t virtual_address = (u_int32_t)va - PGSIZE;
    unsigned int page_directory_index = (virtual_address >> offset_bits) >> page_table_bits;
    unsigned int page_table_index = (virtual_address >> offset_bits) & createMask(0, page_table_bits - 1);
    u_int32_t physical_address = (u_int32_t)page_directory[page_directory_index][page_table_index];

    //Add entry to TLB since it wasn't there before
    add_TLB(va, (void*)physical_address);

    //Add offset return
    physical_address += virtual_address & createMask(0, offset_bits - 1);
    return (pte_t *)physical_address;


    //If translation not successfull
    return NULL;
}

void print_page_directories(){
    printf("==============Start of Directory===============\n");
    int i;
    for (i = 0; i < pow(2, page_directory_bits); i++){
        if (page_directory[i] != NULL){
            printf("-----------Start of Page Table at index [%d]-----------\n", i);
            int j;
            for (j = 0; j < pow(2, page_table_bits); j++){
                printf("    Page table index [%d], Physical Address here [%08x]\n", j, (unsigned int)page_directory[i][j]);
            
            }  
            printf("-----------End of Page Table at index [%d]-----------\n", i);
        }
    }
    printf("==============END of Directory===============\n");


    
    printf("============START of physical bitmap==============\n");
    for (i = 0; i < physical_pages_num; i++){
        printf("Index [%d], Value [%d]\n", i, physical_bitmap[i]);
    }
    printf("============END of physical bitmap==============\n");

    printf("============START of virtual bitmap==============\n");
    for (i = 0; i < virtual_pages_num; i++){
        printf("Index [%d], Value [%d]\n", i, virtual_bitmap[i]);
    }
    printf("============END of virtual bitmap==============\n");
    
}

unsigned createMask(unsigned a, unsigned b) {
   unsigned r = 0;
   for (unsigned i=a; i<=b; i++)
       r |= 1 << i;

   return r;
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

    //printf("Index %d, Virtual hex %08x, Physical mapping %08x\n", *((int *)pgdir), (unsigned int)va, (unsigned int)pa);

    //Turn virtual address back into 32 bit integer
    u_int32_t virtual_address = (u_int32_t)va;

    //Gets the index of the page directory and page tables
    unsigned int page_directory_index = (virtual_address >> offset_bits) >> page_table_bits;
    unsigned int page_table_index = (virtual_address >> offset_bits) & createMask(0, page_table_bits - 1);

    //Check if there is a page table for this directory
    if (page_directory[page_directory_index] == NULL){
        //Create a page table and initalize it
        page_directory[page_directory_index] = malloc(sizeof(pte_t *) * pow(2, page_table_bits));
        memset(page_directory[page_directory_index], 0, sizeof(pte_t *) * pow(2, page_table_bits));
    }

    //Add physical address to page table entry
    page_directory[page_directory_index][page_table_index] = pa;

    //printf("Virtual hex %08x, Physical mapping %08x, directory index %i, table index %i\n", (unsigned int)va, (unsigned int)pa, page_directory_index, page_table_index);
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
    //printf("Total virtual pages is %d\n", virtual_pages_num);
    //Use virtual address bitmap to find the next free page (Contiguous)
    int start_index;
    int end_index;
    if (enough_virtual_pages(num_pages, &start_index, &end_index)){
        printf("Start of block [%d], end of block [%d]\n", start_index, end_index);
    } else {
        return NULL;
    }

    //Now that we have a chunk of virtual memory, we need to map a physical address to each entry
    int i;
    for (i = start_index; i <= end_index; i++){
        //Each i is a virtual page we need to find a physical page to map to
        //Turn i into a virtual address
        
        u_int32_t virtual_address = i * pow(2, 12);
        u_int32_t physical_address = (u_int32_t)get_next_avail_physical();

        //We can set this page table entry to the corresponding values
        PageMap( page_directory, (void*)virtual_address, (void*)physical_address);
    }
    
    //Update virtual bitmap
    for (i = start_index; i <= end_index; i++){
        virtual_bitmap[i] = 1;
    }
    //print_page_directories();

    //We need to return the starting virtual address
    u_int32_t virtual_address_start = start_index * pow(2, 12) + PGSIZE;
    return (void*)virtual_address_start;
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
    char * memory = (char*)physical_memory;
    int i;
    for (i = 0; i < physical_pages_num; i++){
        if (physical_bitmap[i] == 0) {
            physical_bitmap[i] = 1;
            //Get address of the start of this page          
            return memory + (PGSIZE * i);
        }
    }
    
    return NULL;
}

void printTLB(){
    struct tlb * ptr = tlb_store;
    printf("========START======\n");
    while (ptr != NULL){
        u_int32_t va = (u_int32_t)(ptr->virtual_address);
        u_int32_t pa = (u_int32_t)(ptr->physical_address);
        printf("Virtual page [%08x], Physical Page [%08x]\n", (unsigned int)va, (unsigned int)pa);
        ptr = ptr->next;
    }
    printf("=======END=======\n");
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

    //Lock the library
    pthread_mutex_lock(&main_mutex);

    //Get number of pages that need to be allocated
    unsigned int unallocated_pages = (num_bytes / PGSIZE);
    if ((num_bytes % PGSIZE) != 0){
        //Theres some leftover bytes
        unallocated_pages++;
    }

    if (enough_physical_pages(unallocated_pages)){
        //Theres enough pages in physical memory to accomodate this request
        //printf("Enough physical pages!\n");
        //Allocate physical memory and map to virtual memory in directory
        void * allocated_pages = get_next_avail_virtual(unallocated_pages);

        if (allocated_pages != NULL){
            //printf("There's enough continguous Virtual pages\n");
            pthread_mutex_unlock(&main_mutex);
            return allocated_pages;
        } else {
            //printf("Not enough contiguous virtual pages\n");
            pthread_mutex_unlock(&main_mutex);
            return NULL;
        }
    } else {
        //printf("Not enough physical pages :(\n");
        pthread_mutex_unlock(&main_mutex);
        return NULL;
    }

    pthread_mutex_unlock(&main_mutex); //The code will never reach this point, but I'm unlocking just in case
    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid

    //Quick error check
    if (size < 0) return;

    //Lock the library
    pthread_mutex_lock(&main_mutex);

    //Find out how many virtual pages is this freeing 
    unsigned int pages = (unsigned int)ceil((double)size / PGSIZE);
    
    //Find starting and ending virtual page index
    u_int32_t virtual_address = (u_int32_t)va - PGSIZE;
    unsigned int start_index = virtual_address >> offset_bits;
    unsigned int end_index = start_index + pages;

    //Free those entries
    int i;
    for (i = start_index; i < end_index; i++){
        //Find page directory index and page table index
        unsigned int page_directory_index = i >> page_table_bits;
        unsigned int page_table_index = i & createMask(0, page_table_bits - 1);

        if (page_directory[page_directory_index] == NULL || page_directory[page_directory_index][page_table_index] == NULL){
            continue;
        }

        //Get Physical Address
        u_int32_t physical_address = (u_int32_t)page_directory[page_directory_index][page_table_index];

        //Clear entry from page table and virtual bitmap
        page_directory[page_directory_index][page_table_index] = NULL;
        virtual_bitmap[i] = 0;

        //Extract physical page number from address and clear bit from bitmap
        u_int32_t physical_mem_start = (u_int32_t)physical_memory;
        unsigned int physical_page_number = (physical_address - physical_mem_start) / PGSIZE;
        physical_bitmap[physical_page_number] = 0;
    }

    //Unlock the mutex
    pthread_mutex_unlock(&main_mutex);

}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/

    //Lock the library
    pthread_mutex_lock(&main_mutex);
    
    //Determine how many bytes we have already copied from val
    unsigned int bytes_copied = 0;



    if ((size / PGSIZE) != 0 || ( (size % PGSIZE) != 0 && ((size % PGSIZE) + ((u_int32_t)va & createMask(0, offset_bits - 1))) > PGSIZE)){
        //Get physical address Note: Physical address will contain offset 
        void * pa = (void *)Translate(page_directory, va);
        //printf("1:1 virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));

        //Get bytes to copy until next page
        u_int32_t virtual_address = (u_int32_t)va;
        unsigned int bytes_till_next_page = PGSIZE - (virtual_address & createMask(0, offset_bits - 1));

        //Copy the next PGSIZE number of bytes from val to pa
        memcpy(pa, val, bytes_till_next_page);
        //printf("1:2 virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));


        bytes_copied += bytes_till_next_page;
        size -= bytes_till_next_page;

        //Change va to next page
        virtual_address += bytes_till_next_page;
        va = (void*)virtual_address;
    }

    //Translate and copy data until there is less than a page worth of bytes to copy
    while ((size / PGSIZE) != 0){
        //There is more than one page worth of bytes still left to copy
        //Find physical address of given virtual address
        void * pa = (void *)Translate(page_directory, va);
        //printf("1: virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));


        //Copy the next PGSIZE number of bytes from val to pa
        memcpy(pa, val + bytes_copied, PGSIZE);

        bytes_copied += PGSIZE;
        size -= PGSIZE;

        //Change va to next page
        u_int32_t virtual_address = (u_int32_t)va;
        virtual_address += PGSIZE;
        va = (void*)virtual_address;  
    }

    //Copy over leftover data
    if ((size % PGSIZE) != 0){
        unsigned int leftover = size % PGSIZE; //Guanteed to be < PGSIZE

        //Get Physical Address
        void * pa = (void *)Translate(page_directory, va);
        //printf("1: virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));

        //Copy left over
        memcpy(pa, val + bytes_copied, leftover);
    }

    //Unlock the library
    pthread_mutex_unlock(&main_mutex);

}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */

    //Lock the library
    pthread_mutex_lock(&main_mutex);

    //Implementation should be very similar to PutVal, except switch source and dest in memcpy
    unsigned int bytes_copied = 0;

    if ((size / PGSIZE) != 0 || ( (size % PGSIZE) != 0 && ((size % PGSIZE) + ((u_int32_t)va & createMask(0, offset_bits - 1))) > PGSIZE)){
        //Get physical address Note: Physical address will contain offset 
        void * pa = (void *)Translate(page_directory, va);
        //printf("1:1 virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));

        //Get bytes to copy until next page
        u_int32_t virtual_address = (u_int32_t)va;
        unsigned int bytes_till_next_page = PGSIZE - (virtual_address & createMask(0, offset_bits - 1));

        //Copy the next PGSIZE number of bytes from val to pa
        memcpy(val, pa, bytes_till_next_page);
        //printf("1:2 virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));


        bytes_copied += bytes_till_next_page;
        size -= bytes_till_next_page;

        //Change va to next page
        virtual_address += bytes_till_next_page;
        va = (void*)virtual_address;
    }

    //Translate and copy data until there is less than a page worth of bytes to copy
    while ((size / PGSIZE) != 0){
        //There is more than one page worth of bytes still left to copy
        //Find physical address of given virtual address
        void * pa = (void *)Translate(page_directory, va);
        //printf("1: virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));


        //Copy the next PGSIZE number of bytes from val to pa
        memcpy(val + bytes_copied, pa, PGSIZE);

        bytes_copied += PGSIZE;
        size -= PGSIZE;

        //Change va to next page
        u_int32_t virtual_address = (u_int32_t)va;
        virtual_address += PGSIZE;
        va = (void*)virtual_address;  
    }

    //Copy over leftover data
    if ((size % PGSIZE) != 0){
        unsigned int leftover = size % PGSIZE; //Guanteed to be < PGSIZE

        //Get Physical Address
        void * pa = (void *)Translate(page_directory, va);
        //printf("1: virtual address [%08x], physical address [%08x], value pointer [%08x]\n", (unsigned int)va, (unsigned int)pa, (unsigned int)(val + bytes_copied));

        //Copy left over
        memcpy(val + bytes_copied, pa, leftover);
    }

    //Unlock library
    pthread_mutex_unlock(&main_mutex);

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

    //Cast the matrix to int arrays
    int* matrix1 = (int*)mat1;
    int* matrix2 = (int*)mat2;
    int* result = (int*)answer;

    //Perform multiplication
    int i,j,k;
    for (i = 0; i < size; i++){
        for (j = 0; j < size; j++){
            int sum = 0;
            for (k = 0; k < size; k++){
                /*  Usually we would do 
                    sum += mat1[i][k] * mat2[k][j]; but since it's 1-D we have 
                    to change it sum += mat1[i * size + k] * mat2[k * size + j]
                    but since its va we use getval and putval
                */
                int first;
                int second;
                GetVal(matrix1 + (i * size + k), &first, sizeof(int));
                GetVal(matrix2 + (k * size + j), &second, sizeof(int));
                sum += first * second;
            }
            //Usually we could do result[i * size + j] = sum;
            PutVal(result + (i * size + j), &sum, sizeof(int));
        }
    }

}
