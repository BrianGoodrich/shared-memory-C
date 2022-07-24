// Brian Goodrich - The purpose of this program is to find a pair of numbers given as command line arguments that add up to 19, a file must be provided to write to. This program will also store the pair that adds to 19 in shared memory which will be accessed and outputted by the parent process.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void help();

#define SHMKEY 503689

#define BUFF_SZ (sizeof(int) * 2)

int main(int argc, char **argv)
{

	int option;

	while ((option = getopt(argc, argv, "h")) != -1)
	{

		switch (option)

		case 'h':
			help();
		return 0;
	}

	if (argc < 4)
	{
		printf("You need to provide a file to be written to, and at least 2 numbers to sum. Program will terminate.");
		return EXIT_FAILURE;
	}

	char *outputFile = argv[1];
	FILE *outputFilePointer;
	int count = 0;
	outputFilePointer = fopen(outputFile, "a");

	if (outputFilePointer == NULL)
	{
		printf("Bad file. %s cannot be opened", outputFile);
		return EXIT_FAILURE;
	}

	// Here we will allocate enough shared memory for 2 integers.
	int shmid = shmget(SHMKEY, BUFF_SZ, 0777 | IPC_CREAT);

	if (shmid == -1)
	{
		printf("Error in shmget"); // Error in shared memory.
		exit(1);
	}

	char *paddr = (char *)(shmat(shmid, 0, 0));
	int *pint = (int *)(paddr);

	pint[0] = -2;
	pint[1] = -2;

	pid_t childPid = fork();

	if (childPid == -1)
	{
		printf("Error: Failed to fork");
		return 1;
	}
	// CHILD Process
	if (childPid == 0)
	{

		int *inputArr = 0; // Create pointer for our dynamic array

		inputArr = malloc(sizeof(int) * (argc - 2)); // Dynamically allocate array to the amount of args that were passed in minus the file that was passed in for input also.
		if (inputArr <= 0)
		{ // Check to make sure our memory could be allocated
			printf("Could not allocate memory.\n");
			return 1;
		}

		int x;
		for (x = 2; x < argc; x++)
		{ // This for loop will set our dynamic array values to the arguments.
			inputArr[x - 2] = atoi(argv[x]);
		}
		shmid = shmget(SHMKEY, BUFF_SZ, 0777 | IPC_CREAT);

		if (shmid == -1)
		{
			printf("Error in child process with shmget");
			return 1;
		}

		paddr = (char *)(shmat(shmid, 0, 0));
		pint = (int *)(paddr);

		if (pint[0] != -2 && pint[1] != -2)
		{
			printf("Error: shared memory error both integers not equal to -2.");
			return 1;
		}

		int y;
		int i;

		for (y = 0; y < (argc - 2); y++)
		{
			for (i = 0; i < (argc - 2); i++)
			{
				if ((inputArr[y] + inputArr[i]) == 19 && x != i)
				{
					pint[0] = inputArr[y];
					pint[1] = inputArr[i];
					fprintf(outputFilePointer, "Pair: %i %i ", inputArr[y], inputArr[i]);
					count++;
					break;
				}

				if (count > 0)
				{
					break;
				}
			}
			if (count > 0)
			{
				break;
			}
		}
		// If this condition fires then there was no combination that matched for 19 and it will set the shared memory to -1 and then terminate.
		if (count == 0)
		{
			pint[0] = -1;
			pint[1] = -1;
			shmdt(pint);
			return 1;
		}

		fclose(outputFilePointer);
		free(inputArr);
		inputArr = 0;

		shmdt(pint);
		return 0;
	}
	else
	{ // Parent process

		// Now we need to wait for the child process to end in the parent before we output the changes made by the child process.
		int stat = 0;
		wait(&stat);
		// if(WIFEXITED(stat)){
		//	printf("Exit status: %d\n", WEXITSTATUS(stat));
		// }
		// else if (WIFSIGNALED(stat)){
		//	psignal(WTERMSIG(stat), "Exit signal");
		// }

		paddr = (char *)(shmat(shmid, 0, 0));
		pint = (int *)(paddr);

		if (pint[0] == -1 && pint[1] == -1)
		{
			printf("No pair found");
		}
		else if (pint[0] == -2 && pint[1] == -2)
		{
			printf("Error: child did not change values properly for shared memory.");
		}
		else
		{
			printf("Pair found by child: %i, %i", pint[0], pint[1]);
		}

		shmdt(pint);
		shmctl(shmid, IPC_RMID, NULL);

		fclose(outputFilePointer);
	}

	return EXIT_SUCCESS;
}

void help()
{
	printf("This program requires a file to write to as well as several integers. This program will take the integers and see if any of the two integers can be added to equal 19 and if there are two that would add to 19 they will be written to the provided file.");
}
