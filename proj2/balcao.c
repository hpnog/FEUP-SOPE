#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <pthread.h> 
#include <semaphore.h> 
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFSIZE 5

typedef struct
{
	pthread_mutex_t buffer_lock;
	pthread_cond_t slots_cond;
	pthread_cond_t items_cond;
	pthread_mutex_t slots_lock;
	pthread_mutex_t items_lock;
	int buffer[BUFSIZE];
	int bufin;
	int bufout;
	int items;
	int slots;
	int sum; 
	int numeroDeBalcoes;
	
} SharedMem;

typedef struct
{
	int openingTime;
	char * nameOfMem;
} args_struct;

//----------------------------------------CRIA MEMORIA PARTILHADA-------------------------------------------
SharedMem * createSharedMemory(char* shm_name,int shm_size)
{
	int shmfd;
	SharedMem *shm;					//create the shared memory region
	shmfd = shm_open(shm_name,O_CREAT|O_RDWR,0660);		// try with O_EXCL
	if(shmfd<0)
	{
		perror("Failure in shm_open()");
		return NULL;
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
	shm->bufin = 0;
	shm->bufout = 0;
	shm->items = 0;
	shm->slots = BUFSIZE;
	shm->sum = 0;
	return
	(SharedMem *) shm;
}
//--------------------------------------------------------------------------------------------------

//---------------------------------DESTROI A MEMORIA PARTILHADA-------------------------------------
void destroySharedMemory(SharedMem *shm, int shm_size, char * shm_name)
{
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
	SharedMem * shm;
	shm = createSharedMemory(args->nameOfMem, sizeof(SharedMem));

	printf("Name of the memory: %s\n", args->nameOfMem);
	printf("Opening Time: %d\n", args->openingTime);
	getc(stdin);

	destroySharedMemory(shm, sizeof(SharedMem), args->nameOfMem);
	pthread_exit(NULL);
} 

int main(int argc, char *argv[])
{
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

	free(toSend);

	return 0;
}