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
	int choice;     //1 = request, 2 = release, 3 = terminate
	int pid;
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
	pid_t pid;
	int currentResources[10];
	int requestedResource;
};

//Process table
struct PCB processTable[20] = {{0}};

struct PCB process;


//Function prototypes
int help();
static void myhandler(int s);
static int setupinterrupt();
static int setupitimer();
bool deadlock(int available[], int max[][10], int allocation[][10], int numberOfProcesses, int numberOfResources);


//global variables
int totalWorkers = 0, simulWorkers = 0, tempPid = 0, i, c, fileLines = 1, fileLineMax = 99995, messageReceived, billion = 1000000000, resourceRequest = 0;
int processChoice = 0, tempValue = 0, currentPid, grantedInstantly = 0, blocked = 0, queueSize, j, nanoIncrement = 50000, grantedRequests = 0, deadlockTime = 1;
struct my_msgbuf message;
struct my_msgbuf received;
int msqid;
key_t key;
char secondsString[20];
char nanoSecondsString[20];
int maxResources[18][10];
int availableResources[10] = {totalResources, totalResources, totalResources, totalResources, totalResources, totalResources, totalResources, totalResources, totalResources, totalResources };
int allocatedResources[18][10] = {{ 0 }};
int resourceRequests[10] = { 0 };
int resourceReleases[10] = { 0 };


