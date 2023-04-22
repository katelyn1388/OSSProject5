//Katelyn Bowers
//OSS - Project 5
//April 25, 2023
//oss.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <sys/msg.h>
#include "resources.h"



#define PERMS 0644

//Message struct
struct my_msgbuf {
	long mtype;
	int resource;
	int intData;
} my_msgbuf;


//Initializing shared memory for seconds
#define sec_key 25217904          


//Initializing shared memory for nano seconds
#define nano_key 25218510

#define max_processes 20


FILE *logFile;

//Process table blocks
struct PCB {
	int occupied;
	pid_t actualPid;
	int currentResources[10];
	int requestedResource;
};

//Process table
struct PCB processTable[20] = {{0}};

struct PCB process;


//Function prototypes
void incrementClock();
void runProcess(int processType);
int help();
static void myhandler(int s);
static int setupinterrupt();
static int setupitimer();
bool isEmpty(int processType);
bool isFull(int processType);
void Enqueue(struct PCB process, int processType);
void Dequeue(int processType);
struct PCB Front(int processType);


//global variables
int totalWorkers = 0, simulWorkers = 0, tempPid = 0, i, nanoIncrement = 50000000, c, fileLines = 1, fileLineMax = 9995;
double averageCpu = 0, averageWait = 0, averageBlockedTime = 0, totalCpu = 0, totalWait = 0, totalBlockedTime = 0, idleTime = 0, idleStart = 0, billion = 1000000000;
struct my_msgbuf message;
struct my_msgbuf received;
int msqid;
key_t key;
char secondsString[20];
char nanoSecondsString[20];
int availableResources[10] = { totalResources };
int resourceRequests[10] = { 0 };
int resourceReleases[10] = { 0 }'


