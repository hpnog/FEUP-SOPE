// PROGRAMA p07_client.c 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>


int readline(int fd, char *str); 

int main(void) 
{ 
  setbuf(stdout, NULL);

  int   fifo_ans, fifo_req;

  printf("\nWill now attempt to open the FIFO communication chanels");

  do {
   fifo_req=open("/tmp/fifo_req",O_WRONLY);
   if (fifo_req == -1)
   {
    printf("\nAttempt failed - 2.");
    sleep(1); 
  }
} while (fifo_req == -1); 

do {
 fifo_ans=open("/tmp/fifo_ans",O_RDONLY | O_NONBLOCK); 
 if (fifo_ans == -1)
 {
  printf("\nAttempt failed - 1.");
  sleep(1); 
}
} while (fifo_ans == -1); 

printf("\nChannels opened");

char number1[20], number2[20];

number1[0] = '\0';
number2[0] = '\0';

 //do
 //{
printf("\nPlease write two Numbers:\nNumber1: ");
fgets(number1, 20, stdin);

printf("\nNumber2: ");
fgets(number2, 20, stdin);

int messagelen = strlen(number1) + 1; 
write(fifo_req,number1,messagelen); 
messagelen = strlen(number2) + 1; 
write(fifo_req,number2,messagelen);

char resultSum[50], resultDiff[50], resultProduct[50], resultQuo[50];

printf("\nBefore reading\n");
readline(fifo_ans, resultSum);
/*readline(fifo_ans, resultDiff);
readline(fifo_ans, resultProduct);
readline(fifo_ans, resultQuo);
*/

printf("%s", resultSum);
printf("\nAfter reading\n");

/*printf("%s", resultDiff);
printf("%s", resultProduct);
printf("%s", resultQuo);*/

 //} while ();

close(fifo_ans);
close(fifo_req); 

unlink(fifo_req);
unlink(fifo_ans);

return 0; 
} 

int readline(int fd, char *str) 
{ 
  char * temp = str;
  int n; 
  do { 
    n = read(fd,str,1); 
    printf("\n\nChar\n%c\n", *str);
  } while (n > 0 && *(str+1) != '\0' && *str != '\n'); 
  printf("\nAfter reading: %s\n", temp);
  return (n>0); 
}