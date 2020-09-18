#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]){

	int numSysCall = 100000;
	struct timeval start, end;//, avgStart, avgEnd;

	gettimeofday(&start, NULL);
	for (int i = 0; i < numSysCall; i++) {
		getpid();
	}
	gettimeofday(&end, NULL);

	double seconds = (end.tv_sec - start.tv_sec);
	double microseconds = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
	double average_time = microseconds / numSysCall;

	printf("Syscalls Performed: %d\n", numSysCall);
	printf("Total Elapsed Time: %f microseconds\n", microseconds);
	printf("Average Time Per Syscall: %f microseconds\n", average_time);

	return 0;
}
