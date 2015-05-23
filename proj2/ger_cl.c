#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <pthread.h> 
#include <semaphore.h> 
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>

#define N_BALCAO 0
#define N_TEMPO 1
#define N_DURACAO 2
#define N_FIFO 3
#define N_EM_ATENDIMENTO 4
#define N_JA_ATENDIDOS 5
#define TEMPO_MEDIO_ATENDIMENTO 6

#define MAX_NUMBER_LINE 500

typedef struct
{
	pthread_mutex_t buffer_lock;
	pthread_cond_t slots_cond;
	pthread_cond_t items_cond;
	pthread_mutex_t slots_lock;
	pthread_mutex_t items_lock;
	
	pthread_mutex_t mutexLock;

	FILE * logFile;
	char nameOfLog[MAX_NUMBER_LINE];

	int numeroDeBalcoes;
	int numeroDeBalcoesExecucao;
	time_t openingTime;

	double table [7][1000];
	
} SharedMem;

//----------------------------------------LER DO FIFO---------------------------------------------
int readline(int fd, char *str) 
{ 
	int n; 
	do { 
		n = read(fd,str,1); 
	} while (n>0 && *str++ != '\0'); 
	return (n>0); 
} 
//----------------------------------------------------------------------------------------------------
void printTime(FILE * logFile)
{
	char buffer[26];
	time_t timer;
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(logFile, " %s", buffer);
    fprintf(logFile, "\t| ");
}

void printOnLog(SharedMem * shm,char * who, int num, char * message)
{

	shm->logFile = fopen(shm->nameOfLog, "a");
	printTime(shm->logFile);
	fprintf(shm->logFile, "%s\t| %d\t\t| %s\t| fc_%d\n", who,num, message, getpid());
	fclose(shm->logFile);
}
//----------------------------------------CRIA MEMORIA PARTILHADA-------------------------------------------
SharedMem * getSharedMemory(char* shm_name,int shm_size)
{
	int shmfd;
	SharedMem *shm;					//create the shared memory region

	shmfd = shm_open(shm_name, O_RDWR, 0660);
	if (shmfd <= 0)
	{
		perror("shm_open()");
		exit(EXIT_FAILURE);
	}

	if (ftruncate(shmfd,shm_size) < 0)
	{
		perror("ftruncate()");
		exit(EXIT_FAILURE);
	}								//attach this region to virtual memory
	shm = mmap(0,shm_size,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
	if (shm == MAP_FAILED)
	{
		perror( "mmap()");
		exit(EXIT_FAILURE);
	}								//initialize data in shared memory
	
	return (SharedMem *) shm;
}
//--------------------------------------------------------------------------------------------------

int getFdBalcao(SharedMem * shm, int * numberOfDesk)
{
	int result = -1;
	int minClients = -1;
	int i = 0;

	while (pthread_mutex_trylock(&shm->mutexLock)) {}
	while (i < shm->numeroDeBalcoesExecucao)
	{
		if (result == -1)
		{
			result = i;
			minClients = shm->table[N_EM_ATENDIMENTO][i];
		}
		else if (shm->table[N_EM_ATENDIMENTO][i] < minClients)
		{
			minClients = shm->table[N_EM_ATENDIMENTO][i];
			result = i;
		}
		i++;
	}
	*numberOfDesk = result+1;
	int ret = shm->table[N_FIFO][result];
	shm->table[N_EM_ATENDIMENTO][result]++;
	pthread_mutex_unlock(&shm->mutexLock);
	return ret;
}

int main(int argc, char *argv[])
{
	setbuf(stdout, NULL);
	if (argc != 3)
	{
		printf("Wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	int nClients = atoi(argv[2]);
	SharedMem * shm;
	shm = getSharedMemory(argv[1], sizeof(SharedMem));

	int ii = 0;
	printf("\nnClientes: %d", nClients);
	while (ii < nClients)
	{
		pid_t pidF = fork();
		if (pidF < 0)
		{
			perror("fork()");
			exit(EXIT_FAILURE);
		}
		else if (pidF == 0)
		{
			char fifoName[MAX_NUMBER_LINE];
			strcpy(fifoName, "fc_");
			char pid[MAX_NUMBER_LINE];
			sprintf(pid, "%d", getpid());
			strcat(fifoName, pid);

			
			//-------------ABRE O FIFO DO BALCAO-------------------------------
			char fifoB[MAX_NUMBER_LINE] = "/tmp/fb_";
			char pidB[MAX_NUMBER_LINE];
			
			int numberOfDesk = 0;
			int fifo_balcao = getFdBalcao(shm, &numberOfDesk);
			
			sprintf(pidB, "%d", fifo_balcao);
			strcat(fifoB, pidB);

			int fd_b = -1;
			do
			{
				fd_b = open(fifoB, O_WRONLY);
				if (fd_b == -1) sleep(1);
			} while (fd_b == -1);
			//------------------------------------------------------------------
			int messagelen=strlen(fifoName)+1; 
			printOnLog(shm,"Cliente", numberOfDesk, "pede_atendimento           ");
			if (write(fd_b,fifoName,messagelen) < 0)
				perror("write()");								//escreve informacao no fifo do balcao acerca do seu fifo

			if (close(fd_b) < 0)
				perror("close()");
			

			//-------------CRIA E ABRE FIFO DO CLIENTE--------------------------------
			char pathToFifo[MAX_NUMBER_LINE];
			strcpy(pathToFifo, "/tmp/");
			strcat(pathToFifo, fifoName);
			if (mkfifo(pathToFifo, 0660) == -1)
				perror("mkfifo()");

			printOnLog(shm,"Balcao", numberOfDesk, "inicia_atendimento_cliente");
			int fd_cl;
			do
			{
				fd_cl = open(pathToFifo, O_RDONLY);
				if (fd_cl == -1) sleep(1);
			} while (fd_cl == -1);
			//----------------------------------------------------------------

			char endMessage[MAX_NUMBER_LINE];

			while(readline(fd_cl, endMessage)) {
				printf("\nRead final message as follows: %s (Client %d)", endMessage, ii);
				if(strcmp(endMessage,"fim_atendimento") == 0){
					printOnLog(shm, "Cliente", numberOfDesk, endMessage);
					if (close(fd_cl) < 0)
						perror("close()");
					exit(EXIT_SUCCESS);
				}else{
					if (close(fd_cl) < 0)
						perror("close()");
					exit(EXIT_FAILURE);
				}
			} //Le info de retorno

			if (close(fd_cl) < 0)
				perror("close()");
			exit(EXIT_SUCCESS);
		}
		else if (pidF > 0)
			ii++;	
	}
	int i = 0;

	while (i < nClients)
	{
		printf("\nWaiting %d", i);
		int status;
		wait(&status);
		i++;
	}
	putchar('\n');

	exit(EXIT_SUCCESS);
}