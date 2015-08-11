# SOPE-code

Este repositório contém o código realizado nas fichas e projetos da Unidade Curricular de SOPE - FEUP.

## Projeto - 1 (Índisse remissivo)

### C
#### Index

```c
#include <unistd.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define CORRECT_EXIT 0
#define ERROR_EXIT 1
#define TRUE 1

int main(int argc, char * argv[])
{
	int stat;
	char * pathToDir = malloc((strlen(argv[1]) + 1) * sizeof(char));								//Como argumento recebe o caminho para o diretório
	if (argv[1][strlen(pathToDir) - 1] == '/')
		strncpy(pathToDir, argv[1], strlen(argv[1]) - 1);
	else
		strncpy(pathToDir, argv[1], strlen(argv[1]));

	//------------------------CRIA CAMINHO PARA WORDS E VERIFICA SE ESTA CORRETO----------------
	char * pathToWords = malloc((strlen(pathToDir) + 11) * sizeof(char));				//Cria char[] para o caminho para words.txt
	strncpy(pathToWords, pathToDir, strlen(pathToDir));
	strcat(pathToWords, "/words.txt");	

	if (access(pathToWords, F_OK) != 0)						//Verifica se words.txt existe
		exit(ERROR_EXIT);									//Se nao existir terimna e retorna 1
	
	//-------------------------------------------------------------------------------------------
	int counterOfFiles = 1;
	do
	{
		//----------CREATES NAME TO NEXT FILE AND CHECKS IT OUT----------------------------------
		char stringNum[10];
		sprintf(stringNum, "%d", counterOfFiles);										//gets number of the file into string
		char * pathToFile = malloc((strlen(pathToDir) + 11) * sizeof(char));					//Cria char[] para o caminho para words.txt
		strcpy(pathToFile, pathToDir);
		strcat(pathToFile, "/");																//Adiciona ao caminho de dir - /words.txt
		strcat(pathToFile, stringNum);	
		strcat(pathToFile, ".txt");
		if (access(pathToFile, F_OK) != 0)					//Verifica se words.txt existe
			break;	
		//-------------------------------------------------------------------------------------------

		pid_t pid = fork();
		if (pid == 0)
			execlp("./bin/sw", "./bin/sw", pathToWords, pathToFile, NULL);
		else if (pid > 0)
		{
			counterOfFiles++;
			wait(&stat);
		}
		else
		{
			fprintf(stderr, "\nError in Fork in index\n");
			exit(ERROR_EXIT);
		}

		
	} while (TRUE);

	execlp("./bin/csc", "./bin/csc", pathToDir, NULL);

	exit(CORRECT_EXIT);
}
```

#### csc

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define CORRECT_EXIT 0
#define ERROR_EXIT 1
#define READING_PIPE 0
#define WRITING_PIPE 1
#define MAX_SIZE_OF_LINE 50
#define TRUE 1





