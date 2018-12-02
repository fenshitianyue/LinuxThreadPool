#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include"ThreadPool.h"

void *route( void *arg)
{
	int id = *(int *)arg;
	free(arg);
	printf("%#X thread ruing %d\n",pthread_self(),id);
	sleep(1);
}


int main(void)
{
	threadpool_t pool;
	threadpool_init(&pool, 3);
	int i =0;
	for(i = 0;i < 10; i++)
	{
		int *p = malloc(sizeof(int));
		*p = i;
		threadpool_add(&pool, route, p);
	}
	threadpool_destroy(&pool);
}
