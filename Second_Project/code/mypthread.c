// File:	mypthread.c

// List all group member's name: Karan Amin, Saavi Dhingra
// username of iLab:
// iLab Server:

#include "mypthread.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


//ALL method headers not in header file go HERE
static void schedule();
void print_run_queue();

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
	node->joining_thread_ID = UINT_MAX; //Means NO ID
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

	//Configure timer to NOT expire after expiring once
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 0;
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
	node->joining_thread_ID = UINT_MAX; //Means NO ID
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
	if (setitimer(ITIMER_PROF, timer, NULL) == -1)
		handle_error("Error setting timer");

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	/*	
		All we have to do here is call the scheduler
		Since the scheduler will make the current running process to ready state
		And save it's context before switching to another context
	*/
	schedule();
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE

	//Stop timer
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 0;
	setitimer(ITIMER_PROF, timer, NULL);

	//Find current ptr that needs to be exited
	tcb * prev = NULL;
	tcb * current_thread_ptr = run_queue;
	while (current_thread_ptr != NULL){
		if (current_thread_ptr->thread_state == SCHEDULED){
			//Found the thread that wants to exit
			//Check if thread is trying to join
			if (current_thread_ptr->joining_thread_ID != UINT_MAX){
				mypthread_t joining_thread_ID = current_thread_ptr->joining_thread_ID;
				//Theres a thread that wants to join
				//Search for that thread's TCB
				tcb * joining_thread_ptr = run_queue;
				while (joining_thread_ptr != NULL){
					if (joining_thread_ptr->thread_ID == joining_thread_ID){
						//Put return value in the joining thread's return value spot
						joining_thread_ptr->return_value = value_ptr;

						//Change thread state of joining thread to READY
						joining_thread_ptr->thread_state = SCHEDULED;

						//Deallocate the TCB on current_thread_ptr from run_queue
						if (prev != NULL) { 
							prev->next = current_thread_ptr->next;
						} else {
							//The first node in run_queue is the target
							run_queue = current_thread_ptr->next;
						}
						
						current_thread_ptr->next = NULL;
						free(current_thread_ptr->stack);
						free(current_thread_ptr);

						//Switch context to the joining thread
						if (setcontext(&(joining_thread_ptr->context_state)) == -1)
							handle_error("Error from setcontext in exit");
					}
					joining_thread_ptr = joining_thread_ptr->next;
				}

				if (joining_thread_ptr == NULL)
					handle_error("There was a joining thread, but couldn't find its TCB");
				
			}

			//No joining thread
			//Put return value in TCB
			current_thread_ptr->return_value = value_ptr;

			//Change state to JUSTFINISHED, later scheduler will change to FINISHED
			current_thread_ptr->thread_state = JUSTFINISHED;

			//Give control to scheduler to determine next thread task
			schedule();
			
		}
		prev = current_thread_ptr;
		current_thread_ptr = current_thread_ptr->next;
	}

	if (current_thread_ptr == NULL){
		handle_error("Couldn't find running thread in exit");
	}
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE

	//Stop and save the TIMER
	struct itimerval saved_timer;
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 0;
	setitimer(ITIMER_PROF, timer, &saved_timer);

	//The ID of the thread hasn't been created
	if (thread > thread_counter){
		//Do something here maybe? Error? Ignore?
	}

	// Parse through run_queue to find thread with the same thread ID
	tcb * prev = NULL;
	tcb * ptr = run_queue;
	while (ptr != NULL){
		if (ptr->thread_ID == thread && ptr->thread_state == FINISHED){
			//Thread is done running, copy return value
			if (value_ptr != NULL)
				(*value_ptr) = ptr->return_value;

			//Detached tcb block and free the block
			if (prev != NULL) { 
				prev->next = ptr->next;
			} else {
				//The first node in run_queue is the target
				run_queue = ptr->next;
			}
			
			ptr->next = NULL;
			free(ptr->stack);
			free(ptr);

			//Reset timer to remaining time
			saved_timer.it_interval.tv_sec = 0;
			saved_timer.it_interval.tv_usec = 0;
			if (setitimer(ITIMER_PROF, &saved_timer, NULL) == -1)
				handle_error("Error setting timer");

			return 0;
		} else if (ptr->thread_ID == thread && ptr->thread_state != FINISHED){
			//Found thread to join, but it isn't done running
			//Find current thread running
			tcb * current_thread = NULL;
			tcb * current_ptr = run_queue;
			while (current_ptr != NULL){
				if (current_ptr->thread_state == SCHEDULED){
					current_thread = current_ptr;
					break;
				}
				current_ptr = current_ptr->next;
			}

			//Add current thread's ID to the joinging ID of the thread it's joining
			ptr->joining_thread_ID = current_thread->thread_ID;

			//Block current thread from running
			current_thread->thread_state = JUSTBLOCKED;

			//Let the scheduler schedule anothe thread to run
			schedule();

			//If swapcontext ever reaches here, then pthread_exit changed the current thread state to SCHECDULEd
			//That means the thread we were just trying to join will now be FINISHED and deallocated
			//The return value it was trying to save is now copied into current_thread's return value
			if (value_ptr != NULL)
				(*value_ptr) = current_thread->return_value;
			current_thread->return_value = NULL;

			//Timer is set to remaining time before the thread was blocked
			saved_timer.it_interval.tv_sec = 0;
			saved_timer.it_interval.tv_usec = 0;
			if (setitimer(ITIMER_PROF, &saved_timer, NULL) == -1)
				handle_error("Error setting timer");
			
			return 0;

		}
		prev = ptr;
		ptr = ptr->next;
	}

	if (ptr == NULL){
		//Thread ID has been joined on before and been deallocated
		//Double Join Error
		//Do Something
		handle_error("Double join on a thread");
	}
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex
	
	// YOUR CODE HERE


	//Since every mutex needs a thread ID associated to it when its locked
	//We can call create run_queue to add the main thread to the run_queue
	if (run_queue == NULL)
		create_run_queue();


	mutex->status = 0;
	mutex->list_capacity = 10;
	mutex->next_free_spot = 0;
	mutex->thread_ID_list = (mypthread_t *)malloc(sizeof(mypthread_t) * mutex->list_capacity);
	int i;
	for (i = 0; i < mutex->list_capacity; i++){
		mutex->thread_ID_list[i] = UINT_MAX;
	}
	mutex->thread_who_locked = UINT_MAX;

	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
		
		//Stop the timer
		struct itimerval saved_timer;
		timer->it_value.tv_sec = 0;
		timer->it_value.tv_usec = 0;
		if (setitimer(ITIMER_PROF, timer, &saved_timer) == -1)
			handle_error("Error setting timer");
		
		
		tcb * current_thread = NULL;
		tcb * ptr = run_queue;
		while (ptr != NULL){
			if (ptr->thread_state == SCHEDULED){
				current_thread = ptr;
				break;
			}
			ptr = ptr->next;
		}
		

		//Error finding currently running thread
		if (ptr == NULL)
			handle_error("Couldn't find a currently running thread");

		
		while (__atomic_test_and_set(&(mutex->status), 0) == 1){
			//printf("IM in locked\n");
			//Mutex is locked
			
			//Check if list is full
			if (mutex->next_free_spot >= mutex->list_capacity){
				//Expand List
				mutex->list_capacity = 2 * mutex->list_capacity;
				mutex->thread_ID_list = realloc(mutex->thread_ID_list, mutex->list_capacity);
				if (mutex->thread_ID_list == NULL)
					handle_error("Error during realloc");

				//Initialize new alloced array
				int i;
				for (i = mutex->next_free_spot; i < mutex->list_capacity; i++){
					mutex->thread_ID_list[i] = UINT_MAX;
				}
			}

			//Add thread ID to list
			mutex->thread_ID_list[mutex->next_free_spot++] = current_thread->thread_ID;

			//Block current thread
			current_thread->thread_state = JUSTBLOCKED;

			//Schedule another thread to run
			schedule();

			
			//Confirm timer is off before comparing lock again
			timer->it_value.tv_sec = 0;
			timer->it_value.tv_usec = 0;
			if (setitimer(ITIMER_PROF, timer, &saved_timer) == -1)
				handle_error("Error setting timer");		
			
		}	
		
		mutex->thread_who_locked = current_thread->thread_ID;

		
		//Restore timer and return
		saved_timer.it_interval.tv_sec = 0;
		saved_timer.it_interval.tv_usec = 0;
		if (setitimer(ITIMER_PROF, &saved_timer, NULL) == -1)
			handle_error("Error setting timer");
		
		return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.
	// YOUR CODE HERE
	
	//Stop and save timer
	struct itimerval saved_timer;
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 0;
	if (setitimer(ITIMER_PROF, timer, &saved_timer) == -1)
		handle_error("Error setting timer");
	
	
	//Find current thread running
	tcb * current_thread = NULL;
	tcb * ptr = run_queue;
	while (ptr != NULL){
		if (ptr->thread_state == SCHEDULED){
			current_thread = ptr;
			break;
		}
		ptr = ptr->next;
	}
	

	//Check if current thread is allowed to unlock this mutex
	if (mutex->thread_who_locked != current_thread->thread_ID)
		handle_error("Another thread who didn't lock the mutex tried to unlock it");
	

	//Change status to 0 (unlocked)
	mutex->status = 0;
	mutex->thread_who_locked = UINT_MAX;

	//Put all threads blocked into ready state
	int i;
	for (i = 0; i < mutex->next_free_spot; i++){
		mypthread_t temp_ID = mutex->thread_ID_list[i];

		//Find thread in run queue and change state to READY
		tcb * ptr = run_queue;
		while (ptr != NULL){
			if (ptr->thread_ID == temp_ID){
				ptr->thread_state = READY;
				break;
			}
			ptr = ptr->next;
		}
		
		//Error if ID not found in queue
		if (ptr == NULL)
			handle_error("Couldn't find thread ID");
	}

	//Clear the block list for this mutex
	for (i = 0; i < mutex->next_free_spot; i++){
		mutex->thread_ID_list[i] = UINT_MAX;
	}
	mutex->next_free_spot = 0;
	
	//Restore timer and return
	saved_timer.it_interval.tv_sec = 0;
	saved_timer.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_PROF, &saved_timer, NULL) == -1)
		handle_error("Error setting timer");
	
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	free(mutex->thread_ID_list);
	mutex->thread_ID_list = NULL;

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
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 0;
	setitimer(ITIMER_PROF, timer, NULL);

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
		} else if (ptr->thread_state == JUSTBLOCKED){
			ptr->thread_state = BLOCKED;
			current_thread = ptr;

			break;
		} else if (ptr->thread_state == JUSTFINISHED){
			ptr->thread_state = FINISHED;
			current_thread = ptr;

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
		timer->it_value.tv_sec = 0;
		timer->it_value.tv_usec = TIMER;
		
		if (setitimer(ITIMER_PROF, timer, NULL) == -1)
			handle_error("Error setting timer");
		

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
void print_run_queue(){
	printf("=========START===========\n");
	tcb * ptr = run_queue;
	while (ptr != NULL){
		printf("Thread ID [%i], Joining Thread [%i], Thread State [%d], Thread time quanta [%d]\n", ptr->thread_ID, ptr->joining_thread_ID, ptr->thread_state, ptr->time_quanta_counter);
		ptr = ptr->next;
	}
	printf("=========END===========\n");

}

void print_mutex_queue(mypthread_mutex_t *mutex){
	printf("++++++++MUTEX LIST START++++++++++\n");
	printf("Mutex staus [%d]\n", mutex->status);
	printf("List capacaity [%d]\n", mutex->list_capacity);
	printf("Next free spot [%d]\n", mutex->next_free_spot);
	printf("Thread who locked [%d]\n", mutex->thread_who_locked);
	printf("Threads waiting [");
	int i;
	for (i = 0; i < mutex->next_free_spot; i++){
		printf("%d, ", mutex->thread_ID_list[i]);
	}
	printf("]\n++++++++MUTEX LIST END++++++++++\n");

}
