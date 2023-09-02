#include <stdio.h>

#include <string.h>

#include <stdlib.h>	  // exit()

#include <unistd.h>	  // fork(), getpid(), exec()

#include <sys/wait.h> // wait()

#include <signal.h>	  // signal()

#include <fcntl.h>	  // close(), open()



#define MAXLENGTH 10000 // Max size of user command

#define COMMANDLENGTH 1000	 // Max size of command after applying delimiter





// GLOBALS

int curr_len;		// size of commands string returned from parseInput function

pid_t forkChildPID; // Child id used in signal Handling





// This function terminated the use of Ctrl+C and Ctrl+Z in parent process and uses them in Child process

void signalhandler(int signal_num)

{

	if (forkChildPID != 0)

	{

		kill(forkChildPID, SIGKILL);

	}

}





/* this function checks/verifies if given command opts for changing the directory or not 

and if it opts for changing it then chdir function takes a single argument, which is the path to the directory you want to set as the new current working directory */

void verifyWorkingDirectory(char **str)

{

	int result;



	if (str[1] == NULL)

		result = chdir(getenv("HOME"));

	else

		result = chdir(str[1]);





	if (result != 0 && strcmp(str[1], ".") != 0)

		printf("Shell: Incorrect command\n");

}





/*this is process where using fork() system call command is executed , moreover execvp() is also used which terminates if it executes

this is the reason we have used fork() system call , outerwise after execvp() it will not return to our original terminal ...causing problems further*/

void forkingProcess(char **str)

{

	forkChildPID = fork();



	if (forkChildPID < 0)

	{

		printf("Shell: Incorrect command\n"); //fork unsuccessful

		exit(0);

	}

	else if (forkChildPID == 0)

	{

		execvp(str[0], str);



		// If execvp fails exit child process

		printf("Shell: Incorrect command\n");

		exit(0);

	}

	else

	{

		wait(NULL);

	}

}





// Function to parse a command string separated by a delimiter

void parseCommand(char *original, char **commands, const char *delimiter) {

    int i;

    for (i = 0; i < COMMANDLENGTH; i++) {

        commands[i] = strsep(&original, delimiter);



        if (commands[i] == NULL)

            break;

        if (strlen(commands[i]) == 0)

            i--;

    }

    curr_len = i;

}



/* This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter 

(&&, ##, >, or spaces). */

int parseInput(char *original, char **commands) {

    int ret_val = 0;

    

    if (strstr(original, "&&") != NULL) {

        ret_val = 1;

        parseCommand(original, commands, "&&");

    } 

    else if (strstr(original, "##") != NULL) {

        ret_val = 2;

        parseCommand(original, commands, "##");

    } 

    else if (strstr(original, ">") != NULL) {

        ret_val = 3;

        parseCommand(original, commands, ">");

    }

    else{

    	ret_val = 4;

    	parseCommand(original, commands, " ");

    }

    

    return ret_val;

}





// This function will fork a new process to execute a command except cd and exit command

void executeCommand(char **str)

{

	if (strcmp(str[0], "exit") == 0)

	{

		printf("Exiting shell...\n");

		exit(0);

	}

	else if (strcmp(str[0], "cd") == 0)

	{

		verifyWorkingDirectory(str);

		

	}

	else

	{

		forkingProcess(str);

		

	}

}



// This function will run multiple commands in parallel

void executeParallelCommands(char **str)

{

	int total_waits = 0;



	for (int k = 0; k < curr_len; k++)

	{

		char *commands[COMMANDLENGTH];

		char *original = strdup(str[k]);

		int i;



		for (i = 0; i < COMMANDLENGTH; i++)

		{

			commands[i] = strsep(&original, " ");

			if (commands[i] == NULL)

				break;

			if (strlen(commands[i]) == 0)

				i--;

		}



		if (i == 0)

			continue;



		total_waits++;



		// When user uses exit command.

		if (strcmp(commands[0], "exit") == 0)

		{

			printf("Exiting shell...\n");

			exit(0);

		}

		

		else if (strcmp(commands[0], "cd") == 0)

		{

			verifyWorkingDirectory(commands);

		}

		

		else if (strcmp(commands[i - 1], "&&") == 0) { // checking for  parallel command execution operator ==> &&

		    

		    commands[i - 1] = NULL; 		// Remove the "&&" operator

		    forkChildPID = fork();



		    if (forkChildPID < 0) {

		        printf("Shell: Incorrect command\n");

		        exit(1);

		    } else if (forkChildPID == 0) {

		        execvp(commands[0], commands);



		        printf("Shell: Incorrect command\n");

		        exit(1);

		    }

               }

		else

		{

			forkingProcess(commands);

		}

	}



	for (int i = 0; i < total_waits; i++)

		wait(NULL); // waiting for every process to terminated; wait will return random child id after there termination

}



