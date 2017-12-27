/**
    @file
    @author Jan Michalczyk 

    @brief
*/

#include <iostream>
#include <cstdlib>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <algorithm>
#include <limits>
#include <atomic>

#include "utilities.h"

// number of workers
#define K 10 
// length of a worker
#define N 100

// global master node
template<typename t>
  class MasterNode
{
    public:
        MasterNode(std::size_t running_nodes) : running_nodes_(running_nodes), pushed_elements_(0)
        {
            buffer_.resize(running_nodes);
        }


        std::vector<t> buffer_;
        std::size_t    thread_index_;
        std::size_t    running_nodes_;
        std::size_t    pushed_elements_;
};
MasterNode<int> g_master_node(K);

// global mutexes
std::mutex g_lock_master;
std::mutex g_lock_loop_index;
std::mutex g_lock_sorted;

// loop index
std::size_t g_loop_index = 0;

// threads done flag
std::size_t g_threads_done = 0;

// barriers
utilities::Barrier g_writing_barrier(K + 1);
utilities::Barrier g_reading_barrier(K);

// is the computation ready
std::atomic<bool> is_ready(false);

// worker node thread functor
template<typename t>
  class workerThread
{
    public:
        workerThread(const std::vector<t>& vector, std::size_t thread_id) : thread_id_(thread_id),
                                                                            vector_(vector)
        {
        }


        void operator()()
        {
            std::sort(vector_.begin(), vector_.end());
            bool node_killed = true;
            while(true)
            {
                bool thread_mask = ((thread_id_ * N + 1 <= (g_loop_index + 1)) && ((g_loop_index + 1) <= (thread_id_ + 1) * N));
                if(!vector_.empty())
                {
                    std::unique_lock<std::mutex> lock(g_lock_master);
                    g_master_node.buffer_[thread_id_] = vector_.front(); 
                    g_master_node.pushed_elements_++;
                }
                else
                {
                    std::unique_lock<std::mutex> lock(g_lock_master);
                    g_master_node.buffer_[thread_id_] = std::numeric_limits<int>::max(); 
                    g_master_node.pushed_elements_++;
                }
                
                g_writing_barrier.wait();
                    
                if(!vector_.empty())
                {
                    if(thread_id_ == g_master_node.thread_index_)
                    {
                        vector_.erase(vector_.begin());
                    }
                }
                
                if((vector_sorted_.size() < N) && thread_mask)
                {
                    std::unique_lock<std::mutex> lock(g_lock_master);
                    vector_sorted_.push_back(g_master_node.buffer_.front());
                }
                
                g_reading_barrier.wait();
                
                if((vector_.empty()) && (vector_sorted_.size() == N))
                {
                    if(node_killed)
                    {
                        std::unique_lock<std::mutex> lock(g_lock_sorted);
                        std::cout << "-------" << std::endl;
                        std::cout << "worker thread " << thread_id_ << std::endl;
                        utilities::printVector(std::cout, vector_sorted_);
                        std::cout << "-------" << std::endl;
                        g_threads_done++;
                        node_killed = false;
                    }
                }

                if(g_threads_done == K)
                {
                    is_ready.store(true);
                }
            }
        }


    private:
        std::size_t    thread_id_;
        std::vector<t> vector_;
        std::vector<t> vector_sorted_;
};

// master node thread functor
class masterThread
{
    public:
        masterThread()
        {
        }


        void operator()()
        {
            while(g_threads_done != K)
            {
                if(K == g_master_node.pushed_elements_)
                {
                    g_lock_master.lock();
                    auto min = std::min_element(g_master_node.buffer_.begin(), g_master_node.buffer_.end());
                    g_master_node.thread_index_ = min - g_master_node.buffer_.begin();
                    g_master_node.buffer_.front() = *min;
                    g_master_node.pushed_elements_ = 0;
                    g_lock_master.unlock();
                    
                    g_lock_loop_index.lock();
                    g_loop_index++;
                    g_lock_loop_index.unlock();
                    
                    g_writing_barrier.wait();
                }
            }
        }
};

// main
int main(int argc, char** argv)
{
    try
    {
        // generate K * N  long vector to sort for worker nodes
        std::vector<int> workers = utilities::generateRandomVector(K * N, 1);
        
        // print unsorted array
        std::cout << "before sorting" << std::endl;
        utilities::printVector(std::cout, workers);

        // launch threads
        std::vector<std::thread> threads(K + 1);
        for(std::size_t i = 0; i < threads.size() - 1; ++i)
        {
            std::vector<int> subvector(&workers[i * N], &workers[((i + 1) * N)]);
            threads[i] = std::thread(workerThread<int>(subvector, i)); 
        }
        threads[threads.size() - 1] = std::thread(masterThread()); 

        for(std::size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].detach(); 
        }

        // wait for threads to finish
        while(!is_ready.load())
        {
            std::this_thread::yield();
        }
        
        std::cout << "threads done" << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "exception caught: " << e.what() << std::endl;
        std::exit(-1);
    }
    
    return(0);
}
