// PROGRAMA p6a.c

#include <stdio.h>
#include <stdlib.h>

#define BUF_LENGTH 256
#define MAX 256

int main(void) {
	
	FILE *src, *dst;
	char buf[BUF_LENGTH];
	if ((src = fopen("infile.txt", "r")) == NULL) {
		perror("Error opening the file (1)");
		exit(1);
		}
	if ((dst = fopen("outfile.txt", "w")) == NULL) {
		perror("Error opening the file (2)");		
		exit(2);
		}
	while ((fgets(buf, MAX, src)) != NULL)
		fputs(buf, dst);
	fclose(src);
	fclose(dst);
	exit(0);		//zero e geralmente indicativo de "sucesso"
}