// This function will run multiple commands sequentially

void executeSequentialCommands(char **str)

{

	for (int k = 0; k < curr_len; k++)

	{

		char *commands[COMMANDLENGTH];

		char *original = strdup(str[k]);

		int i;



		for (i = 0; i < COMMANDLENGTH; i++)

		{

			commands[i] = strsep(&original, " ");



			if (commands[i] == NULL)

				break;

			if (strlen(commands[i]) == 0)

				i--;

		}



		if (i == 0)

		{

			continue;

		}



		executeCommand(commands);      // function for executing single command

	}

}



// This function will run a single command with output redirected to an output file specificed by user

void executeCommandRedirection(char **str)

{

	int i = 0;

	char *commands[COMMANDLENGTH];

	for (int k = 0; k < curr_len; k++)

	{

		char *original = strdup(str[k]);



		while (i < COMMANDLENGTH)

		{

			commands[i] = strsep(&original, " ");

			if (commands[i] == NULL)

				break;

			if (strlen(commands[i]) != 0)

				i++;

		}

	}



	int n = i;



	forkChildPID = fork();



	if (forkChildPID < 0)

	{

		printf("Shell: Incorrect command\n");

		exit(0);

	}

	else if (forkChildPID == 0)

	{

		close(STDOUT_FILENO); // Redirecting STDOUT

		open(commands[n - 1], O_CREAT | O_RDWR | O_APPEND);



		commands[n - 1] = NULL; // making name null as it is not required

		execvp(commands[0], commands);



		printf("Shell: Incorrect command\n");

		exit(0);

	}

	else

	{

		wait(NULL);

	}

}



int main()

{

	// Ignore SIGINT signal (CTRL+C) and SIGTSTP signal (CTRL+Z) in parent

	signal(SIGINT, signalhandler);

	signal(SIGTSTP, signalhandler);



	// Initial declarations

	char currentWorkingDirectory[MAXLENGTH];

	char *check = NULL;     // for checking if working directory is there or not



	char *input = NULL; // string recieved from user

	size_t size = 0;



	char *original;

	char *commands[COMMANDLENGTH];



	int flag;

	while (1) // This loop will keep your shell running until user exits.

	{

		// getcwd() return the path of current working directory defined in unistd.h library

		check = getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)); 

		

		if (check != NULL)

		{

			printf("%s$",currentWorkingDirectory); // Print the prompt in format - currentWorkingDirectory$

		}

		else

		{

			printf("Shell: Incorrect command\n"); // Too long path cannot be displayed...

		}



		// accept input with 'getline()'

		getline(&input, &size, stdin); // input string contain delimiter also

		input = strsep(&input, "\n");  // Remove delimiter using strsep in



		if (strlen(input) == 0)

		{

			continue;

		}



		original = strdup(input); // copies in heap and give me the string





		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.

		flag = parseInput(original, commands);

		

		

		

		// checking the type of command  ------------------>

		// 0 is for handling case of space

		// 1 represents parallel execution

		// 2 represents sequential execution

		// 3 represents redirection of output 

		// 4 reprensents single command execution

		

		



		if (flag == 0) 

		{

			continue;

		}



		if (flag == 1)

		{

			executeParallelCommands(commands); // This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)

		}

		else if (flag == 2)

		{

			executeSequentialCommands(commands); // This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)

		}

		else if (flag == 3)

		{

			executeCommandRedirection(commands); // This function is invoked when user wants redirect output of a single command to and output file specificed by user

		}

		else

		{

			executeCommand(commands); // This function is invoked when user wants to run a single commands

		}

	}



	return 0;

}


