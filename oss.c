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
#include <resources.h>




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
	pid_t simulatedPid;
	pid_t actualPid;
	double timeInSystem;
	double cpuTimeUsed;      //divide this by 1B at end to get seconds and nanoseconds
	double startTime;
	double lastTimeRan;
	double blockedTime;
	int blockedSeconds;
	int blockedNanoSecs;
	double timeRan;   //For average wait time - multiple this by the quantum, divide by time elapsed in system while it ran    just use cpu time instead
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


int main(int argc, char **argv) {


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



void runProcess(int processType) {
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
}


//Queues
struct PCB readyQueue[max_processes];
struct PCB blockedQueue[max_processes];

//Queue function pointers
int readyFront =  -1; 
int readyRear = -1;
int blockedFront = -1;
int blockedRear = -1;



bool isEmpty(int processType) {
	//1 = ready queue
	if(processType == 1) {
		return(readyFront == -1 && readyRear == -1);
	} else
		return(blockedFront == -1 && blockedRear == -1);
}


bool isFull(int processType) {
	if(processType == 1) {
		if((readyRear + 1) + readyFront == max_processes)
			return true;
	}
	else {
		if((blockedRear + 1) + blockedFront == max_processes) 
			return true;
	}

	return false;
}


//Adding process to queue
void Enqueue(struct PCB process, int processType) {
	if(processType == 1) {
		if(isFull(1)) {
			return;
		}
		if(isEmpty(1)) {
			readyFront = readyRear = 0;
		} else {
			readyRear += 1;
			if(readyRear == max_processes)
				readyRear = readyRear % max_processes;
		}

		readyQueue[readyRear] = process;
	} else {
		if(isFull(2)) {
			return;
		}
		if(isEmpty(2)) 
			blockedFront = blockedRear = 0;
		else {
			blockedRear += 1;
			if(blockedRear == max_processes) 
				blockedRear = blockedRear % max_processes;
		}

		blockedQueue[blockedRear] = process;
	}
}

//Removing process from queue
void Dequeue(int processType) {
	if(processType == 1) {
		if(isEmpty(1)) {
			printf("\n\nError: Ready queue is empty\n\n");
			return;
		} else if(readyFront == readyRear) 
			readyRear = readyFront = -1;
		else {
			readyFront += 1;
			if(readyFront == max_processes)
				readyFront = readyFront % max_processes;
		}
	} else {
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
}

struct PCB Front(int processType) {
	if(processType == 1) {
		if(readyFront == -1) {
			printf("\n\nError: Cannot return front from empty queue: ready\n\n");
			exit(1);
		}
		return readyQueue[readyFront];
	} else {
		if(blockedRear == -1) {
			printf("\n\nError: Cannot return front of empty queue: blocked\n\n");
			exit(1);
		}
		return blockedQueue[blockedFront];
	}
}