int main(int argc, char **argv) {
	bool fileGiven = false, verboseOn = false, messageReceivedBool = false, doneRunning = false, doneCreating = false;
	char *userFile = NULL;
	struct PCB currentProcess;

	while((c = getopt(argc, argv, "hvf:")) != -1) {
		switch(c)
		{
			case 'h':
				help();
			case 'v':
				verboseOn = true;
				break;
			case 'f':
				userFile = optarg;
				fileGiven = true;
				break;
		}
	}


	printf("Program is starting...");


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

		//Process table initial values
		for(i = 0; i < 18; i++) {
			processTable[i].occupied = 0;
			processTable[i].requestedResource = -1;
		}

		//initializing max request table for each process
		for(i = 0; i < 18; i++) {
			for(j = 0; i < 10; j++) {
				maxResources[i][j] = 10;
			}
		}



		//Getting current seconds and adding 3 to stop while loop after 3 real life seconds
		time_t startTime, endTime;
		startTime = time(NULL);
		endTime = startTime + 5;
		
		//Max second and nanosecond for random creation time
		int maxNewNano = 500000000;
	
		//First random creation time
		srand(getpid());
		int randomTime = rand() % maxNewNano;
		int chooseTimeNano = *seconds, chooseTimeSec = *seconds;
		if((*nanoSeconds + randomTime) < billion)
			chooseTimeNano += randomTime;
		else
		{
			chooseTimeNano = ((*seconds + randomTime) - billion);
			chooseTimeSec += 1;
		}

		printf("\n\nRight before loop");

		//Keep running until 40 processes have run or 5 real-life seconds have passed
		while(!doneRunning) {
			//If it's time to make another child, do so as long as there's less than 18 simultaneous already running
			if(*seconds > chooseTimeSec || (*seconds == chooseTimeSec && *nanoSeconds >= chooseTimeNano)) {
				if((simulWorkers < 18) && !doneCreating) {
					printf("\nCreating child\n");
					for(i = 0; i < 18; i++) {
						if(processTable[i].occupied == 0) {
							printf("Checking if empty");
							currentProcess = processTable[i];
							break;
						}
					}

					printf("\nForking a child");

					//Forking child
					tempPid = fork();

					//Filling out process table for child process
					currentProcess.occupied = 1;
					currentProcess.pid = tempPid;

					char* args[] = {"./worker", 0};

					printf("Creating a child - after fork");

					//Execing child off
					if(tempPid < 0) 
						printf("Fork failed");
					else if(tempPid == 0) {
						printf("execing a child");
						if(execlp(args[0], args[0], NULL) == -1) {
							printf("Exec failed, terminating");
							exit(1);
						}
						return 0;
					}

					simulWorkers++;
					totalWorkers++;

					//Setting new random time for next process creation
					randomTime = rand() % maxNewNano;
					chooseTimeNano = *seconds, chooseTimeSec = *seconds;
					if((*nanoSeconds + randomTime) < billion)
						chooseTimeNano += randomTime;
					else
					{
						chooseTimeNano = ((*seconds + randomTime) - billion);
						chooseTimeSec += 1;
					}

				}
			}

			if(totalWorkers > 40 || time(NULL) > endTime) 
				doneCreating = true;

			
			if((messageReceived = msgrcv(msqid, &received, sizeof(my_msgbuf), getpid(), IPC_NOWAIT) == -1) == -1) {
				perror("\n\nFailed to receive message from child\n");
				exit(1);
			//If a process sent a message
			} else if(messageReceived == 0) {
				messageReceivedBool = true;
				resourceRequest = message.resource;
				processChoice = message.choice;
				for(i = 0; i < 18; i++) {
					if(processTable[i].pid == currentPid) {
						currentProcess = processTable[i];
						break;
					} 	
				}
			} else
				messageReceivedBool = false;


			//If process is requesting a resource
			if(messageReceivedBool == true && processChoice == 1) {
				currentPid = message.pid;
				
				//Increasing the number of requests for that resource
				resourceRequests[resourceRequest] += 1;
				if(availableResources[resourceRequest] > 0) {
					//reduce available resource and resource requests since it was granted
					availableResources[resourceRequest] -= 1;
					resourceRequests[resourceRequest] -= 1;


					grantedInstantly++;
					grantedRequests++;

					if(grantedRequests % 20 == 0 && verboseOn) {
						fprintf(logFile, "\n      R0    R1    R2    R3    R4    R5     R6    R7    R8    R9");
						for(i = 0; i < totalWorkers; i++) {
							fprintf(logFile, "\nP%d:", i);
							for(j = 0; j < 10; j++) {     
								fprintf(logFile, "%d    ", processTable[i].currentResources[j]); 
							}
						}
					
						printf("\n      R0    R1    R2    R3    R4    R5     R6    R7    R8    R9");
						for(i = 0; i < totalWorkers; i++) {
							printf("\nP%d:", i);
							for(j = 0; j < 10; j++) {     
								printf("%d    ", processTable[i].currentResources[j]);
							}
						}

					}
					
					//Sending message back to child the good news that their request was granted
					if(msgsnd(msqid, &message, sizeof(my_msgbuf) - sizeof(long), 0) == -1) {
						perror("\n\nmsgsend to child failed\n\n");
						exit(1);
					}
					//Increasing number of resources by 1 for process
					currentProcess.currentResources[resourceRequest] += 1;

				} else {
					//process gets blocked and requested resource gets set for future granting
					blocked++;
					currentProcess.requestedResource = resourceRequest;
				}


			}  //If process is releasing a resource
		       	else if(messageReceivedBool == true && processChoice == 2) {
				int resource = resourceRequest;
				//Increasing the number of available instances of this resource, decreasing amount the process has 
				availableResources[resourceRequest] += 1;
				currentProcess.currentResources[resourceRequest] -= 1;

				//If any processes are blocked and waiting on a resource
				if(blocked > 0) {
					//Decrease number of requests for this resource instance since it's about to be granted
					resourceRequests[resource] -= 1;

					//For each process, if they need the resource just released, give it to them
					for(i = 0; i < 18; i++) {
						if(processTable[i].requestedResource == resource) {
							currentProcess = processTable[i];
							break;
						}
					}

					//Tell lucky process their wish is granted
					message.mtype = currentProcess.pid;
					message.intData = currentProcess.pid;
					if(msgsnd(msqid, &message, sizeof(my_msgbuf) - sizeof(long), 0) == -1) {
						perror("\n\nmsgsend to child failed");
						exit(1);
					}

					//decrease the process's requests, decrease available resources, and decrease number of blocked processes
					currentProcess.requestedResource = -1;
					availableResources[resource] -= 1;
					blocked--;
				}
				
			//If a message was received and the process is terminating 
			} else if(messageReceivedBool == true && processChoice == 3) {
				//Reset PCB table entries for this process
				currentPid = message.pid;
				currentProcess.occupied = 0;
				currentProcess.pid = 0;
				
				//If there are any blocked processes waiting on a resource
				if(blocked > 0) {
					//For each of the 10 resource types
					for(i = 0; i < 10; i++) {
						//If the terminating process has any of that resource
						if(currentProcess.currentResources[i] > 0) {
							int count = currentProcess.currentResources[i];
							
							//For each instance of that resource the process has
							for(i = 0; i < count; i++) {
								availableResources[i] += 1;
								
								//For each process that might need that resource
								for(j = 0; j < 18; j++) {
									//If the currently tested process needs that resource
									if(processTable[j].requestedResource == resourceRequest) {

										//Send process the message that they're finally getting the resource
										message.mtype = processTable[j].pid;
										//message.intData = processTable[j].pid;
										if(msgsnd(msqid, &message, sizeof(my_msgbuf) - sizeof(long), 0) == -1) {
											perror("\n\nmsgsend to child failed");
											exit(1);
										}
										
										//Decrease the available instances of that resource, the requests for it, and the process's request 
										processTable[j].requestedResource = -1;
										availableResources[i] -= 1;
										resourceRequests[i] -= 1;
										//Remove one blocked processs since it got its required resource
										blocked--;	
										break;
									}
								}
							}
						}
					}
				}

				//Decreasing simul workers
				simulWorkers--;
			}

			if(doneCreating && simulWorkers == 0) 
				doneRunning = true;
		
		
			//every second do deadlock detection
			if(*seconds >= deadlockTime) {
				//allocatedResources = forloop stuff
				for(i = 0; i < 18; i++) {
					for(j = 0; j < 10; j++) {
						allocatedResources[i][j] = processTable[i].currentResources[j];
					}
				}
				deadlockTime++;
				bool deadlockFound = true;

				while(deadlockFound) {
					deadlockFound = deadlock(maxResources, maxResources, allocatedResources, simulWorkers, 10);
					if(deadlockFound) {
						//Terminate processes involved 
					}
				}
			}


			//incrementClock(5000);
			if((*nanoSeconds + nanoIncrement) < billion)
				*nanoSeconds += nanoIncrement;
			else
			{
				*nanoSeconds = ((*nanoSeconds + nanoIncrement) - billion);
				*seconds += 1;
			}

		}

		


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
	printf("When the program starts, it will begin launching user processes that will each choose a random number and either terminate, request a resource, or release a resource");
	printf("\nThe program will terminate after either 40 processes have been launched or 5 real life seconds have passed");
	printf("\n\nInput Options:");
	printf("\n-h     output a short description of the project and how to run it");
	printf("\n-v     turns verbose mode on, so the logfile will show all oss actions. Verbose off only shows the output of deadlock detection");
	printf("\n-f     the name of the file for output to be logged in");
	printf("\n\nInput example:");
	printf("\n./oss -f logfile.txt");
	printf("\nThis would launch the program and send all oss output to logfile.txt\n");

	exit(1);
}




