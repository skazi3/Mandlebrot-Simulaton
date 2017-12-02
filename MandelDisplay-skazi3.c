/*
Sarah Kazi
skazi3
CS 361
LAB 7
MandelDisplay-skazi3.cpp
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <sys/msg.h>

struct msg_buf {
    long int mtype;       /* message type, must be > 0 */
    char mtext[200];    /* message data */
}rcv, snd;

int *data;
int imageProcessed = 0;
void sig_handler(int signal){
	//if SIGUSR1, exit with non-neg code = to # of images calculated
	if(signal == SIGUSR1){
		if(shmdt(data) == -1){
	        perror("shmdt");
	        exit(1);
	    }
	    exit(imageProcessed);
	}
}
int main (int argc, char **argv) {
	if(argc >= 1){
		signal(SIGUSR1, sig_handler);
		int nColors = 15;
		char filename[100];
		FILE *fp;
		int    msgqid1, msgqid2, shmid, rc, nRows, nCols, maxIters;
		shmid   = atoi(argv[1]);
		msgqid1 = atoi(argv[2]);
		msgqid2 = atoi(argv[3]);
		double xMin, xMax, yMin, yMax;
		char colors[] = ".-~:+*%08&?$@#X";

		data = (int*)shmat(shmid, NULL, 0);
	    if(data == (int*)-1){
	        perror("shmat fail in child2");
	        exit(-1);
	    }



		while(true){
			scanf("%d", &nRows);
	  	    scanf("%d", &nCols);
	  	    scanf("%lf", &xMin);
	  	    scanf("%lf", &xMax);
	  	    scanf("%lf", &yMin);
	  	    scanf("%lf", &yMax);
	  	    scanf("%d", &maxIters);


			/*debugging purposes*/
			
	/*-----------------------------read filename from message queue 2------------------------------------*/
			ssize_t result;
			rcv.mtype = 1;
			int length = sizeof(struct msg_buf) - sizeof(long int);
			if((result = msgrcv(msgqid2, (void*)&rcv, length, 0,0)) == -1){
				perror("msgrcv child2 failed");
				exit(-1);
			}
    /*------------open file, if file opened properly save data to file--------------------------*/
			fp = fopen(rcv.mtext, "w");
	/*------------read data from shared mem and display image on screen--------------------------*/
			int r, c, n;
			for(r = nRows -1; r > 0; r--){
				for(c = 0; c < nCols-1; c++){
					n = *(data + r * nCols+ c);
					
					fprintf(fp, " %d", n);
					if(n < 0){
						printf(" ");
					}
					else{
						printf("%c", colors[n %nColors]);
					}
				}
				printf("\n");
				fprintf(fp, "\n");
			}


			fclose(fp);
	/*-----------------------------send done to message queue 1------------------------------------*/		
			snd.mtype = 1;
			sprintf(snd.mtext, "done\n");
			if((rc = msgsnd(msgqid1, (void *)&snd, length, 0)) == -1){
	            perror("msgsnd child1 fail");
	            exit(1);
	        }	
	        imageProcessed++;
		}
	}


	return 1;
}
