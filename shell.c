#include <stdio.h>
#include <unistd.h>
#include  <sys/types.h> 
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define MAX_LINE 80

void printHistory (char ***history, int hCount){
	int commandsToPrint = 0;
	int j = 0;// the word in each history element we are on

	if (hCount < 5){// ensure we only print 5 or less history elements
		commandsToPrint = hCount;
	}
	else{
		commandsToPrint = 5;
	}

	int i = (hCount-1)%5; // history entry index
	while (commandsToPrint>0){
		j = 0;
		printf ("%d ", commandsToPrint);
		while (history[i][j] != NULL){ // print  each word in the entry
                        printf("%s ", history[i][j] );
                        j ++;
                }
                printf ("\n");
		
		i --;		
		commandsToPrint --;
		if (i < 0){// if reached beginning of array

			i = 4; // go backwards from index 0 to 4
		}
	}
	return;
}

volatile sig_atomic_t checkErr = 0; // to store whether storing in history should be skipped (when command results in error)

void skipStoreInHist (int sig){
	checkErr = 1;
}

int main (void){

   char *args[MAX_LINE/2 +1];
   int should_run = 1;
   pid_t pid;
   char input [MAX_LINE/2 +1];
   char ***history = malloc(5 * sizeof(char **));
   int hCount = 0; // keep track of where we need to add next history element
   int checkAnd = 0; // flag to check for & in history
   signal (SIGUSR1, skipStoreInHist); // initializes signal handler to know if command finished with error

   char substring[20];
   int count = -1; // count num of chars  in each word
   int x = 0; // keeps track of where in the input string we are
   int i = 0; //counts number of words in a command
   int j = 0; // to parse the history array when storing a command
   
   while (should_run){

	// reset vars before getting input
	count = -1;
	x = 0;
	i = 0;	
	checkAnd = 0;

    printf ("osh>");
    fflush(stdout);
	if (fgets(input, MAX_LINE/2 + 1, stdin) == NULL) {
        	printf("Error reading input.");
    	}
	input[strcspn(input, "\n")] = '\0';  // Remove newline character

	while (input[x] != '\0' && input[x] != '&' && input[x] != ' '){ // stop parsing when first command is stored in args
		count = -1;
        	substring[0] = '\0';
        	while (input[x] !='\0' && input[x] != ' ' && input[x] != '&'){
                	count ++;
                	substring [count] = (char)(input [x]);
			x ++;
        	}

		// add the next word to args
		substring [++count] = '\0';// the next element is the end of the string, not the one we're on
                args[i] = malloc(strlen(substring)+1);
                strcpy(args[i], substring);
		i ++;
		if (input[x] == '\0' || input[x] == ' '){// if at the end of the first command
                	args[i] = NULL; // since storing of command is finished, the next element in args should be Null to denote end of array
			// no x++ so we actually end on input[x] = \0 or & so while loop terminates
		}
		if (input[x] != '\0'){
			x ++;
        	}
	}// end parsing of string while loop
	
	// checks what the input was and executes command accordingly
        if ( strcasecmp(args[0], "exit") == 0){
		should_run = 0; // changed outside of child so parent's value changes too
	}
	else if (strcasecmp(args[0], "history") == 0 || strcasecmp(args[0], "!!") == 0){
		if (hCount == 0) {
			printf ("No history available.\n");
		}
		else {// if there is history
			if (strcasecmp(args[0], "history") == 0){ // and the command was history
				printHistory (history, hCount);
			}
			else{// the command was "!!" so execute previous command in child process
				pid = fork();
				checkAnd = 0;
				for (int j = 0; history[(hCount-1)%5][j] != NULL; j++) {
                                        if (history[(hCount-1)%5][j] == "&"){
                                                checkAnd = 1;
						history[(hCount-1)%5][j] = NULL;
                                        }
                                }

				if (pid > 0 && checkAnd == 0){
                        		wait (NULL); // parent process waits for child
                		}
                       		if (pid == 0){

					if (execvp(history[(hCount-1)%5][0], history[(hCount-1)%5]) == -1){
                               			perror("execvp failed"); 
                				kill(getppid(), SIGUSR1);
		        			exit (1); // exit child if executing command failed
					} 
				}
			}
		}
	}
	else{// if it's a normal command, execute it in child process
        	pid = fork();
		if (pid > 0 && input[x] != '&'){
			wait (NULL); // parent process waits for child
		}
		if (pid == 0 && execvp(args[0], args) == -1){
			perror("execvp failed"); 
			kill(getppid(), SIGUSR1); // send signal to parent that error occured
			exit (1);
		}
	}

	
    if (pid > 0){ // in parent process

		// check if error occured when executing
		usleep (1000); // to ensure parent can get signal from child if invalid command
		if (checkErr == 1){
			checkErr = 0; // reset it
			continue; // skip the adding to history if command not valid
		}

		// store command in history
		if (strcasecmp(args[0], "history") != 0){
		
			int idx = hCount % 5;

			// Free the previously allocated memory if a command already stored in this index
			if (history[idx] != NULL) {
	        		for (int j = 0; history[idx][j] != NULL; j++) {
        				if (history[idx][j] != "&"){
						free(history[idx][j]);  // Free each argument string
    					}
				}
    			free(history[idx]);  // Free the array itself
			}
		
			// allocate memory space to store element in history
        		history[idx] = malloc((MAX_LINE/2 + 1) * sizeof(char *));
        		if (history[idx] == NULL){ printf("allocation error");
				exit (0);
			} 
	
			// re-store previous command if user entered !!
			if (strcasecmp(args[0], "!!") == 0){
				j = 0;
				while (history[(hCount-1)%5][j] != NULL){
                                	history[idx][j] = strdup (history[(hCount-1)%5][j]);
                        		j++;
				} 
				if (checkAnd == 1){ // if there was a & that we removed doing !!, add it back to current and previous
                                        history[idx][j] = "&";
                                        history[idx][j+1] = NULL;
					history[(hCount-1)%5][j] = "&";
                                }
                                else {
                        		history[idx][j] = NULL; // strdup doesn't handle NULL so we stop loop before copying NULL and set it ourselves  
                        	}
				hCount ++;
                	}
			else{//store normal command
				for (int j = 0; j < i; j++){
					history[idx][j] = strdup (args[j]);
				} 
				if (input[x] == '&'){
					history[idx][i] = "&";
					history[idx][i+1] = NULL;
				}
				else {
					history[idx][i] = NULL;  
				}
				hCount++;
			}
		}
    }
	

}// end first while loop 
return 0;
}//end main
