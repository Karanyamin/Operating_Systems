#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5
#define MaxMatrixValue 10

pthread_t * threads;
void printResult(int * matrix, int size_of_matrix);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void verifyMult(int* A, int* B, int size, int* result){
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
                sum += A[i * size + k] * B[k * size + j];
            }
            //printf("%d ", sum);
            result[i * size + j] = sum;
        }
        //printf("\n");
    }
    //printf("\n");
    //printf("Verified Solution of A and B\n");
    //printResult(result, size);

}

void sameMatrix(int* a, int* b, int size){

    int i;
    for (i = 0; i < size; i++){
        if (a[i] != b[i]){
            printf("Matrix A [%p] and B [%p] are different\n", a, b);
            return;
        }
    }

    printf("Matrix A [%p] and B [%p] are the same\n", a, b);
}

void printResult(int * matrix, int size_of_matrix){
    pthread_mutex_lock(&mutex);
    int i,j;
    printf("In print Result\n");
    for (i = 0; i < size_of_matrix; i++){
        for (j = 0; j < size_of_matrix; j++){
            printf("%d ", matrix[i * size_of_matrix + j]);
        }
        printf("\n");
    }
    printf("\n");
    pthread_mutex_unlock(&mutex);
}

void* doMultiplication(void * size){
    int size_of_matrix = *((int *)size);
    //printf("==================================START ID [%lu]=================================\n", pthread_self());
    //printf("Starting multiplication with matrix size [%d], and total bytes for each matrix is [%d]\n", size_of_matrix, (int)pow(size_of_matrix, 2) * sizeof(int) );
    int* matrix1 = (int*)myalloc(sizeof(int) * pow(size_of_matrix, 2));
    int* matrix2 = (int*)myalloc(sizeof(int) * pow(size_of_matrix, 2));
    int* result = (int*)myalloc(sizeof(int) * pow(size_of_matrix, 2));
    if (matrix1 == NULL || matrix2 == NULL || result == NULL){
        printf("Could not allocate memory\n");
        exit(-1);
    }

   //printf("Address of matrix 1 is [%p]\n", matrix1);
    //printf("Address of matrix 1 is [%p]\n", matrix2);
    //printf("Address of result is [%p]\n", result);

    //Create the two matrix
    //printf("\nMatrix 1\n");
    int* A = malloc(sizeof(int) * pow(size_of_matrix, 2));
    int i,j;
    for (i = 0; i < size_of_matrix; i++){
        for (j = 0; j < size_of_matrix; j++){
            A[i * size_of_matrix + j] = rand() % MaxMatrixValue; //Pick a number from 0 to MaxMatrixValue
            //Just to make half the matrix negative on average
            if ((rand() % MaxMatrixValue) < (MaxMatrixValue / 2))
                A[i * size_of_matrix + j] = -A[i * size_of_matrix + j];
            //printf("%d ", A[i * size_of_matrix + j]);
        }
        //printf("\n");
    }
    //printf("\n");

    //printf("Matrix 2\n");
    int* B = malloc(sizeof(int) * pow(size_of_matrix, 2));
    for (i = 0; i < size_of_matrix; i++){
        for (j = 0; j < size_of_matrix; j++){
            B[i * size_of_matrix + j] = rand() % MaxMatrixValue; //Pick a number from 0 to MaxMatrixValue
            if ((rand() % MaxMatrixValue) < (MaxMatrixValue / 2))
                B[i * size_of_matrix + j] = -B[i * size_of_matrix + j];
            //printf("%d ", B[i * size_of_matrix + j]);
        }
        //printf("\n");
    }
    //printf("\n"); 

    for (i = 0; i < size_of_matrix; i++){
        for (j = 0; j < size_of_matrix; j++){
            PutVal(matrix1 + (i * size_of_matrix + j), A + (i * size_of_matrix + j), sizeof(int));
            PutVal(matrix2 + (i * size_of_matrix + j), B + (i * size_of_matrix + j), sizeof(int));
        }
    }
    MatMult(matrix1, matrix2, size_of_matrix, result);
    //printf("Results\n");
    int * matmultresult = malloc(sizeof(int) * pow(size_of_matrix, 2));
    for (i = 0; i < size_of_matrix; i++){
        for (j = 0; j < size_of_matrix; j++){
            int val;
            GetVal(result + (i * size_of_matrix + j), &val, sizeof(int));
            //printf("%d ", val);
            matmultresult[i * size_of_matrix + j] = val;
        }
        //printf("\n");
    }
    //printf("\n");

    //Create a verified matrix
    int* verifiedMatrix = malloc(sizeof(int) * pow(size_of_matrix, 2));
    verifyMult(A, B, size_of_matrix, verifiedMatrix);
    sameMatrix(matmultresult, verifiedMatrix, size_of_matrix);
    //printResult(A, size_of_matrix);
    //printResult(B, size_of_matrix);
    //printResult(matmultresult, size_of_matrix);


    free(verifiedMatrix);
    free(matmultresult);
    free(A);
    free(B);
    //printf("==================================END [%lu]=================================\n", pthread_self());
    pthread_exit(NULL);
}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Not enough arguements\n");
        exit(-1);
    }

    int num_threads = atoi(argv[1]);

    srand((unsigned int)time(NULL));

    int size_of_matrix = rand() % SIZE;

    //Just so we dont have a matrix with size 0 or 1
    while (size_of_matrix == 0 || size_of_matrix == 1){
        size_of_matrix = rand() % SIZE;
    }

    threads = malloc(sizeof(pthread_t) * num_threads);

    int i;
    for (i = 0; i < num_threads; i++){
        pthread_create(threads + i, NULL, doMultiplication, &size_of_matrix);
    }

    for (i = 0; i < num_threads; i++){
        pthread_join(threads[i], NULL);
    }

    print_TLB_missrate();

}