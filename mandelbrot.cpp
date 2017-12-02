// Zakee Jabbar (zjabba2)
// CS 361
// HW 4


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#define READ 0
#define WRITE 1

using namespace std;

typedef struct msgBuf 
{
    long int mtype;
    char message[200];
}msgBuf;


pid_t child1;
pid_t child2;

int main(int argc, char **argv)
{
    int pipe1[2];
    int pipe2[2];
    FILE * pipeFile;
    int msgQID1;
    int msgQID2;
    int shmID;
    int rs;

    //_____________________________________________________

    // My information

    printf("Name: Zakee Jabbar\n");
    printf("NetID: zjabba2\n");
    printf("CS 362\n");

    // ____________________________________________________

    // Pipe Stuff

    if(pipe(pipe1) == -1)
    {
        perror("Pipe 1");
        exit(-5);
    }

    if(pipe(pipe2) == -1)
    {
        perror("Pipe 2");
        exit(-6);
    }

    // ____________________________________________________

    // Message Queues Stuff

    if((msgQID1 = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1)
    {
        perror("msgget failed");
        exit(-11);
    }
    if((msgQID2 = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1)
    {
        perror("msgget failed");
        exit(-11);
    }

    char msgQID1_char[16];
    sprintf(msgQID1_char, "%d", msgQID1);

    char msgQID2_char[16];
    sprintf(msgQID2_char, "%d", msgQID2);

    cout << "Queue ID 1: " << msgQID1 << endl;
    cout << "Queue ID 2: " << msgQID2 << endl;

    //______________________________________________________

    // Shared memory stuff

    if((shmID = shmget(IPC_PRIVATE, 4000*sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        perror("shmget failed");
        exit(-14);
    }

    char shmID_char[16];
    sprintf(shmID_char, "%d", shmID);

    cout << "Shared Memory ID: " << shmID << endl;



    // ______________________________________________________







    child1 = fork();

    if(child1 < 0)
    {
    	perror("Fork error");
    	exit(-1);
    }
    else if(child1 == 0)
    {
        close(pipe2[READ]);
        close(pipe1[WRITE]);

        if(dup2(pipe1[READ], STDIN_FILENO) == -1)
        {
            perror( "dup2 failed" );
            exit(-7);
        }

        close(pipe1[READ]);


        if(dup2(pipe2[WRITE], STDOUT_FILENO) == -1)
        {
            perror( "dup2 failed" );
            exit(-8);
        }

        close(pipe2[WRITE]);

        
        char* str[5] = {"./MandelCalc", msgQID1_char, shmID_char, NULL};
    	execvp(str[0], str);
        perror("MandelCalc failed");
        exit(-3);
    }
    else
    {
    	cout << "Parent created process " << child1 << endl;
    }

    if(child1 != 0)
    {
    	child2 = fork();
    }

    if(child2 < 0)
    {
    	perror("Fork error");
    	exit(-2);
    }
    else if(child2 == 0)
    {
        close(pipe1[READ]);
        close(pipe1[WRITE]);
        close(pipe2[WRITE]);

        if(dup2(pipe2[READ], STDIN_FILENO) == -1)
        {
            perror( "dup2 failed" );
            exit(-9);
        }
        close(pipe2[READ]);

        char* str[6] = {"./MandelDisplay", msgQID1_char, msgQID2_char, shmID_char, NULL};
    	execvp(str[0], str);
        perror("MandelDisplay failed");
        exit(-4);
    }
    else
    {
    	cout << "Parent created process " << child2 << endl;
    }

    close(pipe1[READ]);
    close(pipe2[READ]);
    close(pipe2[WRITE]);

    pipeFile = fdopen(pipe1[WRITE], "w");
    if(pipeFile == NULL)
    {
        perror("fdopen failed");
        exit(-10);
    }

    close(pipe1[WRITE]);

    msgBuf filename;
    filename.mtype = 1;
    sprintf(filename.message, "%s", "output.txt");
    int length = sizeof(msgBuf) - sizeof(long int);    
    if((rs = msgsnd(msgQID2, (void*) &filename, length, 0)) == -1)
    {
        perror("msgsnd failed");
        exit(-13);
    }
   


    int status1;
    int status2;

     pid_t wpid1 = waitpid(child1, &status1,0); // Wait for child1
     pid_t wpid2 = waitpid(child2, &status2,0); // Wait for child2

    ssize_t result;
    msgBuf child1Done;
    child1Done.mtype = 1;
    result = msgrcv(msgQID1, (void*)&child1Done, length, 0, MSG_NOERROR | IPC_NOWAIT);
    if(result == -1)
    {
        perror("msgrcv failed:");
        exit(-1);
    }
    else
    {
        cerr << "Received in parent from child1: " << child1Done.message << endl;
    }

    msgBuf child2Done;
    child2Done.mtype = 1;
    result = msgrcv(msgQID1, (void*)&child2Done, length, 0, MSG_NOERROR | IPC_NOWAIT);
    if(result == -1)
    {
        perror("msgrcv failed:");
        exit(-1);
    }
    else
    {
        cerr << "Received in parent from child2: " << child2Done.message << endl;
    }

     if(WIFEXITED(status1)) // Normal exit
     {
        cout << endl << "Child " << child1 << " exited with exit status " << WEXITSTATUS(status1) << endl;
     }

     
     if(WIFEXITED(status2)) // Normal exit
     {
        cout << endl << "Child " << child2 << " exited with exit status " << WEXITSTATUS(status2) << endl;
     }

     return 0;




}