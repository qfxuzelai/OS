#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RAND_NUM 1000000    // num of random number
#define MAX_VAL 1000000000  // max value of random number

int main()
{
    /*** define variables ***/
    int cnt;
    FILE *fp;

    /*** open file ***/
    if((fp = fopen("test.in", "w+")) == NULL)
    {
        printf("error: the file cannot be opened.\n");
        exit(EXIT_FAILURE);
    }

    /*** generate random number and write to file ***/
    srand((int)time(0));
    for(cnt=0; cnt<RAND_NUM; cnt++)
    {
        fprintf(fp,"%d ",rand()%MAX_VAL);
    }
    fclose(fp);

    exit(EXIT_SUCCESS);
}