//Handler function for signal to stop program after 60 seconds or with Ctrl + C
static void myhandler(int s) {
	int i, pid;
	for(i = 0; i <= 19; i++) {
		pid = processTable[i].pid;
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




bool deadlock(int available[], int max[18][10],  int allocation[18][10], int numberOfProcesses, int numberOfResources) {
	int i, j;
	int work[numberOfResources];
	int finish[numberOfResources];
	int need[numberOfProcesses][numberOfResources];
	int numFinished = 0;

	//Initializing work, finish, and need arrays
	for(i = 0; i < numberOfResources; i++) {
		work[i] = available[i];
	}

	for(i = 0; i < numberOfProcesses; i++) {
		finish[i] = 0;
	}

	for(i = 0; i < numberOfProcesses; i++) {
		for(j = 0; j < numberOfResources; j++) {
			need[i][j] = max[i][j] - allocation[i][j];
		}
	}

	//Need matrix calculation
	while(numFinished < numberOfProcesses) {
		int found = 0;
		for(i = 0; i < numberOfProcesses; i++) {
			if(!finish[i]) {
				int canFinish = 1;
				for(j = 0; j < numberOfResources; j++) {
					if(need[i][j] > work[j]) {
						canFinish = 0;
						break;
					}
				}
				if(canFinish) {
					for(j = 0; j < numberOfResources; j++) {
						work[j] += allocation[i][j];
					}
					finish[i] = 1;
					numFinished++;
					found = 1;
				}
			}
		}
		
		if(!found) {
			//deadlock detected
			return 1;
		}
	}

	//No deadlock detected
	return 0;
}
