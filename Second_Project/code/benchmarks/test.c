#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */
void * do_nothing(void * ptr){
	while (1){
		//do nothing
		printf("IN do nothing\n");
	}
}


int main(int argc, char **argv) {

	printf("Hello this is a tester file\n");
	sleep(5);

	pthread_t thread;
	pthread_create(&thread, NULL, do_nothing, NULL);
	while (1){
		//do nothing
		printf("IN main\n");
	}
	

	return 0;
}
