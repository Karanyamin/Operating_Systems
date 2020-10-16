// File:	mypthread.c

// List all group member's name: Karan Amin, Saavi Dhingra
// username of iLab:
// iLab Server:

#include "mypthread.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


//ALL method headers not in header file go HERE
static void schedule();

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
uint thread_counter = 0;
tcb * run_queue = NULL;
struct itimerval * timer;

//ADD ALL HELPER FUNCTIONS HERE
void create_run_queue(){
	//Creates the run queue with the first node being the current context running
	tcb * node = malloc(sizeof(tcb));

	//Intialize tcb variables
	node->thread_ID = thread_counter++;
	node->stack = malloc(STACK_SIZE);
	//if (getcontext(&(node->context_state)) == -1)
	//	handle_error("getcontext error");
	//node->context_state.uc_link = NULL;
	//node->context_state.uc_stack.ss_sp = node->stack;
	//node->context_state.uc_stack.ss_size = STACK_SIZE;
	//node->context_state.uc_stack.ss_flags = 0;
	node->thread_state = SCHEDULED;
	node->return_value = NULL; 
	node->time_quanta_counter = 0; 
	node->next = NULL;

	run_queue = node;

	//Use sigaction to register handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &schedule;
	sigaction(SIGPROF, &sa, NULL);

	//Create timer
	timer = malloc(sizeof(struct itimerval));

	//Initalize timer to expire after
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = TIMER;

	//Configure timer to expire every
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = TIMER;
}


/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
       // create Thread Control Block
       // create and initialize the context of this thread
       // allocate space of stack for this thread to run
       // after everything is all set, push this thread int
       // YOUR CODE HERE
	//If there's no runqueue, create runqueue and add the main context as the first node. Also create timer
	if (run_queue == NULL) create_run_queue();

	//Create a TCB for the new thread with the associated function
	tcb * node = malloc(sizeof(tcb));

	//Intialize tcb variables
	node->thread_ID = thread_counter++;
	node->stack = malloc(STACK_SIZE);
	node->thread_state = READY;
	node->return_value = NULL;
	node->time_quanta_counter = 0;
	if (getcontext(&(node->context_state)) == -1)
		handle_error("getcontext error");
	node->context_state.uc_link = NULL;
	node->context_state.uc_stack.ss_sp = node->stack;
	node->context_state.uc_stack.ss_size = STACK_SIZE;
	node->context_state.uc_stack.ss_flags = 0;
	makecontext(&(node->context_state), (void *)function, 1, arg);

	//Add this tcb node to the start of the run_queue
	node->next = run_queue;
	run_queue = node;

	//Assign thread ID to thread
	(*thread) = node->thread_ID;

	//Set the timer and go back to main
	setitimer(ITIMER_PROF, timer, NULL);

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE
	
	//STOP THE TIMER
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 0;

	//Find current thread running and save context and change state to READY
	tcb * current_thread = NULL;
	tcb * ptr = run_queue;
	while (ptr != NULL){
		if (ptr->thread_state == SCHEDULED){
			ptr->thread_state = READY;
			current_thread = ptr;

			//Increment time quanta counter for this thread
			current_thread->time_quanta_counter++;
			break;
		}
		ptr = ptr->next;
	}

	//Find the READY thread with the lowest time quanta counter
	ptr = run_queue;
	tcb * highest_priority_thread = NULL;
	int lowest_time_counter = INT_MAX;
	while (ptr != NULL){
		if (ptr->thread_state == READY && ptr->time_quanta_counter < lowest_time_counter){
			highest_priority_thread = ptr;
			lowest_time_counter = ptr->time_quanta_counter;
		}
		ptr = ptr->next;
	}

	if (highest_priority_thread != NULL){
		//Found the thread with the lowest time quanta counter AKA shortest job
		//Change highest priority state to SCHEDULED
		highest_priority_thread->thread_state = SCHEDULED;

		//Set timer and start timer
		timer->it_interval.tv_sec = 0;
		timer->it_interval.tv_usec = TIMER;
		setitimer(ITIMER_PROF, timer, NULL);

		//Swap contexts
		if (swapcontext(&(current_thread->context_state), &(highest_priority_thread->context_state)) == -1)
			handle_error("swapcontext error");
	}

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE
