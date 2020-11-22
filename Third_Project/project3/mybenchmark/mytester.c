#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5

int main() {
    printf("Starting tester\n");
    void * memory = myalloc(5000);
    char arr[300];
    int i;
    for(i = 0; i < sizeof(arr); i++){
        arr[i] = 't';
    }
    PutVal(memory, arr, sizeof(char) * sizeof(arr));
    myfree(memory + 400, 4100);
    printf("Sucessfull library call, starting address %08x\n", (unsigned int)memory);
}