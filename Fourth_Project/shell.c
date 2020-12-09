#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define numberOfCommands 10

#define sizeOfEachCommand 1000

//All implicit declarations
int execute_command(char * command);
void signal_handler();

char current_directory[4096]; //Should be called cwd but too
char previous_directory[4096];

pid_t children[sizeOfEachCommand];
int number_of_children;

bool stop_making_children = false;

int main(int argc, char** argv){

    //Use sigaction to register handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa, NULL);

    //Initialize children array
    memset(children, 0, sizeof(children));
    number_of_children = 0;

    getcwd(current_directory, sizeof(current_directory));

    printf("Starting Shell...\n\n");

    printf("%s $ ", current_directory);

    char * command_given = NULL;
    size_t command_length = 0;

    while (getline(&command_given, &command_length, stdin) != -1){


        if (strcmp(command_given , "exit\n") != 0){   
            //printf("From Console: [%s]\n", command_given);

            //Create an array of strings to hold each command
            char ** all_commands = malloc(sizeof(char*) * numberOfCommands);
            int i;
            for (i = 0; i < numberOfCommands; i++){
                all_commands[i] = malloc(sizeof(char) * sizeOfEachCommand);
                memset(all_commands[i], '\0', sizeof(char) * sizeOfEachCommand);
            }
            int counter = 0;

            //First copy the command_given since strtok is destructive
            char * command_given_copy = malloc(sizeof(char) * (strlen(command_given) + 1));
            strcpy(command_given_copy, command_given);

            //Tokenize the command_given into multiple single commands using ";" as a delimiter
            char * command_token = strtok(command_given_copy, ";\n");
            
            //Run execute command on the first token
            if (command_token != NULL){
                strcpy(all_commands[counter++], command_token);
                //printf("Token: [%s]\n", command_token);
            }

            //If there are multiple tokens, run the command sequentially 
            while ((command_token = strtok(NULL, ";\n")) != NULL){
                //execute_command(command_token);
                strcpy(all_commands[counter++], command_token);
                //printf("Token: [%s]\n", command_token);
            }

            //Execute all the commands
            for (i = 0; i < counter; i++){
                if (execute_command(all_commands[i]) == -1){
                    break;
                }
            }
            
            //Free all malloced memory
            free(command_given_copy);
        } else {
            //exit was given as a command, exit shell
            break;
        }

        //Update pathname if it changed and print it out
        memset(current_directory, 0, sizeof(current_directory));
        getcwd(current_directory, sizeof(current_directory));
        printf("%s $ ", current_directory);

        //Reset command_given and command_length
        free(command_given);
        command_length = 0;
    } 
    printf("End of shell...Goodbye\n");

    return 0;
}

/*  Given a string str, split the string into two parts seperated by > or >>
    store the first part in the first index of the result array and the second
    part in the second index. Store single, multi, no redirect in the third index
*/

char** splitRedirect(char* str){

    //Create the result array and initialize it
    char** result = malloc(sizeof(char*) * 3);
    int i;
    for (i = 0; i < 3; i++){
        result[i] = malloc(sizeof(char) * sizeOfEachCommand);
        memset(result[i], '\0', sizeof(char) * sizeOfEachCommand);
    }

    //Create a copy of str since strtok is destructive
    char* str_cpy = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(str_cpy, str);

    //Create a buffer to store the result of strtok
    char* buffer;

    //Tokenize str_cpy by > or >> and store in result
    if ((buffer = strtok(str_cpy, ">")) != NULL){
        strcpy(result[0], buffer);
    }
    if ((buffer = strtok(NULL, ">")) != NULL){
        strcpy(result[1], buffer);
    }
    if ((buffer = strtok(NULL, ">")) != NULL){
        strcpy(result[2], buffer);
    }
    
    //Check if no > or >> was found and return if theree was none
    if (strlen(result[0]) == 0 || strlen(result[1]) == 0){
        strcpy(result[2], "none");
        return result;
    }

    //Check if its it was a single > or >>
    if ((strlen(result[0]) + strlen(result[1]) + 1) == strlen(str)) {
        //Single >
        strcpy(result[2], "single");
    } else {
        strcpy(result[2], "double");
    }
    
    return result;
}

/*  Split str into multiple substrings seperated by |
    store the result in an array and store the size of 
    the arraay in resultSize
*/
char** splitPipes(char* str, int* resultSize){
    //Create the result array and intialize is to the sizeOfEachCommand
    char** result = malloc(sizeof(char*) * sizeOfEachCommand);

    //Create a copy of str since strtok is destructive
    char* str_cpy = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(str_cpy, str);

    //Start tokenizing str_cpy by | and store the tokens in result, but first create a buffer and a counter for result
    char* buffer;
    int counter = 0;

    if ((buffer = strtok(str_cpy, "|")) != NULL){
        result[counter] = malloc(sizeof(char) * sizeOfEachCommand);
        strcpy(result[counter++], buffer);
    }

    //Loop until end of str_cpy
    while ((buffer = strtok(NULL, "|")) != NULL){
        result[counter] = malloc(sizeof(char) * sizeOfEachCommand);
        strcpy(result[counter++], buffer);
    }

    //store counter in resultSize and return
    *resultSize = counter;
    return result;
}

