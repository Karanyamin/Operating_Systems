#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5

int main() {
    printf("Starting tester\n");
    myalloc(1073741824);
    printf("Sucessfull library call\n");
}