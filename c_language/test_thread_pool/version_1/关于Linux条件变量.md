# 关于Linux条件变量的几个难点



### 一、必须要结合一个互斥体一起使用。使用流程如下：

```c
pthread_mutex_lock(&threadinfo.mutex)           
pthread_cond_wait(&threadinfo.cond, &threadinfo.mutex);
pthread_mutex_unlock(&threadinfo.mutex);
```

上面的代码，我们分为一二三步，当条件不满足是pthread_cond_wait会挂起线程，但是不知道你有没有注意到，如果在第二步挂起线程的话，第一步的mutex已经被上锁，谁来解锁？mutex的使用原则是谁上锁谁解锁，所以不可能在其他线程来给这个mutex解锁，但是这个线程已经挂起了，这就死锁了。所以pthread_cond_wait在挂起之前，额外做的一个事情就是给绑定的mutex解锁。反过来，如果条件满足，pthread_cond_wait不挂起线程，pthread_cond_wait将什么也不做，这样就接着走pthread_mutex_unlock解锁的流程。而在这个加锁和解锁之间的代码就是我们操作受保护资源的地方。

### 二、不知道你有没有注意到pthread_cond_wait是放在一个while循环里面的：

```c
        pthread_mutex_lock(&threadinfo.mutex);

        while (threadinfo.tasknum <= 0)
        {
            //如果获得了互斥锁，但是条件不合适的话，wait会释放锁，不往下执行。
            //当变化后，条件合适，将直接获得锁。
            pthread_cond_wait(&threadinfo.cond, &threadinfo.mutex);

            if (!threadinfo.thread_running)
                break;

            current = thread_pool_retrieve_task();

            if (current != NULL)
                break;
        }

        pthread_mutex_unlock(&g_threadinfo.mutex);
```

注意，我说的是内层的while循环，不是外层的。pthread_cond_wait一定要放在一个while循环里面吗？一定要的。这里有一个非常重要的关于条件变量的基础知识，叫条件变量的虚假唤醒（spurious wakeup），那啥叫条件变量的虚假唤醒呢？假设pthread_cond_wait不放在这个while循环里面，正常情况下，pthread_cond_wait因为条件不满足，挂起线程。然后，外部条件满足以后，调用pthread_cond_signal或pthread_cond_broadcast来唤醒挂起的线程。这没啥问题。但是条件变量可能在某些情况下也被唤醒，这个时候pthread_cond_wait处继续往下执行，但是这个时候，条件并不满足（比如任务队列中仍然为空）。这种唤醒我们叫“虚假唤醒”。为了避免虚假唤醒时，做无意义的动作，我们将pthread_cond_wait放到while循环条件中，这样即使被虚假唤醒了，由于while条件（比如任务队列是否为空，资源数量是否大于0）仍然为true，导致线程进行继续挂起。