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

typedef struct
{
	time_t atOpen;
	int openingTime;
	char nameOfMem[MAX_NUMBER_LINE];
} args_struct;

typedef struct
{
	SharedMem *shm;
	char pathToFifo[MAX_NUMBER_LINE];
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
void printTime(FILE * logFile)
{
	char buffer[26];
	time_t timer;
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(logFile, " %-24s", buffer);
    fprintf(logFile, "| ");
}

void initializeLogFile(SharedMem *shm)
{
	shm->logFile = fopen(shm->nameOfLog, "w");

	fprintf(shm->logFile, " quando                  | quem      | balcao | o_que                        | canal_criado/usado\n");
	fprintf(shm->logFile, "-------------------------------------------------------------------------------------------------------------\n");

	fclose(shm->logFile);
}

void printOnLogPid(FILE * logFile, char * nameOfLog,char * who, int num, char * message, char * pid)
{	
	logFile = fopen(nameOfLog, "a");
	printTime(logFile);
	fprintf(logFile, " %-9s| %-7d| %-29s| %-17s\n", who,num+1, message, pid);
	fclose(logFile);
}

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
			perror("shm_open()");
			exit(EXIT_FAILURE);
		}
		else
			exists = 1;
	}								//specify the size of the shared memory region
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
	if (exists == 0)
	{
		shm->openingTime = time(NULL);
		shm->numeroDeBalcoes = 1;
		shm->numeroDeBalcoesExecucao = 1;
		char * nameOfLog = malloc(sizeof(char) * MAX_NUMBER_LINE);
		strcpy(nameOfLog, shm_name);
		nameOfLog++;
		strcat(nameOfLog, ".log");
		strcpy(shm->nameOfLog, nameOfLog);
		nameOfLog--;
		free(nameOfLog);

		initializeLogFile(shm);
		
		printOnLogPid(shm->logFile, shm->nameOfLog,"Balcao", 0, "inicia_mem_partilhada       ", "-");

		//Inicializa o mutex
		pthread_mutex_init(&shm->mutexLock, NULL);
		
	}
	if (exists == 1)
	{
		while (pthread_mutex_trylock(&shm->mutexLock)) {}
		shm->numeroDeBalcoes++;
		shm->numeroDeBalcoesExecucao++;
		pthread_mutex_unlock(&shm->mutexLock);
	}
	return (SharedMem *) shm;
}
//--------------------------------------------------------------------------------------------------

//---------------------------------DESTROI A MEMORIA PARTILHADA-------------------------------------
void destroySharedMemory(SharedMem *shm, int nBalcao, int shm_size, char * shm_name)
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

	char pidN[MAX_NUMBER_LINE];
	sprintf(pidN,"fb_%d", getpid());
	printOnLogPid(shm->logFile, shm->nameOfLog,"Balcao", nBalcao, "fecha_loja", pidN);

	if (munmap(shm,shm_size) < 0)
	{
		perror("munmap()");
		exit(EXIT_FAILURE);
	}
	if
		(shm_unlink(shm_name) < 0)
	{
		perror("shm_unlink()");
		exit(EXIT_FAILURE);
	}
} 
//--------------------------------------------------------------------------------------------------

void *thr_atendimento(void *arg)
{
	args2_struct *args = (args2_struct*) arg;

	int fifo_cl_int = -1;
	do
	{
		fifo_cl_int = open(args->pathToFifo, O_RDWR);
	} while (fifo_cl_int == -1);
	
	int waitTime;

	while (pthread_mutex_trylock(&args->shm->mutexLock)) {}
	int tempo = args->shm->table[N_EM_ATENDIMENTO][args->nBalcao];
	pthread_mutex_unlock(&args->shm->mutexLock);

	if (tempo > 10)
		waitTime = 10;
	else
		waitTime = tempo;

	sleep(waitTime);

	while (pthread_mutex_trylock(&args->shm->mutexLock)) {}
	args->shm->table[N_EM_ATENDIMENTO][args->nBalcao]--;
	args->shm->table[N_JA_ATENDIDOS][args->nBalcao]++;
	pthread_mutex_unlock(&args->shm->mutexLock);

	char endMessage[] = "fim_atendimento";

	int messagelen = strlen(endMessage)+1;


	if (write(fifo_cl_int, endMessage,messagelen) < 0)
		perror("write()");
	else
	{
		char * pathT = malloc(sizeof(char) * MAX_NUMBER_LINE);
		strcpy(pathT, args->pathToFifo);
		pathT += 5;
		printOnLogPid(args->shm->logFile, args->shm->nameOfLog,"Balcao", args->nBalcao, "fim_atendimento_cliente", pathT);
		pathT -= 5;
		free(pathT);
		printf("\nSent final message to %s", args->pathToFifo);
			
	}

	if (close(fifo_cl_int) < 0)
		perror("close()");

	free(args);
	pthread_exit(NULL);
}