int main(int argc, char * argv[])
{	
	int stat;

	int cscPipe[2];
	if (pipe (cscPipe) < 0) {
		fprintf(stderr, "\nError creating Pipe\n");
		exit(ERROR_EXIT);
	}

	char * pathToDir = argv[1];
	char * pathToIndex = malloc((strlen(pathToDir) + 11) * sizeof(char));
	strncpy(pathToIndex, pathToDir, strlen(pathToDir) + 1);
	strcat(pathToIndex, "/index.txt");

	//------------------PUTS CONTENT ON PIPE AND REMOVES FILE---------------------------
	char nameOfFile[20] = "";
	int fileCounter = 1;
	while (TRUE)
	{
		sprintf(nameOfFile, "%d", fileCounter);
		strcat(nameOfFile, "_temp.txt");
		if (access(nameOfFile, F_OK) != 0)
			break;

		pid_t pid = fork();
		if (pid == 0) {
			close(cscPipe[READING_PIPE]);		
			dup2(cscPipe[WRITING_PIPE], STDOUT_FILENO);
			execlp("cat", "cat", nameOfFile, NULL);
		}
		else if (pid < 0)
		{
			fprintf(stderr, "\nError in Fork in csc\n");
			exit(ERROR_EXIT);
		}
		else
			wait(&stat);
		fileCounter++;

		//------------REMOVES FILE----------------------
		pid_t pid2 = fork();
		if (pid2 == 0)
			execlp("rm", "rm", nameOfFile, NULL);
		else if (pid2 < 0)
		{
			fprintf(stderr, "\nError in Fork in csc\n");
			exit(ERROR_EXIT);
		}
		else
			wait(&stat);
		//----------------------------------------------
	}
	//----------------------------------------------------------------------------------

	close(cscPipe[WRITING_PIPE]);
	FILE *readStream;
	char line[MAX_SIZE_OF_LINE];
	readStream = fdopen(cscPipe[READING_PIPE], "r");

	//---------------------------WRITES EVERYTHING ON FILE------------------------------
	FILE * outFile = NULL;
	outFile = fopen(pathToIndex, "w");
	if (outFile == NULL)
		fclose (outFile);

	while (fgets(line, MAX_SIZE_OF_LINE, readStream) != NULL)
		fputs(line, outFile);

	fclose(outFile);
	//----------------------------------------------------------------------------------
	int sortPipe[2];
	if (pipe (sortPipe) < 0) {
		fprintf(stderr, "\nError creating Pipe\n");
		exit(ERROR_EXIT);
	}
	//----------------------------SORT OF THE FILE INTO THE PIPE------------------------
	char pathToIndexTemp[strlen(pathToIndex) + 4];
	strncpy(pathToIndexTemp, pathToIndex, strlen(pathToIndex) - 4);
	strcat(pathToIndexTemp, "temp.txt");
	pid_t pid = fork();
	if (pid == 0) {
		execlp("sort", "sort", "-V", "-f", pathToIndex, "-o", pathToIndexTemp, NULL);
	}
	else if (pid < 0)
	{
		fprintf(stderr, "\nError in Fork in csc\n");
		exit(ERROR_EXIT);
	}
	else {
		wait(&stat);
	}
	//----------------------------------------------------------------------------------
	//---------------------------WRITES EVERYTHING ON FILE------------------------------

	readStream = NULL;
	outFile = NULL;

	readStream = fopen(pathToIndexTemp, "r");
	outFile = fopen(pathToIndex, "w");

	if (outFile == NULL)
		fclose (outFile);

	if (readStream == NULL)
		fclose(readStream);

	char title[] = "INDEX";
	fputs(title, outFile);

	char * word = malloc((1 + MAX_SIZE_OF_LINE) * sizeof(char));
	while (fgets(line, MAX_SIZE_OF_LINE, readStream) != NULL)
	{
		int i;
		char wordTemp[MAX_SIZE_OF_LINE] = "";
		for (i = 0; line[i] != ' ' && i < (MAX_SIZE_OF_LINE - 1); i++)
			wordTemp[i] = line[i];
		wordTemp[i] = '\0';

		if (strncmp(word, wordTemp, strlen(wordTemp)) != 0)
		{
			strcat(word, "\n\n");
			fputs(word, outFile);
			word = malloc((10 + MAX_SIZE_OF_LINE) * sizeof(char));
			strncpy(word, wordTemp, strlen(wordTemp) + 1);
			strcat(word, ": ");
			char a[10] = "";
			strncpy(a, line + strlen(wordTemp) + 3, strlen(line) - 4 - strlen(wordTemp));
			strcat(word, a);
		}
		else
		{
			char a[10] = "";
			strncpy(a, line + strlen(wordTemp) + 3, strlen(line) - 4 - strlen(wordTemp));
			strcat(word, ", ");
			strcat(word, a);
		}
	}
	strcat(word, "\n");
	fputs(word, outFile);

	fclose(outFile);
	fclose(readStream);
	
	//------------REMOVES FILE----------------------
	pid_t pid3 = fork();
	if (pid3 == 0)
		execlp("rm", "rm", pathToIndexTemp, NULL);
	else if (pid3 < 0)
	{
		fprintf(stderr, "\nError in Fork in csc\n");
		exit(ERROR_EXIT);
	}
	else
		wait(&stat);
		//----------------------------------------------
	//----------------------------------------------------------------------------------

	exit(CORRECT_EXIT);
}
```

#### sw

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define CORRECT_EXIT 0
#define ERROR_EXIT 1
#define READING_PIPE 0
#define WRITING_PIPE 1
#define MAX_SIZE_OF_LINE 50


/*
 * argv[1] = nome do ficheiro
 * argv[2] = lista de palavras
 */

 int main(int argc, char *argv[])
 {
 	int stat;
 	char *pathToFile = malloc((strlen(argv[2]) + 1) * sizeof(char));
 	char *pathToWords = malloc((strlen(argv[1]) + 1) * sizeof(char));
 	strncpy(pathToFile, argv[2], strlen(argv[2]) + 1);
 	strncpy(pathToWords, argv[1], strlen(argv[1]) + 1);

 	int swPipe[2];
 	FILE *readStream;

 	//--------------------Verificacoes de erro dos pipes
 	if (pipe (swPipe) < 0) {
 		fprintf(stderr, "\nError creating Pipe\n");
 		exit(1);
 	}
 	//--------------------------------------------------

 	pid_t pid = fork();
 	if (pid < 0)
 	{
 		fprintf(stderr, "\nError in Fork in sw\n");
 		exit(ERROR_EXIT);
 	}
 	else if (pid == 0) 
 	{
 		close(swPipe[READING_PIPE]);		
 		dup2(swPipe[WRITING_PIPE], STDOUT_FILENO);
 		execlp("grep","grep","-n", "-o", "-w", "-f", pathToWords, pathToFile, NULL);
 	}
 	else
 	{
		wait(&stat);
 		close(swPipe[WRITING_PIPE]);		
 		readStream = fdopen(swPipe[READING_PIPE], "r");
 		char line[MAX_SIZE_OF_LINE];
 		//-----------------CREATE FILE AND STORE DATA IN IT--------------------------------
 		//-----CREATE FILE------
 		char * lastSlash = strrchr(pathToFile, '/');
 		if (lastSlash == NULL)
 			lastSlash = pathToFile;
 		int numberOfLastSlash = lastSlash - pathToFile;
 		char nameOfOutput[200] = "";
 		strncpy(nameOfOutput, lastSlash + 1, strlen(pathToFile) - numberOfLastSlash - 5);	//4 simboliza o numero de caracteres em: .txt
 		
 		char nameToPrint[200] = "";
 		strncpy(nameToPrint, nameOfOutput, strlen(nameOfOutput));

 		strcat(nameOfOutput, "_temp.txt");
 		
 		FILE * outFile = NULL;

 		outFile = fopen(nameOfOutput, "w");
 		 if (outFile == NULL)
  		{
    		fclose (outFile);
  		}
 		//----------------------
 			while (fgets(line, MAX_SIZE_OF_LINE, readStream) != NULL)
 			{
 			//Introduz a palavra
 			char * lineNumber;										//char * com o numero da linha
 			char destination[200];										//char * com a informacao da palavra
 			int number;												//int com a posicao na string
 			lineNumber = strchr(line, ':');							//le ate aos 2 pontos
 			number = lineNumber - line;
 			strncpy(destination, line + number + 1, strlen(line) - number);	//copia o nome da palavra

 			destination[strlen(destination) - 1] = '\0';

 			//introduz os 2 pontos
 			strcat(destination, " : ");								
 			
 			//introduz o nome do novo ficheiro
 			strcat(destination, nameToPrint);

 			//introduz o hifen
 			strcat(destination, "-");

 			//introduz o numero da linha
 			char num[200];
 			strncpy(num, line, number);
 			strcat(destination, num);

 			strcat(destination, "\n");

 			fputs(destination, outFile);
 		}
		//----------------------------------------------------------------------------------
 		//----CLOSE FILE-------
 		fclose(outFile);
 		//---------------------

 		fclose(readStream);
 		close(swPipe[READING_PIPE]);
 	}
 	exit(CORRECT_EXIT);
 }
 ```
 
