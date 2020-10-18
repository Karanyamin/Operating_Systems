#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

long counter = 0;
pthread_mutex_t mutex;

void * count_to_big(void * arg){
	int i;
	for (i = 0; i < 1000000000; i++){
		//pthread_mutex_lock(&mutex);
		counter++;
		//pthread_mutex_unlock(&mutex);
	}
	pthread_exit(NULL);
}

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */
void * do_nothing(void * ptr){
	int n = 1000;
	while (1){
		if (n < 0) pthread_exit(NULL);
		//do nothing
		printf("IN do nothing\n");
		n--;
	}
}




int main(int argc, char **argv) {
	/*
	printf("Hello this is a tester file\n");
	sleep(5);

	pthread_t thread;
	pthread_create(&thread, NULL, do_nothing, NULL);
	while (1){

		//do nothing
		printf("IN main\n");
		//mypthread_yield();
		pthread_join(thread, NULL);
	}
	*/
	pthread_t t1;
	pthread_t t2;
	pthread_create(&t1, NULL, count_to_big, NULL);
	pthread_create(&t2, NULL, count_to_big, NULL);
	//pthread_mutex_init(&mutex, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	//pthread_mutex_destroy(&mutex);
	printf("Done. Counter = %ld\n", counter);

	return 0;
}
