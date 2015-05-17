#include <sys/types.h>
#include <sys/mnam.h>
#include <unistd.h>
#include <fcntl.h>

char SHM_NAME[] = "/shmVirtualShop";	//Nome da memoria partilhada

typedef struct
{
	int numeroDeBalcoes;
	/**
	 *	PARA COMPLETAR
	 */
} SharedMem;

int main(int argc, char *argv[])
{
	SharedMem * shm;
	/**
	 *	O_CREATE flag para criar caso ainda nao exista
	 *	O_RDWR flag para ter acesso de escrita e leitura
	 *	O_EXCL flag para retornar erro caso já exista
	 */
	int shmfd = shm_open(SHM_NAME, O_CREATE | O_RDWR | O_EXCL, 0660);
	
	//Verifica se houve erro ou nao
	if (shmfd < 0)	
	{
		shmfd = shm_open(SHM_NAME, O_RDWR, 0660);
		if (shmfd < 0)
		{
			perror("Error in shm_open([...])");
			return 1;
		}
	}

	shm = 
}