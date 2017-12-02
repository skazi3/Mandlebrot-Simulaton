/*
Sarah Kazi
skazi3
HW 4
Mandelbrot-skazi3.cpp
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

#define READ 0
#define WRITE 1

pid_t pid1, pid2, wpid1, wpid2;
int msgqid1, msgqid2, shmid;

struct msg_buf {
    long int mtype;       /* message type */
    char mtext[200];      /* message data */
}msg, child1, child2;

void sig_handler(int signal){
    if(signal == SIGCHLD){
        //children exit due to error conditions
        exit(-1);
    }
    if(signal == SIGINT){
        kill(pid1, SIGUSR1);
        kill(pid2, SIGUSR2);
        if(shmctl(shmid, IPC_RMID, 0) == -1){
            perror("shmctl fail");
            exit(-1);
        }
        if (msgctl(msgqid1, IPC_RMID, NULL) == -1) {
            perror("msgctl1: msgctl failed");
            exit(-1);
        }
        if (msgctl(msgqid2, IPC_RMID, NULL) == -1) {
            perror("msgctl2: msgctl failed");
            exit(-1);
        }
        exit(-1);
    }

}


/*begin main*/
int main (int argc, char **argv) {
    /*look out for SIGCHLD*/
    signal(SIGCHLD, sig_handler);
    signal(SIGINT, sig_handler);

    //initialize variables

	int stat1, stat2, opt1, opt2, rc;
    int nRows, nCols, maxIters;
    double xMin, xMax, yMin, yMax;
    char *filename = (char *)malloc(sizeof(char)*200);
    int pipe1[2], pipe2[2];
    FILE* fp;

    printf("HW 4\nSarah Kazi\nskazi3\nCS 361\n10/31/2017\n");
/*----------------------------------CREATE PIPES-----------------------------------------*/
    if (pipe(pipe1) == -1) {
        perror("pipe1");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipe2) == -1) {
        perror("pipe2");
        exit(EXIT_FAILURE);
    }

