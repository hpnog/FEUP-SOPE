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

int getFdBalcao(SharedMem * shm)
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
	int ret = shm->table[N_FIFO][result];
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
			
			int fifo_balcao = getFdBalcao(shm);
			
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
		wait();
		i++;
	}
	putchar('\n');

	exit(EXIT_SUCCESS);
}