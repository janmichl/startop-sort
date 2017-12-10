/**
    @file
    @author Jan Michalczyk 

    @brief
*/

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <algorithm>
#include <limits>

#include "utilities.h"

#define K 10
#define N 100

// allocate K * N resulting sorted vector
std::vector<int> g_sorted_vector(K * N);

// global master node
class MasterNode
{
    public:
        MasterNode() : running_nodes_(K), pushed_elements_(0)
        {
            buffer_.resize(K);
        }


        std::vector<int> buffer_;
        std::size_t      thread_index_;
        std::size_t      running_nodes_;
        std::size_t      pushed_elements_;
};
MasterNode g_master_node;

// global mutexes
std::mutex g_lock_master;
std::mutex g_lock_nodes_counter;
std::mutex g_lock_k;
std::mutex g_lock_sorted;

// loop index
std::size_t g_loop_index = 0;

// threads done flag
std::size_t g_threads_done = 0;

// barriers
utilities::Barrier writing_barrier(K + 1);
utilities::Barrier reading_barrier(K);

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
            while(!vector_.empty())
            {
                g_lock_master.lock();
                g_master_node.buffer_[thread_id_] = vector_.front(); 
                g_master_node.pushed_elements_++;
                g_lock_master.unlock();
                
                writing_barrier.wait();
                
                if(thread_id_ == g_master_node.thread_index_)
                {
                    vector_.erase(vector_.begin());
                }

                bool thread_mask = ((thread_id_ * N + 1 <= g_loop_index) && ( g_loop_index <= (thread_id_ + 1) * N));
                if(thread_mask)
                {
                    g_lock_master.lock();
                    vector_sorted_.push_back(g_master_node.buffer_.front());
                    g_lock_master.unlock();
                }
                
                reading_barrier.wait();
                
                g_lock_sorted.lock();
                if((vector_sorted_.size() == 100) && thread_mask)
                {
                    std::cout << "thread " << thread_id_ << std::endl;
                    utilities::printVector(std::cout, vector_sorted_);
                    
                    g_threads_done++;
                    if(g_threads_done == K)
                    {
                        std::exit(0);
                    }
                }
                g_lock_sorted.unlock();
            }

            g_lock_nodes_counter.lock();
            g_master_node.running_nodes_--;
            g_lock_nodes_counter.unlock();
                
            g_lock_master.lock();
            g_master_node.buffer_[thread_id_] = std::numeric_limits<int>::max(); 
            g_lock_master.unlock();
            
            while(g_master_node.running_nodes_)
            {
                writing_barrier.wait();
                reading_barrier.wait();
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
            while(g_loop_index != K * N)
            {
                if(g_master_node.running_nodes_ == g_master_node.pushed_elements_)
                {
                    g_lock_master.lock();
                    auto min = std::min_element(g_master_node.buffer_.begin(), g_master_node.buffer_.end());
                    g_master_node.thread_index_ = min - g_master_node.buffer_.begin();
                    g_master_node.buffer_.front() = *min;
                    g_master_node.pushed_elements_ = 0;
                    g_lock_master.unlock();
                    std::unique_lock<std::mutex> lock1(g_lock_k);
                    g_loop_index++;
                    writing_barrier.wait();
                }
            }
        }
};

// main
int main(int argc, char** argv)
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
        threads[i].join(); 
    }
    
    return(0);
}