pid_t create_process(int inFromLast, int outToNext, char* command){
    
    pid_t child;

    if ((child = fork()) == 0){
        if (inFromLast != STDIN_FILENO){
            dup2(inFromLast, STDIN_FILENO);
            close(inFromLast);
        }
        if (outToNext != STDOUT_FILENO){
            dup2(outToNext, STDOUT_FILENO);
            close(outToNext);
        }

        //Create a copy of command to tokenize
        char * command_cpy = malloc(sizeof(char) * (strlen(command) + 1));
        strcpy(command_cpy, command);

        char* token = strtok(command_cpy, " ");

        //First token will be the program name/path
        char* file = malloc(sizeof(char) * sizeOfEachCommand);
        strcpy(file, token);

        //Copy all the args
        char** argv = malloc(sizeof(char*) * sizeOfEachCommand);
        int i;
        for (i = 0; i < sizeOfEachCommand; i++){
            argv[i] = NULL;
        }
        int counter = 0;

        //Copy name of program to argv[0]
        argv[counter] = malloc(sizeof(char) * sizeOfEachCommand);
        strcpy(argv[counter++], token);

        char* buffer; 
        while ((buffer = strtok(NULL, " ")) != NULL){
            argv[counter] = malloc(sizeof(char) * sizeOfEachCommand);
            strcpy(argv[counter++], buffer);
        }

        //Execute process
        execvp(file, argv);
        printf("%s command not found\n", file);
        exit(1);
    }
    return child;
}

void wait_for_children(){
    pid_t temp;
    int i;
    while ((temp = wait(NULL)) != -1){
        for (i = 0; i < number_of_children; i++ ){
            if (children[i] == temp){
                children[i] = 0; //Child terminated
            }
        }
    }

    if (errno != ECHILD){
        printf("Something weird happened\n");
    }

}

void signal_handler(){
    //Terminate all child processes if pid_t children is not NULL
    if (number_of_children != 0){
        int i;

        for (i = 0; i < number_of_children; i++){
            if (children[i] != 0){
                printf("Sendinf it twice\n");
                //kill(children[i], SIGINT);
            }
        }
        
        //Communiate to stop making more child processes
        stop_making_children = true;
    }

}

int execute_pipe_array(char** pipeArray, int numPipes, char** redirectArray){
    int i;
    int inFromLast = STDIN_FILENO;
    int fd[2];
    //pid_t lastChild;

    for (i = 0; i < numPipes - 1; i++){
        pipe(fd);

        //lastChild = create_process(inFromLast, fd[1], pipeArray[i]);

        //Check if we should make new children
        if (stop_making_children){
            //A signal has been sent to all children to terminate. Wait on the children and return
            wait_for_children();
            close(fd[0]);
            close(fd[1]);

            //Allow future commands to make processes
            stop_making_children = false;

            return -1;
        }

        //Add child to global children array to be used by signal handler
        children[number_of_children++] = create_process(inFromLast, fd[1], pipeArray[i]);

        close(fd[1]);

        inFromLast = fd[0];
    }

    //Last process, if there's a redirect to a file do so, if not direct output of last process to STDOUT
    if (strcmp(redirectArray[2], "none") == 0){
        //lastChild = create_process(inFromLast, STDOUT_FILENO, pipeArray[i]);
        if (stop_making_children){
            //A signal has been sent to all children to terminate. Wait on the children and return
            wait_for_children();
            close(fd[0]);

            //Allow future commands to make processes
            stop_making_children = false;

            return -1;
        }
        children[number_of_children++] = create_process(inFromLast, STDOUT_FILENO, pipeArray[i]);
    } else if (strcmp(redirectArray[2], "single") == 0){
        //Extract redirect file name by tokenizing it, first make a copy
        char * redirect_file_copy = malloc(sizeof(char) * (strlen(redirectArray[1]) + 1));
        strcpy(redirect_file_copy, redirectArray[1]);

        //Extract the file name
        char * file = strtok(redirect_file_copy, " ");
    
        int redirectfd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        //lastChild = create_process(inFromLast, redirectfd, pipeArray[i]);
        if (stop_making_children){
            //A signal has been sent to all children to terminate. Wait on the children and return
            wait_for_children();
            close(fd[0]);

            //Allow future commands to make processes
            stop_making_children = false;

            return -1;
        }
        children[number_of_children++] = create_process(inFromLast, redirectfd, pipeArray[i]);

        //Close and free any memory
        close(redirectfd);
        free(redirect_file_copy);
    } else if (strcmp(redirectArray[2], "double") == 0){
        //Extract redirect file name by tokenizing it, first make a copy
        char * redirect_file_copy = malloc(sizeof(char) * (strlen(redirectArray[1]) + 1));
        strcpy(redirect_file_copy, redirectArray[1]);

        //Extract the file name
        char * file = strtok(redirect_file_copy, " ");

        int redirectfd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
        //lastChild = create_process(inFromLast, redirectfd, pipeArray[i]);
        if (stop_making_children){
            //A signal has been sent to all children to terminate. Wait on the children and return
            wait_for_children();
            close(fd[0]);

            //Allow future commands to make processes
            stop_making_children = false;

            return -1;
        }
        children[number_of_children++] = create_process(inFromLast, redirectfd, pipeArray[i]);

        //Close and free any memory
        close(redirectfd);
        free(redirect_file_copy);
    } else {
        printf("Error in piping to redirect in execute_pipe_array\n");
    }
    
    close(fd[0]);

    //Wait for all the children to finish
    wait_for_children();
    //waitpid(lastChild, 0, 0);

    //clear children array
    memset(children, 0, sizeof(children));
    number_of_children = 0;
    
    //Allow next process to create children Return success
    stop_making_children = false;
    return 0;
}

