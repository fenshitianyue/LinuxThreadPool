#pragma once
#include <string> //std::string
#include <queue> //std::queue
#include <vector> //std::vector
#include <memory> //std::make_shared
#include <future> //std::futrue | std::packged_task
#include <thread> //std::this_thread::sleep_for
#include <chrono> //std::chrono::seconds
#include <mutex> //std::mutex | std::unique_lock
#include <condition_variable>//std::condition_variable
#include <utility> //std::move
#include <stdexcept> //std::runtime_error
#include <functional> //std::function | std::bind
class ThreadPool{
public:
  ThreadPool(size_t threads); //创建 threads 个工作线程
  template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) ->std::future<typename std::result_of<F(Args...)>::type>;
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

inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
  //启动 threads 个工作线程
  for(size_t i = 0; i < threads; ++i){
    workers.emplace_back(
        [this]{
          while(true){
            std::function<void()> task;       
            //临界区
            {
              std::unique_lock<std::mutex> lock(this->queue_mutex); //创建互斥锁
              //阻塞当前线程，直到 condition_variable 被唤醒
              this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
              //如果当前线程池已经结束，并且等待任务队列为空，则返回
              if(this->stop && this->tasks.empty()){
                return;
              }
              //否则就让任务队列的队首任务作为需要执行的任务出队
              task = std::move(this->tasks.front());
              this->tasks.pop();
            }
            task();
          }//end while
        }//end lambda
    ); //end emplace_back
  }//end for
}

inline ThreadPool::~ThreadPool(){
  //临界区
  {
    //创建互斥锁
    std::unique_lock<std::mutex> lock(queue_mutex);
    //设置线程池的状态
    stop = true;
  }
  //通知所有等待线程
  condition.notify_all();
  //回收所有线程
  for(std::thread& worker : workers){
    worker.join();
  }
}

template<class F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
     ->std::future<typename std::result_of<F(Args...)>::type>
{
  using return_type = typename std::result_of<F(Args...)>::type; 
  //获取当前任务
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...)
  );
  
  std::future<return_type> res = task->get_future();
  //临界区
  {
    std::unique_lock<std::mutex> lock(queue_mutex);

    //禁止在线程池停止后添加新的线程
    if(stop){
      throw std::runtime_error("the ThreadPool has stopped running");
    }
    tasks.emplace([task]{ (*task)(); });
  }
  condition.notify_one();
  return res;
}
