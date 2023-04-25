//Katelyn Bowers
//OSS- Project 5
//April 25, 2023
//worker.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <errno.h>
#include "resources.h"


#define PERMS 0644
struct my_msgbuf {
	long mtype;
	int resource;
	int choice;     //1 = request, 2 = release, 3 = terminate
	int pid;
	int intData;
}my_msgbuf;



int main(int argc, char** iterations) {
	struct my_msgbuf message;
	struct my_msgbuf received;
	int msqid;
	key_t key;
	message.mtype = getppid();
	message.intData = getppid();
	received.mtype = 1;
	message.pid = getpid();

	int currentResources[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	//Making sure message queue works	
	if((key = ftok("msgq.txt", 'B')) == -1) {
		perror("ftok");
		exit(1);
	}

	if((msqid = msgget(key, PERMS)) == -1) {
		perror("msgget failed in child");
		exit(1);
	}

	//Receiving message
	if(msgrcv(msqid, &message, sizeof(my_msgbuf), getpid(), 0) == -1) {
		perror("msgrcv failed in child");
		exit(1);
	}


	//Attaching to shared memory from here til line 81
	const int sec_key = 25217904;
	const int nano_key = 25218510;

	int sec_id = shmget(sec_key, sizeof(int) * 10, IPC_CREAT | 0666);
	if(sec_id <= 0) {
		fprintf(stderr, "Shared memory get for seconds in worker failed\n");
		exit(1);
	}

	int *sec_ptr = (int *) shmat(sec_id, 0, 0);
	if(sec_ptr <= 0) {
		fprintf(stderr, "Shared memory attachment for seconds in worker failed\n");
		exit(1);
	}

	int nano_id = shmget(nano_key, sizeof(int) * 10, IPC_CREAT | 0666);
	if(nano_id <= 0) {
		fprintf(stderr, "Shared memory get for nanoseconds in worker failed\n");
		exit(1);
	}


	int *nano_ptr = (int *) shmat(nano_id, 0, 0);
	if(nano_ptr <= 0) {
		fprintf(stderr, "Shared memory attachment for nanoseconds in worker failed\n");
		exit(1);
	}

	int * sharedSeconds = (int *)(sec_ptr);
	int * sharedNanoSeconds = (int *)(nano_ptr);

	int starterNano = *sharedNanoSeconds;
	int starterSec = *sharedSeconds + 1;


	printf("\n\n\nWorker started\n\n\n");

	//Random number generator
	srand(getpid());
	bool terminated = false, chosen = false, enough = true;
	int randTimeMax = 250000000, billion = 1000000000;
	int task, randomResource;

	//First random time to termiante, request resources, or release
	int randomTime = (rand() % (randTimeMax - 0 + 1)) + 0;
	int chooseTimeNano = *sharedNanoSeconds, chooseTimeSec = *sharedSeconds;
	if((*sharedNanoSeconds + randomTime) < billion)
		chooseTimeNano += randomTime;
	else
	{
		chooseTimeNano = ((*sharedNanoSeconds + randomTime) - billion);
		chooseTimeSec += 1;
	}

	
	//Do nothing before at least 1 second has passed
	while(*sharedSeconds < starterSec || (*sharedSeconds == starterSec && *sharedNanoSeconds < starterNano)) 




	while(!terminated) {
		//Setting message queue variables to send back to parent
		chosen = false;


		//Random number to choose to terminate, request, or release
		if(*sharedSeconds > chooseTimeSec || (*sharedSeconds == chooseTimeSec && *sharedNanoSeconds >= chooseTimeNano)) {
			task = (rand() % (100 - 0 + 1)) + 0;
			chosen = false;

			if(task == 0) {
				terminated = true;
				message.choice = 3;

				//send message to parent that they're terminating and releasing all resources
				if(msgsnd(msqid, &message, sizeof(my_msgbuf) - sizeof(long), IPC_NOWAIT) == -1) {
					perror("msgsend to parent failed");
					exit(1);
				}
			//Process is choosing to request a resource
			} else if(task >= 1 && task <= 95) {
				do {

					message.resource = (rand() % (10 - 0 + 1)) + 0;
					if(currentResources[message.resource] >= 20) 
						enough = false;
					else
						enough = true;
				}while(!enough);
				message.choice = 1;
				printf("Selected resource: %d", message.resource);
				//Pick a random resource, send to parent the request
				if(msgsnd(msqid, &message, sizeof(my_msgbuf) - sizeof(long), IPC_NOWAIT) == -1) {
					perror("msgsend to parent failed");
					exit(1);
				}

				if(msgrcv(msqid, &message, sizeof(my_msgbuf), getpid(), 0) == -1) {
					perror("msgrcv from parent failed");
					exit(1);
				}

				int receivedResource = message.resource;
				currentResources[receivedResource] += 1;


			//Process is releasing a resource
			} else {
				message.choice = 2;
				while(!chosen) {
					randomResource = (rand() % (10 - 0 + 1)) + 0;
					if(currentResources[randomResource] > 0) {
						message.resource = randomResource;
						currentResources[randomResource] -= 1;
						chosen = true;
					}
				}
				
				if(msgsnd(msqid, &message, sizeof(message) - sizeof(long), IPC_NOWAIT) == -1) {
					perror("msgsend to parent failed");
					exit(1);
				}

			}

		}
		
	}

	return 0;

}