void *thr_balcao(void *arg)
{
	args_struct *args = (args_struct*) arg;

	printf("\nEntrou na thread do balcao\n");

	//-------------------CRIA E ABRE O FIFO DO BALCAO---------
	char fifoName[MAX_NUMBER_LINE] = "/tmp/fb_";
	char pid[MAX_NUMBER_LINE];
	sprintf(pid, "%d", getpid());
	strcat(fifoName, pid);
	mkfifo(fifoName, 0660);

	int fd = open(fifoName, O_RDONLY | O_NONBLOCK);

	if (fd == -1)
		perror("open()");
	//-------------------------------------------------------
	SharedMem * shm;
	shm = createSharedMemory(args->nameOfMem, sizeof(SharedMem));

	printf("Name of the memory: %s\n", args->nameOfMem);
	printf("Opening Time: %d\n", args->openingTime);
	printf("\nVariaveis da mem partilhada:\n\n");
	printf("Tempo de abertura: ");
	printf("%s", ctime(&shm->openingTime));
	printf("\nNumero de balcoes: %d\n", shm->numeroDeBalcoes);

	int nBalcao = shm->numeroDeBalcoes-1;
	shm->table[N_BALCAO][nBalcao] = shm->numeroDeBalcoes;
	shm->table[N_TEMPO][nBalcao] = time(NULL) - shm->openingTime;
	shm->table[N_DURACAO][nBalcao] = -1;					//a alterar quando o balcao fecha
	shm->table[N_FIFO][nBalcao] = getpid();
	shm->table[N_EM_ATENDIMENTO][nBalcao] = 0;			//a alterar sempre que um cliente envia info
	shm->table[N_JA_ATENDIDOS][nBalcao] = 0;				//a alterar sempre que um cliente termina o seu atendimento
	shm->table[TEMPO_MEDIO_ATENDIMENTO][nBalcao] = 0;	//a alterar semrpe que um cliente termina o seu atendimento
	
	char pidW[MAX_NUMBER_LINE];
	sprintf(pidW, "fb_%d", getpid());
	printOnLogPid(shm->logFile, shm->nameOfLog,"Balcao", nBalcao, "inicia_linh_mem_partilhada", pidW);

	int startAssisting = time(NULL); 
	int elapsedTime = time(NULL) - startAssisting;

	pthread_t answer_thread[5000];
	int threadCounter = 0;

	while (elapsedTime < args->openingTime)		
	{
		char str[MAX_NUMBER_LINE];
		if (readline(fd, str))
		{
			//--------------------ABRE/FECHA FIFO DO CLIENTE E ENVIA INFO----------
			args2_struct *toSend = malloc(sizeof(args2_struct));

			char fifo_c[MAX_NUMBER_LINE];
			strcpy(fifo_c, "/tmp/");
			strcat(fifo_c, str);

			toSend->shm = shm;
			strcpy(toSend->pathToFifo, fifo_c);
			toSend->nBalcao = nBalcao;

			printf("\nWhat is about to be sent to thr_atendimento: |%s|", toSend->pathToFifo);

			if (pthread_create(&answer_thread[threadCounter], NULL, thr_atendimento, (void*) toSend) == 0)
						threadCounter++;
					else
						perror("pthread_create()");
			//----------------------------------------------------------------

		}
		elapsedTime = time(NULL) - startAssisting;
	}

	printf("\nNumero de threads: %d", threadCounter);

	if (close(fd) < 0) perror("close()");		//fecha o fifo

	int j = 0;
	while (threadCounter > j)
	{
		if (pthread_join(answer_thread[j], NULL) != 0) perror("pthread_join()");
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

	char pidW2[MAX_NUMBER_LINE];
	sprintf(pidW2, "fb_%d", getpid());
	printOnLogPid(shm->logFile, shm->nameOfLog, "Balcao", nBalcao, "fecha_balcao", pidW2);
	printf("\n\n\nTotal de clientes atendidos: %d", total);
	printf("\nBalcao esteve aberto %d tempo\n", args->openingTime);
	printf("\nNumero de balcoes em execucao: %d\n\n", shm->numeroDeBalcoesExecucao);
	if (shm->numeroDeBalcoesExecucao == 1)
		destroySharedMemory(shm,nBalcao, sizeof(SharedMem), args->nameOfMem);
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
	strcpy(toSend->nameOfMem, argv[1]);
	toSend->atOpen = time(NULL);

	pthread_t desk_thread;
	if (pthread_create(&desk_thread, NULL, thr_balcao, (void*) toSend) != 0) perror("pthread_create()");

	pthread_exit(NULL);

	return 0;
}
