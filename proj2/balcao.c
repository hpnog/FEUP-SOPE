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

#define BUFSIZE 5
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

typedef struct
{
	int openingTime;
	char * nameOfMem;
} args_struct;

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
SharedMem * createSharedMemory(char* shm_name,int shm_size)
{
	int shmfd;
	int exists = 0;
	SharedMem *shm;					//create the shared memory region
	shmfd = shm_open(shm_name,O_CREAT|O_RDWR | O_EXCL,0660);		
	if(shmfd<0)
	{
		shmfd = shm_open(shm_name, O_RDWR, 0660);
		if (shmfd <= 0)
		{
			exists = -1;
			perror("Failure in shm_open()");
			return NULL;
		}
		else
			exists = 1;
	}								//specify the size of the shared memory region
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
	if (exists == 0)
	{
		shm->openingTime = time(NULL);
		shm->numeroDeBalcoes = 1;
		shm->numeroDeBalcoesExecucao = 1;
	}
	if (exists == 1)
	{
		shm->numeroDeBalcoes++;
		shm->numeroDeBalcoesExecucao++;
	}
	return (SharedMem *) shm;
}
//--------------------------------------------------------------------------------------------------

//---------------------------------DESTROI A MEMORIA PARTILHADA-------------------------------------
void destroySharedMemory(SharedMem *shm, int shm_size, char * shm_name)
{
	printf("\n\nTabela:\n\n");
	printf("N_B\tT\tDUR\tFIFO\tEM_AT\tJA_AT\tTMED\n");
	int i = 0;
	while (i < shm->numeroDeBalcoes)
	{
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", 
			shm->table[N_BALCAO][i],
			shm->table[N_TEMPO][i],
			shm->table[N_DURACAO][i],
			shm->table[N_FIFO][i],
			shm->table[N_EM_ATENDIMENTO][i],
			shm->table[N_JA_ATENDIDOS][i],
			shm->table[TEMPO_MEDIO_ATENDIMENTO][i]);
		i++;
	}

	if (munmap(shm,shm_size) < 0)
	{
		perror("Failure in munmap()");
		exit(EXIT_FAILURE);
	}
	if
		(shm_unlink(shm_name) < 0)
	{
		perror("Failure in shm_unlink()");
		exit(EXIT_FAILURE);
	}
} 
//--------------------------------------------------------------------------------------------------
void *thr_balcao(void *arg)
{
	args_struct *args = (args_struct*) arg;

	printf("\nEntrou na thread do balcao\n");

	//-------------------CRIA E ABRE O FIFO DO BALCAO---------
	char fifoName[100] = "/tmp/fb_";
	char pid[50];
	sprintf(pid, "%d", getpid());
	strcat(fifoName, pid);
	mkfifo(fifoName, 0660);

	int fd = open(fifoName, O_RDONLY | O_NONBLOCK);
	putchar('\n');
	//-------------------------------------------------------
	SharedMem * shm;
	shm = createSharedMemory(args->nameOfMem, sizeof(SharedMem));

	printf("Name of the memory: %s\n", args->nameOfMem);
	printf("Opening Time: %d\n", args->openingTime);
	printf("\nVariaveis da mem partilhada:\n\n");
	printf("Tempo de abertura: ");
	printf("%s", ctime(&shm->openingTime));
	printf("\nNumero de balcoes: %d\n", shm->numeroDeBalcoes);

	shm->table[N_BALCAO][shm->numeroDeBalcoes-1] = shm->numeroDeBalcoes;
	shm->table[N_TEMPO][shm->numeroDeBalcoes-1] = time(NULL) - shm->openingTime;
	printf("\nTempo: %d", shm->table[N_TEMPO][shm->numeroDeBalcoes-1]);
	shm->table[N_DURACAO][shm->numeroDeBalcoes-1] = -1;					//a alterar quando o balcao fecha
	shm->table[N_FIFO][shm->numeroDeBalcoes-1] = getpid();
	shm->table[N_EM_ATENDIMENTO][shm->numeroDeBalcoes-1] = 0;			//a alterar sempre que um cliente envia info
	shm->table[N_JA_ATENDIDOS][shm->numeroDeBalcoes-1] = 0;				//a alterar sempre que um cliente termina o seu atendimento
	shm->table[TEMPO_MEDIO_ATENDIMENTO][shm->numeroDeBalcoes-1] = 0;	//a alterar semrpe que um cliente termina o seu atendimento
	int nBalcao = shm->numeroDeBalcoes-1;

	int startAssisting = time(NULL); 
	int elapsedTime = time(NULL) - startAssisting;

	while (elapsedTime < args->openingTime)		
	{
		char str[100] = "";
		if (readline(fd, str))
		{
			printf("\nNext Client: %s\n", str);

			//--------------------ABRE/FECHA FIFO DO CLIENTE E ENVIA INFO----------
			char fifo_c[100] = "/tmp/";
			strcat(fifo_c, str);
			int fifo_cl_int = -1;
			do
			{		
				fifo_cl_int = open(fifo_c, O_WRONLY);
				if (fifo_cl_int == -1) sleep(1);
			} while (fifo_cl_int == -1);
			int waitTime;
			if (shm->table[N_EM_ATENDIMENTO][nBalcao] > 10)
				waitTime = 10;
			else
				waitTime = shm->table[N_EM_ATENDIMENTO][nBalcao];
			sleep(waitTime);
			char endMessage[100];
			sprintf(endMessage, "fim_atendimento");

			shm->table[N_EM_ATENDIMENTO][nBalcao]--;
			shm->table[N_JA_ATENDIDOS][nBalcao]++;

			int messagelen=strlen(endMessage)+1;
			write(fifo_cl_int, endMessage,messagelen);
			close(fifo_cl_int);
			//----------------------------------------------------------------

		}
		elapsedTime = time(NULL) - startAssisting;
	}

	shm->table[N_DURACAO][nBalcao] = args->openingTime;
	shm->table[TEMPO_MEDIO_ATENDIMENTO][nBalcao] = shm->table[N_DURACAO][nBalcao] / shm->table[N_JA_ATENDIDOS][nBalcao];


	printf("\nBalcao esteve aberto %d tempo\n", args->openingTime);
	printf("\nNumero de balcoes em execucao: %d\n\n", shm->numeroDeBalcoesExecucao);

	if (shm->numeroDeBalcoesExecucao == 1)
		destroySharedMemory(shm, sizeof(SharedMem), args->nameOfMem);
	else
		shm->numeroDeBalcoesExecucao--;

	free(args);		//liberta memoria

	close(fd);		//fecha o fifo

	pthread_exit(NULL);
} 

int main(int argc, char *argv[])
{
	setbuf(stdout, NULL);
	if (argc != 3)
	{
		printf("Wrong number of arguments\n");
		return 1;
	}

	args_struct *toSend;
	toSend = (args_struct *) malloc(sizeof(args_struct));
	toSend->openingTime = atoi(argv[2]);
	toSend->nameOfMem = argv[1];

	pthread_t desk_thread;
	pthread_create(&desk_thread, NULL, thr_balcao, (void*) toSend);

	pthread_exit(NULL);

	return 0;
}