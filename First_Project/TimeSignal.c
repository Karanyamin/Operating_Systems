#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// Place any necessary global variables here
int number_of_iterations = 0;
struct timeval start;
struct timeval end;

void handle_sigfpe(int signum){
	if (++number_of_iterations == 100000){
		gettimeofday(&end, NULL);
		double total_time = (1000000 * end.tv_sec + end.tv_usec) - (1000000 * start.tv_sec + start.tv_usec);
		double average_time = total_time / number_of_iterations;

		printf("Exceptions Occurred: %d\n", number_of_iterations);
		printf("Total Elapsed Time: %f microseconds\n", total_time);
		printf("Average Time Per Exception: %f microseconds\n", average_time);
		exit(0);
	} else {
		return;
	}
	// Handler code goes here

}

int main(int argc, char *argv[]){

	int x = 5;
	int y = 0;
	int z = 0;

	signal(SIGFPE, handle_sigfpe);
	gettimeofday(&start, NULL);
	z = x / y;

	printf("Unreachable statement %d", z);

	return 0;

}