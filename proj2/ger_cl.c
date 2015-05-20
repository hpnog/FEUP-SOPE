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

typedef struct
{
	pthread_mutex_t buffer_lock;
	pthread_cond_t slots_cond;
	pthread_cond_t items_cond;
	pthread_mutex_t slots_lock;
	pthread_mutex_t items_lock;

	int numeroDeBalcoes;
	int numeroDeBalcoesExecucao;
	time_t openingTime;
	int table [7][1000];
	
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
		perror("Failure in shm_open()");
		return NULL;
	}

	if (ftruncate(shmfd,shm_size) < 0)
	{
		perror("Failure in ftruncate()");
		return NULL;
	}								//attach this region to virtual memory
	shm = mmap(0,shm_size,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
	if (shm == MAP_FAILED)
	{
		perror( "Failure in mmap()");
		return NULL;
	}								//initialize data in shared memory
	
	return (SharedMem *) shm;
}
//--------------------------------------------------------------------------------------------------

int getFdBalcao(SharedMem * shm)
{
	int result = -1;
	int minClients = -1;
	int i = 0;
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
	shm->table[N_EM_ATENDIMENTO][result]++;
	printf("\nEscolheu o balcao: %d\n", result);
	return shm->table[N_FIFO][result];
}

int main(int argc, char *argv[])
{
	setbuf(stdout, NULL);
	if (argc != 3)
	{
		printf("Wrong number of arguments\n");
		return 1;
	}

	int nClients = atoi(argv[2]);
	SharedMem * shm;
	shm = getSharedMemory(argv[1], sizeof(SharedMem));

	int i = 0;
	while (i < nClients)
	{
		pid_t pid = fork();
		if (pid < 0)
		{
			perror("ERROR in fork().");
			return 1;
		}
		else if (pid == 0)
		{
			char fifoName[100] = "fc_";
			char pid[50];
			sprintf(pid, "%d", getpid());
			strcat(fifoName, pid);

			
			//-------------ABRE O FIFO DO BALCAO-------------------------------
			char fifoB[100] = "/tmp/fb_";
			char pidB[50];
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
			write(fd_b,fifoName,messagelen); //escreve informacao no fifo do balcao acerca do seu fifo

			close(fd_b);
			//-------------CRIA E ABRE FIFO DO CLIENTE--------------------------------
			char pathToFifo[100];
			strcpy(pathToFifo, "/tmp/");
			strcat(pathToFifo, fifoName);
			mkfifo(pathToFifo, 0660);

			int fd_cl = -1;
				fd_cl = open(pathToFifo, O_RDONLY);
			putchar('\n');
			//----------------------------------------------------------------
			char endMessage[100] = "";
			readline(fd_cl, endMessage); //Le info de retorno
			printf("\n%s", endMessage);
			close(fd_cl);
			return 0;
		}
		i++;
	}
	i = 0;
	while (i < nClients)
	{
		wait();
		i++;
	}

	return 0;
}