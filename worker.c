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


#define PERMS 0644
struct my_msgbuf {
	long mtype;
	int timeQuantum;
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

	//Assigning variable to quanutm from oss
	int quantum = message.timeQuantum;


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
	int * sharedNanoSecs = (int *)(nano_ptr);


	//Random number generator
	srand(getpid());
	bool terminated = false, firstTime = true;


	//Setting message queue variables to send back to parent
	message.intData = getppid();

