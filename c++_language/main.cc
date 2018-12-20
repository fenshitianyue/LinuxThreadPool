#include <iostream>
#include "thread_pool.hpp"


int main() {
  ThreadPool pool(4);  
  std::vector<std::future<std::string>> results;
  for(int i = 0; i < 8; ++i){
    results.emplace_back(
        pool.enqueue([i]{
              std::cout << "hello " << i << std::endl;
              //上一行输出后，该线程等待1秒
              std::this_thread::sleep_for(std::chrono::seconds(1));
              //然后继续输出并返回执行情况
              std::cout << "world " << i << std::endl;
              return std::string("---thread ") + std::to_string(i) + std::string(" finished---");
            }
        )//end enqueue
    );//end empace_back
  }//end for

  //输出线程执行结果
  for(auto && result : results){
    std::cout << result.get() << ' ';
  }
  std::cout << std::endl;
  return 0;
}
