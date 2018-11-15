#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#define STOP_NUM 1000           // size of single thread array
#define RAND_NUM 1000000        // num of random number
#define NAME "/quick_sort_shm"  // name of shared memory

/*** function declarations ***/
void swap(int *a, int *b);
void sort_asc(int *a, int *b, int *c);
int partition(int *arr, int left, int right);
void quick_sort(int *arr, int left, int right);
void *quick_sort_multhread(void* arg);

int main()
{
    /*** define variables ***/
    FILE *fp;
    int fd, cnt, thread_arg[2], *arr;
    pthread_t main_thread;
    
    /*** create shared memory ***/
    if((fd = shm_open(NAME, O_RDWR|O_CREAT, 0666)) == -1)
    {
        printf("error: share memory cannot be opened.\n");
        printf("errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if(ftruncate(fd, sizeof(int)*RAND_NUM) < 0)
    {
        printf("error: share memory cannot be malloced.\n");
        printf("errno: %d\n", errno);
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*** map arr to shared memory ***/
    arr = (int *)mmap(NULL, sizeof(int)*RAND_NUM, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(arr == MAP_FAILED)
    {
        printf("error: share memory cannot be mapped.\n");
        printf("errno: %d\n", errno);
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*** read from file ***/
    if((fp = fopen("test.in", "r")) == NULL)
    {
        printf("error: file \"test.in\" cannot be opened.\n");
        printf("errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    for(cnt=0; cnt<RAND_NUM; cnt++)
    {
        fscanf(fp, "%d", arr+cnt);
    }
    fclose(fp);

    /*** launch main thread ***/
    thread_arg[0] = 0;
    thread_arg[1] = RAND_NUM-1;
    pthread_create(&main_thread, NULL, quick_sort_multhread, (void *)thread_arg);
    pthread_join(main_thread, NULL);

    /*** write to file ***/
    if((fp = fopen("test.out", "w")) == NULL)
    {
        printf("error: file \"test.out\" cannot be opened.\n");
        printf("errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    for(cnt=0; cnt<RAND_NUM; cnt++)
    {
        fprintf(fp, "%d ", arr[cnt]);
    }
    fclose(fp);

    /*** unmap and remove shared memory ***/
    munmap((void *)arr, sizeof(int)*RAND_NUM);
    shm_unlink(NAME);

    exit(EXIT_SUCCESS);
}

/// swap the value of a and b
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
    return;
}

/// sort *a, *b, *c in ascending order. i.e. *a<=*b<=*c
void sort_asc(int *a, int *b, int *c)
{
    if(*a > *b) swap(a,b);
    if(*b > *c) swap(b,c);
    if(*a > *b) swap(a,b);
    return;
}

/// partly sort and return partition index
int partition(int *arr, int left, int right)
{
    int end = right;
    sort_asc(&arr[left], &arr[end], &arr[(left+right)/2]);

    while(left < right)
    {
        while(left<right && arr[left]<=arr[end])
        {
            left++;
        }
        while(left<right && arr[right]>=arr[end])
        {
            right--;
        }
        swap(&arr[left], &arr[right]);
    }

    swap(&arr[left], &arr[end]);
    return left;
}

/// quick_sort recursively
void quick_sort(int *arr, int left, int right)
{
    int index;
    if(left < right)
    {
        index = partition(arr, left, right);
        quick_sort(arr, left, index-1);
        quick_sort(arr, index+1, right);
    }
    return;
}

/// multi-threaded quick_sort
void *quick_sort_multhread(void* arg)
{
    /*** define variables ***/
    int left, right;
    int fd, index, left_arg[2], right_arg[2], *arr;
    pthread_t left_thread, right_thread;

    /*** get arguement ***/
    left = ((int *)arg)[0];
    right = ((int *)arg)[1];

    /*** open shared memory ***/
    if((fd = shm_open(NAME, O_RDWR, 0666)) == -1)
    {
        if(errno == 24)
        {
            printf("error: too many open files\n");
            printf("please use \"ulimit -n 4096\" to change the max file open num temporarily\n");
        }
        else
        {
            printf("error: share memory cannot be opened.\n");
            printf("errno: %d\n", errno);
        }
        exit(EXIT_FAILURE);
    }

    /*** map arr to shared memory ***/
    arr = (int *)mmap(NULL, sizeof(int)*RAND_NUM, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(arr == MAP_FAILED)
    {
        printf("error: share memory cannot be mapped.\n");
        printf("errno: %d\n", errno);
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*** partition and quick_sort recursively ***/
    if(right-left < STOP_NUM)
    {
        quick_sort(arr, left, right);
    }
    else
    {
        index = partition(arr, left, right);

        left_arg[0] = left;
        left_arg[1] = index-1;
        pthread_create(&left_thread, NULL, quick_sort_multhread, (void *)left_arg);

        right_arg[0] = index+1;
        right_arg[1] = right; 
        pthread_create(&right_thread, NULL, quick_sort_multhread, (void *)right_arg);
        
        pthread_join(left_thread, NULL);
        pthread_join(right_thread, NULL);
    }

    /*** unmap shared memory ***/
    munmap((void *)arr, sizeof(int)*RAND_NUM);

    /*** thread exit ***/
    pthread_exit(NULL);
    return NULL;
}
