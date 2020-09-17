#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]){

	int numSysCall = 100000;
	double avgTime = 0;
	pid_t process_id;
	struct timeval start, end, avgStart, avgEnd;

	gettimeofday(&start, NULL);
	for (int i = 0; i < 100000; i++) {
		gettimeofday(&avgStart, NULL);
		process_id = getpid();
		gettimeofday(&avgEnd, NULL);
		double avgSeconds = (avgEnd.tv_sec - avgStart.tv_sec);
		double avgMicroseconds = ((avgSeconds * 1000000) + avgEnd.tv_usec) - (avgStart.tv_usec);
		avgTime += avgMicroseconds;
	}
	gettimeofday(&end, NULL);

	double seconds = (end.tv_sec - start.tv_sec);
	double microseconds = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
	avgTime = avgTime/numSysCall;

	printf("Syscalls Performed: %d\n", numSysCall);
	printf("Total Elapsed Time: %f microseconds\n", microseconds);
	printf("Average Time Per Syscall: %f microseconds\n", avgTime);

	return 0;
}
