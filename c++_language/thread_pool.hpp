#pragma once
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <thread>

class ThreadPool{
public:
  ThreadPool(size_t threads);
  ~ThreadPool();
private:
  std::vector<std::thread> workers;
  //任务队列
  std::queue<std::function<void()>> tasks;
  //同步相关
  std::mutex queue_mutex; //互斥锁
  std::condition_variable condition; //互斥条件变量
  //停止相关
  bool stop;
};
