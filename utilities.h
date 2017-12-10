/**
    @file
    @author Jan Michalczyk 

    @brief
*/

#pragma once

namespace utilities
{
    template<typename t>
      inline void printVector(std::ostream& output_stream, const std::vector<t>& vector)
    {
        for(std::size_t i = 0; i < vector.size() - 1; ++i)
        {
            output_stream << vector[i] << " ";
        }
        output_stream << vector[vector.size() - 1] << std::endl;
    }
    
    template<typename t>
      inline void printVector(std::ostream& output_stream, const std::vector<std::vector<t>>& vector)
    {
        for(std::size_t i = 0; i < vector.size(); ++i)
        {
            for(std::size_t j = 0; j < vector[i].size() - 1; ++j)
            {
                output_stream << vector[i][j] << " ";
            }
            output_stream << vector[i][vector[i].size() - 1] << std::endl;
        }
    }

    template<typename t>
      inline std::vector<t> generateShuffledVector(std::size_t n_of_items)
    {
        std::vector<t> v(n_of_items);
        for(std::size_t i = 0; i < v.size(); ++i)
        {
            v[i] = i;
        }

        std::random_shuffle(v.begin(), v.end());
        return(v);
    }
    
    inline std::vector<int> generateRandomVector(std::size_t n_of_items, std::size_t seed_init)
    {
        std::srand(seed_init + 3);
        
        std::vector<int> v(n_of_items);
        for(std::size_t i = 0; i < v.size(); ++i)
        {
            v[i] = std::rand() % v.size() + 1;
        }

        return(v);
    }

    class Barrier
    {
        public:
            explicit Barrier(std::size_t count) : threshold_(count), count_(count), generation_(0)
            {
            }


            void wait()
            {
                std::unique_lock<std::mutex> lock{mutex_};
                auto gen = generation_;
                if(!--count_)
                {
                    generation_++;
                    count_ = threshold_;
                    condvar_.notify_all();
                }
                else
                {
                    condvar_.wait(lock, [this, gen] { return gen != generation_; });
                }
            }


        private:
            std::mutex              mutex_;
            std::condition_variable condvar_;
            std::size_t             threshold_;
            std::size_t             count_;
            std::size_t             generation_;
    };
}//utilities