int execute_command(char * command){

    int returnCode;
    
    //Copy command
    char * command_copy = malloc(sizeof(char) * (strlen(command) + 1));
    memset(command_copy, '\0', sizeof(char) * (strlen(command) + 1));
    strcpy(command_copy, command);

    //Tokenize the command to get the command and see if it's "cd"
    char * token = strtok(command_copy, " ");

    //Check if the command is change directory
    if (strcmp(token, "cd") == 0){
        //Get the directory to change to
        char * new_path = strtok(NULL, " ");
        bool freePath = false;
        
        if (new_path == NULL || strcmp(new_path, "~") == 0){
            //No path given, default to home 
            new_path = getenv("HOME");
        } else if (strcmp(new_path, "-") == 0){
            //Change directory to what it was previously
            freePath = true;
            new_path = malloc(sizeof(char) * (strlen(previous_directory) + 1) );
            memset(new_path, '\0', sizeof(char) * (strlen(previous_directory) + 1));
            strcpy(new_path, previous_directory);
        }
        
        //Check if there are any more arguements
        char * arguments = strtok(NULL, " ");
        if (arguments != NULL){
            //Too many argurments
            printf("shell: cd: too many arguments\n");
            return 0;
        }

        //Save current Directory in buffer
        char prev_buffer[4096];
        memset(prev_buffer, '\0', sizeof(prev_buffer));
        strcpy(prev_buffer, current_directory);

        //Change the current working directory and return
        if (chdir(new_path) == -1){
            printf("shell: cd: %s: No such file or directory\n", new_path);
            return 0;
        }

        //Copy old directory into previous_directory
        memset(previous_directory, '\0', sizeof(previous_directory));
        strcpy(previous_directory, prev_buffer);

        //free memory
        if (freePath){
            //Switched to old path, print out new path too, then free
            char directory[4096];
            memset(directory, '\0', sizeof(directory));
            getcwd(directory, sizeof(directory));
            printf("%s\n", directory);
            free(new_path);
        }
        
        returnCode = 0;

    } else if (strcmp(token, "pwd") == 0){
        char directory[4096];
        memset(directory, '\0', sizeof(directory));
        getcwd(directory, sizeof(directory));
        printf("%s\n", directory);

        returnCode = 0;

    } else {
        //Call splitRedirect to split the command by > or >> if it needs to
        char ** redirectArray = splitRedirect(command);

        //Call splitPipe to split the first portion of the redirect by |
        int numberOfPipes;
        char ** pipeArray = splitPipes(redirectArray[0], &numberOfPipes);

        returnCode = execute_pipe_array(pipeArray, numberOfPipes, redirectArray);


        /*
        //Keep track of two sub process's ID
        pid_t firstChild;
        pid_t secondChild;

        //Execute each sub command in pipeArray
        int lpipe[2]; //Pipe coming in from the previous process
        int rpipe[2]; //Pipe goin to the next process

        //Create first right pipe and check for error
        if (pipe(rpipe) == -1){
            return;
        }

        //Fork twice to start the two children
        if ((firstChild = fork()) == -1){
            //Couldn't fork return
            return;
        }
        

        //Let the parent fork again
        if (firstChild > 0 && (secondChild = fork()) == -1){
            //Second fork failed, kill the first child and return
            kill(firstChild, SIGKILL);
            return;
        }

        //Initialize the first child with approprite pipes
        if (firstChild == 0){
            //Set
        }
        */

        /*
        if (strcmp(redirectArray[2], "none") == 0){
            printf("There are no redirects\n");
        } else if (strcmp(redirectArray[2], "single") == 0) {
            printf("There is a single redirect, [%s] [%s]\n", redirectArray[0], redirectArray[1]);
        } else if (strcmp(redirectArray[2], "double") == 0) {
            printf("There is a doule redirect, [%s] [%s]\n", redirectArray[0], redirectArray[1]);
        }

        //Print the pipes
        
        printf("Pipes: ");
        for (i = 0; i < numberOfPipes; i++){
            printf("-> [%s] ", pipeArray[i]);
        }
        printf("\n");
        */
        //free memory
        int i; 
        for (i = 0; i < 3; i++){
            free(redirectArray[i]);
        }
        free(redirectArray);
        for (i = 0; i < numberOfPipes; i++){
            free(pipeArray[i]);
        }
        free(pipeArray);
    }
    free(command_copy);

    return returnCode;

}