/*--------------------------set up message queues --------------------------------*/    
    if((msgqid1 = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1){
        perror("msgget1 failed");
        exit(1);
    }

    if((msgqid2 = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1){
        perror("msgget2 failed");
        exit(1);
    }

/*----------------------set up shared memory------------------------------------------*/
    if((shmid = shmget(IPC_PRIVATE, 6000*sizeof(int), IPC_CREAT | 0666)) == -1){
        perror("shmget failed");
        exit(1);
    }



/*---------------------parse to send as arguments--------------------------------*/

    char mqIDbuf1[20];
    sprintf(mqIDbuf1, "%d", msgqid1);

    char mqIDbuf2[20];
    sprintf(mqIDbuf2, "%d", msgqid2);

    char shmIDbuf[20];
    sprintf(shmIDbuf, "%d", shmid);



    pid1 = fork();
    if(pid1 < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
/*------------------------first child---------------------------------------------------*/
	else if(pid1  == 0){
        
        close(pipe2[READ]);
        close(pipe1[WRITE]);
        if(dup2(pipe1[READ], STDIN_FILENO) == -1 ) /* stdin to the read end of pipe 1*/{
            perror( "dup2 read failed" );
            exit(1);
        }
        if(dup2(pipe2[WRITE], STDOUT_FILENO) == -1 ) /*stdout to write end of pipe2 */{
            perror( "dup2 write failed" );
            exit(1);
        }  
        close(pipe1[READ]);   
        close(pipe2[WRITE]);
        
		execlp("./MandelCalc", "./MandelCalc", shmIDbuf, mqIDbuf1, NULL);
        perror("Bad");
		return -1;

	}

    pid2 = fork();
/*------------------------------------second child-----------------------------------------*/
	if(pid2 == 0){
        close(pipe1[READ]);
        close(pipe1[WRITE]);
        close(pipe2[WRITE]);

        dup2(pipe2[READ], STDIN_FILENO); /*redirect stdout to pipe 2 read end*/
        close(pipe2[READ]);

		execlp("./MandelDisplay", "./MandelDisplay", shmIDbuf, mqIDbuf1, mqIDbuf2, NULL);
        perror("Bad");
		return -2;
	}
  
    else if(pid2 < 0){
        perror("MandelDisplay fail");
        exit(EXIT_FAILURE);
    }

/*------------------------------PARENT-------------------------------------------------------*/
    else{
        printf("I am the parent: I created processes %d and %d!!\n", pid1, pid2);
    }
    /*close unused pipes*/
    close(pipe1[READ]);
    close(pipe2[READ]);
    close(pipe2[WRITE]);

    fp = fdopen(pipe1[WRITE], "w");
    if(fp == NULL)
    {
        perror("fdopen");
        exit(-1);
    }

    while(true){

        //read user problem
        printf("Enter the # of rows to display fam: (0 to quit):");
        scanf("%d", &nRows);
        if(nRows != 0){ 
            printf("Enter the # of cols to display now: (0 to quit):");
            scanf("%d", &nCols);
            if(nCols == 0) {exit(1);}
            
            printf("Enter min x: ");
            scanf("%lf", &xMin);
            printf("Enter max x: ");
            scanf("%lf", &xMax);
            printf("Enter min y: ");
            scanf("%lf", &yMin);
            printf("Enter max x: ");
            scanf("%lf", &yMax);
            printf("Enter max number of iterations: ");
            scanf("%d", &maxIters);

            printf("Enter name of output file: ");
            scanf("%s", filename);
            
            //this is the filename to write to mq
            msg.mtype = 1;
            sprintf(msg.mtext,"%s", filename);
            int length = sizeof(struct msg_buf) - sizeof(long int);
            if((rc = msgsnd(msgqid2, (void *)&msg, length, 0)) == -1){
                perror("msgsnd parent fail");
                exit(1);
            }
            //send info to child about xMin...nRows
            fprintf(fp, "%d\n", nRows);
            fprintf(fp, "%d\n", nCols);
            fprintf(fp, "%lf\n", xMin);
            fprintf(fp, "%lf\n", xMax);
            fprintf(fp, "%lf\n", yMin);
            fprintf(fp, "%lf\n", yMax);
            fprintf(fp, "%d\n", maxIters);
            fflush(fp);

            //check now if received message from children "done"
            ssize_t result1;
            child1.mtype = 1;
            if((result1 = msgrcv(msgqid1, (void*)&child1, length, 0, 0)) == -1){
                perror("msgrcv from child1 failed");
                exit(-1);
            }
            else
                printf("in parent: msg child1 received: %s\n", child1.mtext);

            ssize_t result2;
            child2.mtype = 1;
            if((result2 = msgrcv(msgqid1, (void*)&child2, length, 0, 0)) == -1){
                perror("msgrcv from child 2 failed");
                exit(-1);
            }
            else
                printf("in parent: msg child2 received: %s\n", child2.mtext);
        }
        else{
            //user is done entering: kill children
            kill(pid1, SIGUSR1);
            kill(pid2, SIGUSR1);
            wpid1 = waitpid(pid1, &stat1,0); // Wait for both children
            wpid2 = waitpid(pid2, &stat2,0); 

            //report children exit status
            if(WIFEXITED(stat1))
                printf("Child %d exited. Exit code = %d\n", pid1, WEXITSTATUS(stat1));
            
             
            if(WIFEXITED(stat2)) 
                printf("Child %d exited. Exit code = %d\n", pid2, WEXITSTATUS(stat2));

            //close file and pipe end
            fclose(fp);
            close(pipe1[WRITE]);

/*------------------------------------------free resources------------------------------------------------*/
            if(shmctl(shmid, IPC_RMID, 0) == -1){
                perror("shmctl failed");
                exit(1);
            }
             if (msgctl(msgqid1, IPC_RMID, NULL) == -1) {
                perror("msgctl1 failed");
                exit(1);
            }
            if (msgctl(msgqid2, IPC_RMID, NULL) == -1) {
                perror("msgctl2 failed");
                exit(1);
            }

            break;
        }
        
    }
 return 0;
}
       


   