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

	pthread_mutex_t mutexLock;

	int counter;

	int numeroDeBalcoes;
	int numeroDeBalcoesExecucao;
	time_t openingTime;
	double table [7][1000];
	
} SharedMem;

typedef struct
{
	time_t atOpen;
	int openingTime;
	char * nameOfMem;
} args_struct;

typedef struct
{
	SharedMem *shm;
	char * pathToFifo;
	int nBalcao;

} args2_struct;

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
			perror("\nFailure in shm_open()");
			return NULL;
		}
		else
			exists = 1;
	}								//specify the size of the shared memory region
	if (ftruncate(shmfd,shm_size) < 0)
	{
		perror("\nFailure in ftruncate()");
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
		//Inicializa o mutex
		pthread_mutex_init(&shm->mutexLock, NULL);
	}
	if (exists == 1)
	{
		pthread_mutex_lock(&shm->mutexLock);
		shm->numeroDeBalcoes++;
		shm->numeroDeBalcoesExecucao++;
		pthread_mutex_unlock(&shm->mutexLock);
	}
	return (SharedMem *) shm;
}
//--------------------------------------------------------------------------------------------------

//---------------------------------DESTROI A MEMORIA PARTILHADA-------------------------------------
void destroySharedMemory(SharedMem *shm, int shm_size, char * shm_name)
{
	printf("\n\nTabela:\n\n");
	printf("N_B\t\tT\t\tDUR\t\tFIFO\t\tEM_AT\t\tJA_AT\t\tTMED\n");
	int i = 0;
	while (i < shm->numeroDeBalcoes)
	{
		printf("%f\t%f\t%f\t%f\t%f\t%f\t%f\n", 
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
		perror("\nFailure in munmap()");
		exit(EXIT_FAILURE);
	}
	if
		(shm_unlink(shm_name) < 0)
	{
		perror("\nFailure in shm_unlink()");
		exit(EXIT_FAILURE);
	}
} 
//--------------------------------------------------------------------------------------------------
int terminated = 0;

void *thr_atendimento(void *arg)
{
	args2_struct *args = (args2_struct*) arg;

	int fifo_cl_int = -1;
	do
	{
		fifo_cl_int = open(args->pathToFifo, O_RDWR);
	} while (fifo_cl_int == -1);
	
	int waitTime;
	pthread_mutex_lock(&args->shm->mutexLock);
	if (args->shm->table[N_EM_ATENDIMENTO][args->nBalcao] > 10)
		waitTime = 10;
	else
		waitTime = args->shm->table[N_EM_ATENDIMENTO][args->nBalcao];
	pthread_mutex_unlock(&args->shm->mutexLock);
	sleep(waitTime);

	pthread_mutex_lock(&args->shm->mutexLock);
	args->shm->table[N_EM_ATENDIMENTO][args->nBalcao]--;
	args->shm->table[N_JA_ATENDIDOS][args->nBalcao]++;
	pthread_mutex_unlock(&args->shm->mutexLock);

	char endMessage[] = "fim_atendimento";

	int messagelen = strlen(endMessage)+1;
	write(fifo_cl_int, endMessage,messagelen);
	close(fifo_cl_int);

	terminated++;
	printf("\nTerminou uma thread de atendimento: %d", terminated);
	pthread_exit(NULL);
}


void *thr_balcao(void *arg)
{
	args_struct *args = (args_struct*) arg;

	//--------------------------INICIALIZA A CONDITION VARIABLE---------------------------

	//------------------------------------------------------------------------------------

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
	printf("\nTempo: %f", shm->table[N_TEMPO][shm->numeroDeBalcoes-1]);
	shm->table[N_DURACAO][shm->numeroDeBalcoes-1] = -1;					//a alterar quando o balcao fecha
	shm->table[N_FIFO][shm->numeroDeBalcoes-1] = getpid();
	shm->table[N_EM_ATENDIMENTO][shm->numeroDeBalcoes-1] = 0;			//a alterar sempre que um cliente envia info
	shm->table[N_JA_ATENDIDOS][shm->numeroDeBalcoes-1] = 0;				//a alterar sempre que um cliente termina o seu atendimento
	shm->table[TEMPO_MEDIO_ATENDIMENTO][shm->numeroDeBalcoes-1] = 0;	//a alterar semrpe que um cliente termina o seu atendimento
	int nBalcao = shm->numeroDeBalcoes-1;


	int startAssisting = time(NULL); 
	int elapsedTime = time(NULL) - startAssisting;

	pthread_t answer_thread[5000];
	int threadCounter = 0;

	args2_struct *toSend;
	toSend = (args2_struct *) malloc(sizeof(args2_struct));

	while (elapsedTime < args->openingTime)		
	{
		char str[100] = "";
		if (readline(fd, str))
		{
			//--------------------ABRE/FECHA FIFO DO CLIENTE E ENVIA INFO----------
			char fifo_c[] = "/tmp/";
			strcat(fifo_c, str);
			int fifo_cl_int = -1;

			toSend->shm = shm;
			toSend->pathToFifo = fifo_c;
			toSend->nBalcao = nBalcao;

			shm->table[N_EM_ATENDIMENTO][nBalcao]++;
			pthread_create(&answer_thread[threadCounter], NULL, thr_atendimento, (void*) toSend);
			threadCounter++;
			//----------------------------------------------------------------

		}
		elapsedTime = time(NULL) - startAssisting;
	}


	close(fd);		//fecha o fifo

	int j = 0;
	while (threadCounter > j)
	{
		pthread_join(answer_thread[j], NULL);
		printf("\nJoin: %d", (j + 1));
		j++;
	}
	printf("\nThread Counter: %d", threadCounter);

	shm->table[N_DURACAO][nBalcao] = args->openingTime;
	shm->table[TEMPO_MEDIO_ATENDIMENTO][nBalcao] = shm->table[N_DURACAO][nBalcao] / shm->table[N_JA_ATENDIDOS][nBalcao];
	shm->table[N_EM_ATENDIMENTO][nBalcao] = 0;
	shm->table[N_DURACAO][nBalcao] = time(NULL) - args->atOpen;
	int total = 0;
	int c = 0;
	while (c < shm->numeroDeBalcoes) 
	{
		total += shm->table[N_JA_ATENDIDOS][c];
		c++;
	}

	printf("\n\n\nTotal de clientes atendidos: %d", total);
	printf("\nBalcao esteve aberto %d tempo\n", args->openingTime);
	printf("\nNumero de balcoes em execucao: %d\n\n", shm->numeroDeBalcoesExecucao);
	if (shm->numeroDeBalcoesExecucao == 1)
		destroySharedMemory(shm, sizeof(SharedMem), args->nameOfMem);
	else
		shm->numeroDeBalcoesExecucao--;

	free(args);		//liberta memoria

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
	toSend->atOpen = time(NULL);

	pthread_t desk_thread;
	pthread_create(&desk_thread, NULL, thr_balcao, (void*) toSend);

	pthread_exit(NULL);

	return 0;
}