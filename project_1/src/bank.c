#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#define TIME_UNIT 10000   // 1 time unit = 10ms = 10000us
#define US_PER_S 1000000  // 1s = 1000000us

/*** global variables ***/
int num_to_serve;
int num_to_call = 0;
struct timeval t_start;
int *client_no, *enter_time, *serve_time;
int *start_time, *end_time, *serving_teller;

/*** mutexes and semaphores ***/
sem_t queue;
sem_t *sem_client;
pthread_mutex_t mutex_nts, mutex_ntc;
pthread_t *client_thread, *teller_thread;

/*** function declarations ***/
void *client(void *arg);
void *teller(void *arg);
int get_client_num(char *filename);

int main(int argc, char *argv[])
{
    /*** define variables ***/
    FILE *fp;
    int cnt, *temp;
    int client_num, teller_num;
    char *filename = "test.in";

    /*** check inputs ***/
    if(argc != 3)
    {
        printf("error: the number of arguments should be 3.\n");
        exit(EXIT_FAILURE);
    }
    if(strcmp(argv[1], "-n"))
    {
        printf("error: the second argument should be \"-n\".\n");
        exit(EXIT_FAILURE);
    }
    if((teller_num = atoi(argv[2])) < 1)
    {
        printf("error: the third argument should be a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    /*** get client_num and malloc ***/
    client_num = get_client_num(filename);
    num_to_serve = client_num;
    client_no = (int *)malloc(sizeof(int)*client_num);
    enter_time = (int *)malloc(sizeof(int)*client_num);
    serve_time = (int *)malloc(sizeof(int)*client_num);
    start_time = (int *)malloc(sizeof(int)*client_num);
    end_time = (int *)malloc(sizeof(int)*client_num);
    serving_teller = (int *)malloc(sizeof(int)*client_num);
    temp = (int *)malloc(sizeof(int)*(client_num+teller_num));
    sem_client = (sem_t *)malloc(sizeof(sem_t)*client_num);
    client_thread = (pthread_t *)malloc(sizeof(pthread_t)*client_num);
    teller_thread = (pthread_t *)malloc(sizeof(pthread_t)*teller_num);

    /*** read from file ***/
    if((fp = fopen("test.in", "r")) == NULL)
    {
        printf("error: the file cannot be opened.\n");
        exit(EXIT_FAILURE);
    }
    for(cnt=0; cnt<client_num; cnt++)
    {
        fscanf(fp, "%d%d%d", client_no+cnt, enter_time+cnt, serve_time+cnt);
    }
    fclose(fp);
    
    /*** init mutexes and semaphores ***/
    sem_init(&queue, 0, 0);
    pthread_mutex_init(&mutex_nts, NULL);
    pthread_mutex_init(&mutex_ntc, NULL);
    for(cnt=0; cnt<client_num; cnt++)
    {
        sem_init(sem_client+cnt, 0, 0);
    }

    /*** create threads ***/
    gettimeofday(&t_start, NULL);
    for(cnt=0; cnt<client_num; cnt++)
    {
        *(temp+cnt) = cnt;
        pthread_create(client_thread+cnt, NULL, client, (void *)(temp+cnt));
    }
    for(cnt=0; cnt<teller_num; cnt++)
    {
        *(temp+client_num+cnt) = cnt;
        pthread_create(teller_thread+cnt, NULL, teller, (void *)(temp+client_num+cnt));
    }

    /*** wait for threads to end ***/
    for(cnt=0; cnt<client_num; cnt++)
    {
        pthread_join(*(client_thread+cnt), NULL);
    }
    for(cnt=0; cnt<teller_num; cnt++)
    {
        pthread_join(*(teller_thread+cnt), NULL);
    }

    /*** print results ***/
    printf("No\tenter\tstart\tend\tteller\n");
    for(cnt=0; cnt<client_num; cnt++)
    {
        printf("%d\t%d\t%d\t%d\t%d\n", client_no[cnt], enter_time[cnt], start_time[cnt], end_time[cnt], serving_teller[cnt]);
    }

    /*** destroy mutexes and semaphores ***/
    for(cnt=0; cnt<client_num; cnt++)
    {
        sem_destroy(sem_client+cnt);
    }
    pthread_mutex_destroy(&mutex_ntc);
    pthread_mutex_destroy(&mutex_nts);
    sem_destroy(&queue);

    /*** free memory ***/
    free(teller_thread);
    free(client_thread);
    free(sem_client);
    free(temp);
    free(serving_teller);
    free(end_time);
    free(start_time);
    free(serve_time);
    free(enter_time);
    free(client_no);

    exit(EXIT_SUCCESS);
}

/// start funtion for client thread
void *client(void *arg)
{
    /*** define variables ***/
    int num;
    struct timeval t_end;

    /*** get arguement ***/
    num = *(int *)arg;

    /*** sleep until enter ***/
    usleep(TIME_UNIT*enter_time[num]);
    gettimeofday(&t_end, NULL);
    enter_time[num] = ((t_end.tv_sec-t_start.tv_sec)*US_PER_S+(t_end.tv_usec-t_start.tv_usec))/TIME_UNIT;
    
    /*** wait in queue ***/
    sem_post(&queue);

    /*** wait to be called ***/
    sem_wait(&sem_client[num]);
    gettimeofday(&t_end, NULL);
    start_time[num] = ((t_end.tv_sec-t_start.tv_sec)*US_PER_S+(t_end.tv_usec-t_start.tv_usec))/TIME_UNIT;  
    
    /*** sleep until serve end ***/
    usleep(TIME_UNIT*serve_time[num]);
    gettimeofday(&t_end, NULL);
    end_time[num] = ((t_end.tv_sec-t_start.tv_sec)*US_PER_S+(t_end.tv_usec-t_start.tv_usec))/TIME_UNIT;

    /*** thread exit ***/
    pthread_exit(NULL);
    return NULL;
}

/// start funtion for teller thread
void *teller(void *arg)
{
    /*** define variables ***/
    int num, nts, ntc;

    /*** get arguement ***/
    num = *(int *)arg;

    /*** read and update n_t_s ***/
    pthread_mutex_lock(&mutex_nts);
    nts = num_to_serve;
    num_to_serve--;
    pthread_mutex_unlock(&mutex_nts);

    while(nts > 0)
    {
        /*** wait for clients ***/
        sem_wait(&queue);

        /*** read and update n_t_c ***/
        pthread_mutex_lock(&mutex_ntc);
        ntc = num_to_call;
        num_to_call++;
        pthread_mutex_unlock(&mutex_ntc);

        /*** call the client ***/
        sem_post(&sem_client[ntc]);
        serving_teller[ntc] = num;

        /*** sleep until serve end ***/
        usleep(TIME_UNIT*serve_time[ntc]);

        /*** read and update n_t_s ***/
        pthread_mutex_lock(&mutex_nts);
        nts = num_to_serve;
        num_to_serve--;
        pthread_mutex_unlock(&mutex_nts);
    }

    /*** thread exit ***/
    pthread_exit(NULL);
    return NULL;
}

/// get the number of clients
int get_client_num(char *filename)
{
    /*** define variables ***/
    FILE *fp;
    int word_num, client_num;
    char cmd[80] = "wc -w ";
    
    /*** shell cmd: wc -w filename ***/
    if((fp = popen(strcat(cmd,filename), "r")) == NULL)
    {
        printf("error: commmand failed.\n");
        exit(EXIT_FAILURE);
    }

    /*** get return value ***/
    fscanf(fp, "%d", &word_num);
    client_num = word_num/3;
    pclose(fp);

    return client_num;
}
