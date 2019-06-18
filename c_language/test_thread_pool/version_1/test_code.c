#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  init_thread_pool(5); 
  struct task* t = NULL;
  for(int i = 0; i < 100; ++i){
    t = (struct task*)malloc(sizeof(struct task));
    t->_value = i + 1;
    t->_pNext = NULL;
    printf("add task, value is [%d]\n", t->_value);
    thread_pool_add_task(t);
  }
  sleep(5);
  destroy_thread_pool();
  return 0;
}