int main(int argc, char **argv) {
	bool doneRunning = false, fileGiven = false;
	char *userFile = NULL;

	while((c = getopt(argc, argv, "hf:")) != -1) {
		switch(c)
		{
			case 'h':
				help();
			case 'f':
				userFile = optarg;
				fileGiven = true;
				break;
		}
	}


	printf("Program is starting...");
	printf("Total resources: %d", totalResources);
	//printf("Middle element in array: %d", availableResources[4]);


	//Opening log file
	if(fileGiven) 
		logFile = fopen(userFile, "w");
	else
		logFile = fopen("logfile.txt", "w");


	//Setting up interrupt
	if(setupinterrupt() == -1) {
		perror("Failed to set up handler for SIGPROF");
		return 1;
	}

	if(setupitimer() == -1) {
		perror("Failed to set up the ITIMER_PROF interval timer");
		return 1;
	}

	signal(SIGINT, myhandler);

	for( ; ; )
	{
		printf("\nProgram running in OSS\n");
		int sec_id = shmget(sec_key, sizeof(int) * 10, IPC_CREAT | 0666);        //Allocating shared memory with key
		if(sec_id <= 0) {                                                       //Testing if shared memory allocation was successful or not
			fprintf(stderr, "Shared memory get failed\n");
			exit(1);
		}


		//Initializing shared memory for nano seconds
		int nano_id = shmget(nano_key, sizeof(int) * 10, IPC_CREAT | 0666);
		if(nano_id <= 0) {
			fprintf(stderr, "Shared memory for nanoseconds failed\n");
			exit(1);
		}


		const int *sec_ptr = (int *) shmat(sec_id, 0, 0);      //Pointer to shared memory address
		if(sec_ptr <= 0) {                               //Testing if pointer is actually working
			fprintf(stderr, "Shared memory attach failed\n");
			exit(1);
		}


		const int *nano_ptr = (int *) shmat(nano_id, 0, 0);
		if(nano_ptr <= 0) {
			fprintf(stderr, "Shared memory attachment for nanoseconds failed\n");
			exit(1);
		}



		//Setting seconds and nanoseconds to initial values
		int * seconds = (int *)(sec_ptr);
		*seconds = 0;

		int * nanoSeconds = (int *)(nano_ptr);
		*nanoSeconds = 0;

		//Message queue setup
		system("touch msgq.txt");
		if((key = ftok("msgq.txt", 'B')) == -1) {
			perror("ftok");
			exit(1);
		}

		if((msqid = msgget(key, PERMS | IPC_CREAT)) == -1) {
			perror("msgget");
			exit(1);
		}

		message.mtype = 1;


		//Getting current seconds and adding 3 to stop while loop after 3 real life seconds
		time_t startTime, endTime;
		startTime = time(NULL);
		endTime = startTime + 3;

		//Max second and nanosecond for random creation time
		int maxNewNano = 500000000;
	
		//First random creation time
		srand(getpid());
		int randomTime = rand() % maxNewNano;
		int chooseTimeNano = *sharedNanoSeconds, chooseTimeSec = *sharedSeconds;
		if((*sharedNanoSeconds + randomTime) < billion)
			chooseTimeNano += randomTime;
		else
		{
			chooseTimeNano = ((*sharedNanoSeconds + randomTime) - billion);
			chooseTimeSec += 1;
		}



		//Keep running until all processes are terminated
		while(!doneRunning) {
			if(*sharedSeconds > chooseTimeSec || (*sharedSeconds == chooseTimeSec && *sharedNanoSeconds >= chooseTimeNano)) {
				if(simulWorkers < 18) {
					
				}
			}

			incrementClock(

		}

		
		
		
		
		//Calculating average stats for all processes
		averageCpu = (totalCpu / (double)totalWorkers);
		averageWait = (totalWait / (double)totalWorkers);
		averageBlockedTime = (totalBlockedTime / (double)totalWorkers);

		fprintf(logFile, "\nAverage CPU utlization: %f%%", averageCpu);
		fprintf(logFile, "\nAverage wait time: %f", averageWait);
		fprintf(logFile, "\nAverage blocked time: %f", averageBlockedTime);
		fprintf(logFile, "\nTotal CPU idle time: %f\n", idleTime);




		//Deallocating shared memory
		shmdt(sec_ptr);
		shmctl(sec_id, IPC_RMID, NULL);

		shmdt(nano_ptr);
		shmctl(nano_id, IPC_RMID, NULL);

		//Closing log file
		fclose(logFile);

		//Closing message queue
		if(msgctl(msqid, IPC_RMID, NULL) == -1) {
			perror("msgctl");
			exit(1);
		}

		printf("\n\n\n\nProgram is done\n");
		return(0);

	

	}
}


int help() {
	printf("\nThis program takes in a text file, uses it as a log file, and prints to it the output of the following:\n");
	printf("When the program starts, it will begin launching user processes that will each choose a random number and either terminate, use their whole time quantum, or use");
	printf("part of its time quantum. It will then send a message back to the main oss process of its choice. \nThe program will terminate after either 100 processes have been");
	printf("launched or 3 real life seconds have passed.");
	printf("\n\nInput Options:");
	printf("\n-h     output a short description of the project and how to run it");
	printf("\n-f     the name of the file for output to be logged in");
	printf("\n\nInput example:");
	printf("\n./oss -f logfile.txt");
	printf("\nThis would launch the program and send all oss output to logfile.txt\n");

	exit(1);
}


void incrementClock(int nanoIncrement) {
	int sec_id = shmget(sec_key, sizeof(int) * 10, IPC_CREAT | 0666);        //Allocating shared memory with key
	if(sec_id <= 0) {                                                       //Testing if shared memory allocation was successful or not
		fprintf(stderr, "Shared memory get failed\n");
		exit(1);
	}


	//Initializing shared memory for nano seconds
	int nano_id = shmget(nano_key, sizeof(int) * 10, IPC_CREAT | 0666);
	if(nano_id <= 0) {
		fprintf(stderr, "Shared memory for nanoseconds failed\n");
		exit(1);
	}


	const int *sec_ptr = (int *) shmat(sec_id, 0, 0);      //Pointer to shared memory address
	if(sec_ptr <= 0) {                               //Testing if pointer is actually working
		fprintf(stderr, "Shared memory attach failed\n");
		exit(1);
	}


	const int *nano_ptr = (int *) shmat(nano_id, 0, 0);
	if(nano_ptr <= 0) {
		fprintf(stderr, "Shared memory attachment for nanoseconds failed\n");
		exit(1);
	}


	//Setting seconds and nanoseconds to initial values
	int * seconds = (int *)(sec_ptr);

	int * nanoSeconds = (int *)(nano_ptr);

	if((*nanoSeconds + nanoIncrement) < billion)
		*nanoSeconds += nanoIncrement;
	else
	{
		*nanoSeconds = ((*nanoSeconds + nanoIncrement) - billion);
		*seconds += 1;
	}
}



/*void runProcess(int processType) {
	//Shared memory attachment
	int sec_id = shmget(sec_key, sizeof(int) * 10, IPC_CREAT | 0666);        //Allocating shared memory with key
	if(sec_id <= 0) {                                                       //Testing if shared memory allocation was successful or not
		fprintf(stderr, "Shared memory get failed\n");
		exit(1);
	}


	//Initializing shared memory for nano seconds
	int nano_id = shmget(nano_key, sizeof(int) * 10, IPC_CREAT | 0666);
	if(nano_id <= 0) {
		fprintf(stderr, "Shared memory for nanoseconds failed\n");
		exit(1);
	}


	const int *sec_ptr = (int *) shmat(sec_id, 0, 0);      //Pointer to shared memory address
	if(sec_ptr <= 0) {                               //Testing if pointer is actually working
		fprintf(stderr, "Shared memory attach failed\n");
		exit(1);
	}


	const int *nano_ptr = (int *) shmat(nano_id, 0, 0);
	if(nano_ptr <= 0) {
		fprintf(stderr, "Shared memory attachment for nanoseconds failed\n");
		exit(1);
	}


	//Setting seconds and nanoseconds to initial values
	int * seconds = (int *)(sec_ptr);

	int * nanoSeconds = (int *)(nano_ptr);

	//Message queue setup
	if((key = ftok("msgq.txt", 'B')) == -1) {
		perror("ftok");
			exit(1);
	}

	if((msqid = msgget(key, PERMS | IPC_CREAT)) == -1) {
		perror("msgget");
		exit(1);
	}

	message.mtype = 1;
	message.timeQuantum = 200000;   //Time quantum of 200,000 ns
	int quantum = message.timeQuantum;



	//Geting process at front of queue
	struct PCB currentProcess = Front(processType);

	//Removing it from queue
	Dequeue(processType);

	//Ensuring there is an actual process to run
	if(currentProcess.actualPid != 0) {
		if(fileLines < fileLineMax) {
			fprintf(logFile, "%d: Dispatching process: %d from queue %d at time %d:%d\n", fileLines, currentProcess.actualPid, processType, *seconds, *nanoSeconds);
			fileLines++;
		}

		int processPid = currentProcess.actualPid;
		message.mtype = processPid;
		message.intData = processPid;

		//converting current time from 2 ints into 1 double
		sprintf(secondsString, "%d", *seconds);
		sprintf(nanoSecondsString, "%d", *nanoSeconds);
		strcat(secondsString, nanoSecondsString);

		//If it's a blocked process but its still blocked by something
		if(processType == 2) {
			if(currentProcess.blockedSeconds < *seconds && currentProcess.blockedNanoSecs < *nanoSeconds) {
				Enqueue(currentProcess, processType);
				incrementClock(nanoIncrement);
				EXIT_SUCCESS;
			}
			//Process's blocked time += the current clock time - when it last ran
			currentProcess.blockedTime += (atof(secondsString) - currentProcess.lastTimeRan);
		}

		
		//Sending message with time quantum to child
		if(msgsnd(msqid, &message, sizeof(my_msgbuf) - sizeof(long), 0) == -1) {
			perror("\n\nmsgsnd to child failed\n\n");
			printf(": %d", currentProcess.actualPid);
			exit(1);
		}


		//Receving message from child
		if(msgrcv(msqid, &received, sizeof(my_msgbuf), getpid(), 0) == -1) {
			perror("\n\nFailed to receive message from child\n");
			exit(1);
		}

		//Getting the time quantum the child sent back/used
		int childsOutput = received.timeQuantum;
		int dispatchTime = 0;

		if(childsOutput == quantum) {                                             //Child used time quantum
			if(fileLines < fileLineMax) {
				fprintf(logFile, "%d: Receiving that process %d ran for %d nanoseconds, using its whole time quantum\n", fileLines, currentProcess.actualPid,
					       	childsOutput);
				fileLines++;
			}
			//Adding quantum used to process's cpuTime Used and blockedTime
			currentProcess.cpuTimeUsed += childsOutput;
			if(fileLines < fileLineMax) {
				fprintf(logFile, "%d: Putting process %d into ready queue\n", fileLines, currentProcess.actualPid);
				fileLines++;
			}

			//Putting process in ready queue
			Enqueue(currentProcess, 1);
			incrementClock(nanoIncrement + childsOutput);

			dispatchTime = (nanoIncrement + childsOutput);
		
			if(fileLines < fileLineMax) {
				fprintf(logFile, "%d: Total time spent in dispatch was %d nanoseconds\n", fileLines, dispatchTime);
				fileLines++;
			}

		} else if(childsOutput < 0) {                                //Child got blocked
			childsOutput = (0 - childsOutput);
			if(fileLines < fileLineMax) {
				fprintf(logFile, "%d: Receiving that process %d ran for %d nanoseconds, not using its whole time quantum\n", fileLines, currentProcess.actualPid, 
						childsOutput);
				fileLines++;
			}

			
			//Putting process in blocked queue
			Enqueue(currentProcess, 2);

			//Setting the last time ran to current time for when it becomes unblocked
			currentProcess.lastTimeRan = atof(secondsString);
			incrementClock(9000000 + childsOutput);

			dispatchTime = (9000000 + childsOutput);
		
			if(fileLines < fileLineMax) {
				fprintf(logFile, "%d: Total time spent in dispatch was %d nanoseconds\n", fileLines, dispatchTime);
				fileLines++;
			}

		} else {                                                                  //Child terminated
			if(fileLines < fileLineMax) {
				fprintf(logFile, "%d: Receiving that process %d ran for %d nanoseconds, not using its whole time quantum and terminating\n", fileLines, 
						currentProcess.actualPid, childsOutput);
				fileLines++;
			}

			dispatchTime = (nanoIncrement + childsOutput);

			currentProcess.cpuTimeUsed += childsOutput;
			//Calculate cpu time and clear the PCB table of this process
			currentProcess.timeRan = ((atof(secondsString) - currentProcess.startTime) / billion);
			currentProcess.cpuTimeUsed = (currentProcess.cpuTimeUsed / billion);

			//Adding to the stats for the end
			totalCpu += (currentProcess.cpuTimeUsed  / currentProcess.timeRan) * 100;
			totalWait += ((currentProcess.timeRan - currentProcess.cpuTimeUsed) / billion);
			totalBlockedTime += (currentProcess.blockedTime / billion);

			if(fileLines < fileLineMax) {
				fprintf(logFile, "\n%d: Clearing process %d's PCB block after termination\n", fileLines, currentProcess.actualPid);
				fileLines++;
			}


			//Emptying the current process's table slot
			currentProcess.occupied = 0;
			currentProcess.actualPid = 0;
			currentProcess.cpuTimeUsed = 0;
			currentProcess.timeRan = 0;
			currentProcess.blockedTime = 0;

			simulWorkers--;

			incrementClock(nanoIncrement + childsOutput);
			if(fileLines < fileLineMax) {
				fprintf(logFile, "\n%d: Total time spent in dispatch was %d nanoseconds\n", fileLines, dispatchTime);
				fileLines++;
			}
		}

	}
}*/


//Queues
struct PCB blockedQueue[max_processes];

//Queue function pointers
int blockedFront = -1;
int blockedRear = -1;



bool isEmpty(int processType) {
	return(blockedFront == -1 && blockedRear == -1);
}


bool isFull(int processType) {
	if((blockedRear + 1) + blockedFront == max_processes) 
		return true;

	return false;
}


//Adding process to queue
void Enqueue(struct PCB process, int processType) {
	if(isFull(2)) 
		return;
		
	if(isEmpty(2)) 
		blockedFront = blockedRear = 0;
	else {
		blockedRear += 1;
		if(blockedRear == max_processes) 
			blockedRear = blockedRear % max_processes;
	}

	blockedQueue[blockedRear] = process;
	
}

//Removing process from queue
void Dequeue(int processType) {
	if(isEmpty(2)) {
		printf("\n\nError: Blocked queue is empty\n\n");
		return;
	} else if(blockedFront == blockedRear)
		blockedRear = blockedFront = -1;
	else {
		blockedFront += 1;
		if(blockedFront == max_processes)
			blockedFront = blockedFront % max_processes;
	}
}

struct PCB Front(int processType) {
	if(blockedRear == -1) {
		printf("\n\nError: Cannot return front of empty queue: blocked\n\n");
		exit(1);
	}
	return blockedQueue[blockedFront];
}



//Handler function for signal to stop program after 60 seconds or with Ctrl + C
static void myhandler(int s) {
	int i, pid;
	for(i = 0; i <= 19; i++) {
		pid = processTable[i].actualPid;
		kill(pid, SIGKILL);
	}

	int sec_id = shmget(sec_key, sizeof(int) * 10, IPC_CREAT | 0666);        //Allocating shared memory with key
	if(sec_id <= 0) {                                                       //Testing if shared memory allocation was successful or not
		fprintf(stderr, "Shared memory get failed\n");
		exit(1);
	}


	//Initializing shared memory for nano seconds
	int nano_id = shmget(nano_key, sizeof(int) * 10, IPC_CREAT | 0666);
	if(nano_id <= 0) {
		fprintf(stderr, "Shared memory for nanoseconds failed\n");
		exit(1);
	}


	const int *sec_ptr = (int *) shmat(sec_id, 0, 0);      //Pointer to shared memory address
	if(sec_ptr <= 0) {                               //Testing if pointer is actually working
		fprintf(stderr, "Shared memory attach failed\n");
		exit(1);
	}


	const int *nano_ptr = (int *) shmat(nano_id, 0, 0);
	if(nano_ptr <= 0) {
		fprintf(stderr, "Shared memory attachment for nanoseconds failed\n");
		exit(1);
	}

	averageCpu = (totalCpu / (double)totalWorkers);
	averageWait = (totalWait / (double)totalWorkers);
	averageBlockedTime = (totalBlockedTime / (double)totalWorkers);

	fprintf(logFile, "\nAverage CPU utlization: %f%%", averageCpu);
	fprintf(logFile, "\nAverage wait time: %f", averageWait);
	fprintf(logFile, "\nAverage blocked time: %f", averageBlockedTime);
	fprintf(logFile, "\nTotal CPU idle time: %f\n", idleTime);

	shmdt(sec_ptr);
	shmctl(sec_id, IPC_RMID, NULL);
	shmdt(nano_ptr);
	shmctl(nano_id, IPC_RMID, NULL);
	fclose(logFile);

	if(msgctl(msqid, IPC_RMID, NULL) == -1) {
		perror("msgctl");
		exit(1);
	}
	exit(1);
}


//Interrupt and timer functions for signal
static int setupinterrupt(void) {
	struct sigaction act;
	act.sa_handler = myhandler;
	act.sa_flags = 0;
	return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL));
}

static int setupitimer(void) {
	struct itimerval value;
	value.it_interval.tv_sec = 60;
	value.it_interval.tv_usec = 0;
	value.it_value = value.it_interval;
	return (setitimer(ITIMER_PROF, &value, NULL));
}
