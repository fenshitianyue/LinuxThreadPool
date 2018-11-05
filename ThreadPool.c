#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<errno.h>
#include"ThreadPool.h"

void threadpool_init(threadpool_t *pool, int max)
{
	condition_init(&pool->cond);
	pool->first = NULL;
	pool->tail  = NULL;
	pool->max_thread = max;
	pool->idle = 0;
	pool->counter = 0;
	pool->quit = 0;
}

void *myroute(void *arg)
{
	threadpool_t *pool = (threadpool_t *)arg;
	int timeout = 0;
	while(1)
	{
		condition_lock(&pool->cond);
		pool->idle ++;
		timeout = 0;
		while(pool->first == NULL)
		{
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME,&ts);
			ts.tv_sec += 2;
			int r = condition_timedwait(&pool->cond, &ts);
			if(r == ETIMEDOUT){
				timeout = 1;
			}
		}
		pool->idle--;
		if(pool->first != NULL){
			task_t *p = pool->first;
			pool->first = p->next;
			condition_unlock(&pool->cond);
			(p->pfun)(p->arg);
			condition_lock(&pool->cond);
			free(p);
		}
		if(pool->quit == 1 && pool->first == NULL){
			printf("%#lX thread destroy\n",pthread_self());
			pool->counter--;
			if(pool->counter == 0)
				condition_signal(&pool->cond);
			condition_unlock(&pool->cond);
			break;
		}
		if(pool->first == NULL && timeout == 1)
		{
			condition_unlock(&pool->cond);
			printf("%#lX thread timeout\n",pthread_self());
			break;
		}
		condition_unlock(&pool->cond);
	}
}

void threadpool_add(threadpool_t *pool, void*(*pf)(void *), void *arg)
{
	//创建任务节点
	task_t* new = (task_t*)malloc(sizeof(task_t));
	new->pfun = pf;
	new->arg = arg;
	new->next = NULL;
	
	//对任务池的操作要上锁
	if(pool->first == NULL)}{
		pool->first = new;
	}else{
		pool->tail->next = new;
	}
	pool->tail = new;
	if(pool->idle > 0){
		condition_signal(&pool->cond);
	}else if(pool->counter < pool->max_thread){
		pthread_t tid;
		pthread_create(&tid, NULL, myroute, (void*)pool);
		pool->counter ++;
	}
	condition_unlock(&pool->cond);
}


void threadpool_destroy(threadpool_t *pool)
{
	if(pool->quit) return;
	condition_lock(&pool->cond);
	if(pool->counter > 0){
		if(pool->idle > 0)
			condition_boardcast(&pool->cond);
	}
	while(pool->counter > 0)
	{
		condition_wait(&pool->cond);
	}
	condition_unlock(&pool->cond);
	condition_destroy(&pool->cond);
}



