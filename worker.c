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
	int intData;
}my_msgbuf;



int main(int argc, char** iterations) {
	struct my_msgbuf message;
	int msqid;
	key_t key;
	message.mtype = getpid();

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


	//Random number generator
	srand(getpid());
	bool terminated = false, firstTime = true;
	int randTimeMax = 250000000, billion = 1000000000;
	int task;

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

	//Setting message queue variables to send back to parent
	message.intData = getppid();
	message.mtype = getppid();

	//Do nothing before at least 1 second has passed
	while(*sharedSeconds < starterSec || (*sharedSeconds == starterSec && *sharedNanoSeconds < starterNano)) 




	while(!terminated) {

		//Random number to choose to terminate, request, or release
		if(*sharedSeconds > chooseTimeSec || (*sharedSeconds == chooseTimeSec && *sharedNanoSeconds >= chooseTimeNano)) {
			task = (rand() % (100 - 0 + 1)) + 0;

			if(task == 0) {
				terminated = true;
				//send message to parent that they're terminating and releasing all resources
			} else if(task >= 1 && task <= 95) {
				message.resource = (rand() % (10 - 1 + 1)) + 1;
				//Pick a random resource, send to parent the request
			}



			//Wait receive to see if resource was granted
		}
		
	}


	return 0;

}
