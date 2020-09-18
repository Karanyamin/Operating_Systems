#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){

	int numForks = 5000;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	for (int i = 0; i < numForks; i++) {
		if (fork() == 0) {
			exit(0);
		} else {
			wait(NULL);
		}
	}
	gettimeofday(&end, NULL);

	double seconds = (end.tv_sec - start.tv_sec);
	double microseconds = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
	double average_time = microseconds / numForks;

	printf("Forks Performed: %d\n", numForks);
	printf("Total Elapsed Time: %f microseconds\n", microseconds);
	printf("Average Time Per Fork: %f microseconds\n", average_time);

	return 0;
}