## Projeto - 2 (cliente - balcão)
### C
 
#### balcao
 
 ```c
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <pthread.h> 
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

typedef struct
{
	int dur;
	char pathToFifo[MAX_NUMBER_LINE];

} args3_struct;

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
	printf("N_B\tT\tDUR\tFIFO\tEM_AT\tJA_AT\tTMED\n");
	int i = 0;
	while (i < shm->numeroDeBalcoes)
	{
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%f\n", 
			(int) shm->table[N_BALCAO][i],
			(int) shm->table[N_TEMPO][i],
			(int) shm->table[N_DURACAO][i],
			(int) shm->table[N_FIFO][i],
			(int) shm->table[N_EM_ATENDIMENTO][i],
			(int) shm->table[N_JA_ATENDIDOS][i],
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
void *thr_fifoOpener(void * arg)
{
	args3_struct *args = (args3_struct*) arg;
	int fifo = open(args->pathToFifo, O_WRONLY | O_NONBLOCK);
	sleep(args->dur);
	printf("\nTempo de abertura passou");
	close(fifo);
	free(arg);
	pthread_exit(NULL);
}


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

	//-------------------CRIA E ABRE O FIFO DO BALCAO---------
	char fifoName[MAX_NUMBER_LINE] = "/tmp/fb_";
	char pid[MAX_NUMBER_LINE];
	sprintf(pid, "%d", getpid());
	strcat(fifoName, pid);
	mkfifo(fifoName, 0660);

	int fd = open(fifoName, O_RDONLY | O_NONBLOCK);
	if (fd == -1)
		perror("open()");

	args3_struct * send = malloc(sizeof(args3_struct));
	send->dur = args->openingTime;
	strcpy(send->pathToFifo, fifoName);
	pthread_t fifoOpener;
	pthread_create(&fifoOpener, NULL, thr_fifoOpener, (void*) send);

	//-------------------------------------------------------

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
	if (pthread_join(fifoOpener, NULL) != 0) perror("pthread_join()");

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
```

#### ger_cl

```c
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
    fprintf(logFile, " %-24s", buffer);
    fprintf(logFile, "| ");
}

void printOnLog(SharedMem * shm,char * who, int num, char * message)
{

	shm->logFile = fopen(shm->nameOfLog, "a");
	printTime(shm->logFile);
	fprintf(shm->logFile, " %-9s| %-7d| %-29s| fc_%-14d\n", who,num, message, getpid());
	fclose(shm->logFile);
}

void printOnLogPid(SharedMem * shm,char * who, int num, char * message, int pid)
{

	shm->logFile = fopen(shm->nameOfLog, "a");
	printTime(shm->logFile);
	fprintf(shm->logFile, " %-9s| %-7d| %-29s| fc_%-14d\n", who,num, message, pid);
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
					char end[MAX_NUMBER_LINE];
					sprintf(end, "%s", endMessage);
					printOnLog(shm, "Cliente", numberOfDesk, end);
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
```
