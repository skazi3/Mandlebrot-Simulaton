/*
Sarah Kazi
skazi3
CS 361
LAB 7
MandelCalc-skazi3.cpp
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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

int* data;
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
struct msg_buf {
    long int mtype;     /* message type, must be > 0 */
    char mtext[200];    /* message data */
}msg;

int main (int argc, char **argv) {
	if(argc >= 1){
		signal(SIGUSR1, sig_handler);
		int shmid, msgqid, rc;
		shmid = atoi(argv[1]);
		msgqid = atoi(argv[2]);

/*------------------------shmat returns mem address given shmid.---------------------------------------*/

		data = (int*)shmat(shmid, NULL, 0);
	    if(data == (int*)-1){
	        perror("shmat fail");
	        exit(-1);
	    }

		int nRows, nCols, maxIters;
		double xMin, xMax, yMin, yMax;
		while(true){

/*-------------------read nCols, nRows, xMin, xMax, yMin, yMax, maxIters from stdin---------------------*/
			scanf("%d", &nRows);
	  	    scanf("%d", &nCols);
	  	    scanf("%lf", &xMin);
	  	    scanf("%lf", &xMax);
	  	    scanf("%lf", &yMin);
	  	    scanf("%lf", &yMax);
	  	    scanf("%d", &maxIters);
	  	
/*---------------------------implement mandelbrot algorithm----------------------------------------------*/
	  	    double deltaX, deltaY;
	  	    double Zx, Zy, Zx_next, Zy_next, Cy, Cx;
	  	    int n;
	  	    deltaX = (xMax - xMin)/ (nCols -1);
	  	    deltaY = (yMax - yMin)/ (nRows -1);
	  	    int r, c;
	  	    for(r = 0; r < nRows-1; r++){
	  	    	Cy = yMin + r * deltaY;
	  	    	for( c = 0; c < nCols-1; c++){
	  	    		Cx = xMin+ c * deltaX;
	  	    		Zx = Zy = 0.0;
	  	    		for(n = 0; n < maxIters; n++){
	  	    			if(Zx * Zx + Zy * Zy >= 4.0)
	  	    				break;
	  	    			Zx_next = Zx * Zx - Zy * Zy + Cx;
	  	    			Zy_next = 2.0 * Zx * Zy + Cy;
	  	    			Zx = Zx_next;
	  	    			Zy = Zy_next;
	  	    		}
	  	    		if(n >= maxIters)
	  	    			*(data + r * nCols + c) = -1;
	  	    		else
	  	    			*(data + r * nCols + c) = n;
	  	    	}
	  	    }

			//write xMin...nCols to stdout
			printf("%d\n", nRows);
			printf("%d\n", nCols);
			printf("%lf\n", xMin);
			printf("%lf\n", xMax);
			printf("%lf\n", yMin);
			printf("%lf\n", yMax);
			printf("%d\n", maxIters);

			fflush(stdout);
			//write done to parent
			msg.mtype = 1;
			sprintf(msg.mtext, "done");
			int length = sizeof(struct msg_buf) - sizeof(long int);
			if((rc = msgsnd(msgqid, (void *)&msg, length, 0)) == -1){
                perror("msgsnd child1 fail");
                exit(1);
            }
     
            fflush(stderr);
            imageProcessed++;
		
		}
		
	}


	return 1;
}
