#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>

struct thread_info threadinfo;
int thread_running = 0;

void init_thread_pool(int thread_num){
  if(thread_num <= 0)
    thread_num = 5;
  //初始化锁
  pthread_mutex_init(&threadinfo._mutex, NULL);
  pthread_cond_init(&threadinfo._cond, NULL);
  
  threadinfo._thread_num = thread_num; //线程池当前线程数
  threadinfo._thread_running = 1; //线程池正在工作的线程数
  threadinfo._task_num = 0;
  threadinfo._tasks = NULL;

  thread_running = 1;
  
  threadinfo._thread_id = (pthread_t*)malloc(sizeof(pthread_t) * thread_num);

  //创建 thread_num 个线程
  int i;
  for(i = 0; i < thread_num; ++i){
    pthread_create(&threadinfo._thread_id[i], NULL, thread_routine, NULL);
  }
}

void destroy_thread_pool(){
  threadinfo._thread_running = 0;
  thread_running = 0;
  //等待线程
  for(int i = 0; i < threadinfo._thread_num; ++i){
    pthread_join(threadinfo._thread_id[i], NULL);
  }
  free(threadinfo._thread_id);

  //销毁锁
  pthread_mutex_destroy(&threadinfo._mutex);
  pthread_cond_destroy(&threadinfo._cond);
}

void thread_pool_add_task(struct task* t){
  if(NULL == t)
    return;
  //加锁
  pthread_mutex_lock(&threadinfo._mutex);
  struct task* head = threadinfo._tasks;
  if(NULL == head){
    threadinfo._tasks = t;
  }else{
    //遍历任务链表，将任务添加到链表尾部
    while(head->_pNext){
      head = head->_pNext;
    }
    //此时head->_pNext = NULL, 将任务t的地址赋值给它
    head->_pNext = t;
  }
  ++threadinfo._thread_num;
  //向任务池中添加任务后，使用signal通知wait函数
  pthread_cond_signal(&threadinfo._cond);
  //解锁
  pthread_mutex_unlock(&threadinfo._mutex);
}

struct task* thread_pool_retrieve_task(){
  struct task* head = threadinfo._tasks;
  //从任务链表中取出任务并返回
  if(head != NULL){
    threadinfo._tasks = head->_pNext;
    --threadinfo._task_num;
    printf("retrieve a task, task value is [%d]\n", head->_value);
    return head;
  } 
  printf("no task\n");
  return NULL;
}

//执行任务池中指定的任务
void thread_pool_do_task(struct task* t){
  if(NULL == t)
    return;
  //TODO:DO SOMETHING
  printf("task value is [%d]\n", t->_value);
  //TODO:如果t需要处理善后工作，在这里做
}

//线程函数
void* thread_routine(void* thread_param){
  printf("thread NO.%ld start.\n", pthread_self());
  while(thread_running){ //thread_running == threadinfo._thread_running -> true
    struct task* current = NULL;
    //加锁
    pthread_mutex_lock(&threadinfo._mutex);
    while(threadinfo._task_num <= 0){
      pthread_cond_wait(&threadinfo._cond, &threadinfo._mutex);
      if(!threadinfo._thread_running)
        break;
      current = thread_pool_retrieve_task();
      if(current != NULL) 
        break;
    }
    //解锁
    pthread_mutex_unlock(&threadinfo._mutex);
    thread_pool_do_task(current);
  } 
  printf("thread NO.%ld exit.\n", pthread_self());
}


