#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]){

	int numForks = 5000;
	double avgTime = 0;
	pid_t cpid;
	struct timeval start, end, avgStart, avgEnd;

	gettimeofday(&start, NULL);
	for (int i = 0; i < 5000; i++) {
		gettimeofday(&avgStart, NULL);
		if (fork() == 0) {
			exit(0);
		} else {
			wait(NULL);
		}
		gettimeofday(&avgEnd, NULL);

		double avgSeconds = (avgEnd.tv_sec - avgStart.tv_sec);
		double avgMicroseconds = ((avgSeconds * 1000000) + avgEnd.tv_usec) - (avgStart.tv_usec);
		avgTime += avgMicroseconds;
	}
	gettimeofday(&end, NULL);

	avgTime = avgTime/numForks;

	double seconds = (end.tv_sec - start.tv_sec);
	double microseconds = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

	printf("Forks Performed: %d\n", numForks);
	printf("Total Elapsed Time: %f microseconds\n", microseconds);
	printf("Average Time Per Fork: %f microseconds\n", avgTime);

	return 0;
}
