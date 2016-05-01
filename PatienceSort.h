#ifndef PATIENCESORT_H
#define PATIENCESORT_H

#include <vector>
#include <list>
#include <array>
#include <algorithm>

#include "RunPool.h"


const float kMaxSortedness =    0.35f;
const float kRunPoolSize =      1.5f;
const float kBlockPoolFactor =  15.0f;


struct RunInfo {
    size_t array_index, elem_index, run_size;
public:
    RunInfo() { }

    RunInfo(size_t arr_index, size_t el_index, size_t run_count) {
        array_index = arr_index;
        elem_index = el_index;
        run_size = run_count;
    }
};

template <class RAI>
class PatienceSorting {
public:

    typedef typename RAI::value_type        ValueType;
    typedef std::vector<ValueType>          ValueVector;
    typedef std::list<RunInfo>              RunInfoList;


    void Sort(RAI begin, RAI end) {
        std::vector<RunPool<ValueType>*> runs;
        GenerateRuns(begin, end, runs);
        Merge(begin, runs);

        // release pool memory of runs and runblocks
        RunPool<ValueType>::Release();
        Release();
    }


private:
    std::vector<ValueType> lasts_;
    std::vector<ValueType> heads_;
    long num_elements_ = 0;
    size_t num_runs_ = 0;
    float sortedness_ = 0.0f;
    int values_not_in_order_ = 0;

    static RunPool<ValueType>* memory_;
    static RunPool<ValueType>* next_free_;
    static size_t run_blocks_;

    void GenerateRuns(RAI begin, RAI end, std::vector<RunPool<ValueType>*>& runs) {
        runs.clear();
        num_elements_ = std::distance(begin, end);
        num_runs_ = static_cast<size_t>(sqrt(num_elements_));

        // calculate the degree of disorder
        values_not_in_order_ = GetOrderLevel(begin, end);
        sortedness_ = values_not_in_order_ / static_cast<float>(num_elements_);

        // fallback to std::sort if input data is too random
        if(sortedness_ > kMaxSortedness) {
            std::sort(begin, end);
            return;
        }

        // Pool for runs
        SetMemSize(num_runs_ * kRunPoolSize);
        Init();

        // Pool for runblocks
        RunPool<ValueType>::SetMemSize(GetMemPoolSize(num_elements_, num_runs_));
        RunPool<ValueType>::Init();


        runs.reserve(num_runs_);
        lasts_.reserve(num_elements_);
        heads_.reserve(num_elements_);

        for (auto it = begin; it != end; ++it) {

            // binary search the right run to insert the current element
            ValueType value = *it;
            auto key = std::lower_bound(lasts_.begin(), lasts_.end(), value, [](ValueType& v1, ValueType v2) { return v1 > v2;});

            if (key != lasts_.end()) {      // if suitable run is found, append
                size_t i = std::distance(lasts_.begin(), key);
                runs[i]->Add(value);
                lasts_[i] = value;

                // if we add to the first run, try to add as many elements as possible to avoid expensive binary search
                if (i == 0) {
                    auto next_value = std::next(it, 1);
                    while (next_value != end
                           && *next_value > lasts_[i]) {
                        it++;
                        runs[0]->Add(*it);
                        lasts_[0] = *it;
                    }
                }
            }
            else {      // no suitable run found, so we try to add the element to the begin of a run

                // binary search the beginnings to find a suitable run
                key = std::lower_bound(heads_.begin(), heads_.end(), value);
                if (key == heads_.end()) {      // no suitable run found, create a new run and add it to sorted runs vector

                    RunPool<ValueType>* run = Alloc();
                    new(run) RunPool<ValueType>();      // necessary, otherwise the objects are not initialized -> memory error
                    runs.push_back(run);

                    runs.back()->Add(value);
                    lasts_.push_back(value);
                    heads_.push_back(value);
                } else {
                    // suitable run found, so append to its beginning.
                    size_t i = std::distance(heads_.begin(), key);
                    runs[i]->AddFront(value);
                    heads_[i] = value;
                }
            }
        }
    }

    void Merge(RAI begin, std::vector<RunPool<ValueType>*>& runs) {
        // if no runs exist, input is probably empty, so exit here
        if(runs.size() == 0) {
            return;
        }

        // if only 1 run exists, this means the input data is in ascending order or in reversed, but
        // by adding to the front of a run it is automatically reversed
        if (runs.size() < 2) {
            // copy content to target array
            auto end = runs[0]->last();
            auto first_copy = begin;
            for(auto it = runs[0]->begin(); it != end; ++it) {
                *first_copy = *it;
                first_copy++;
            }
            *first_copy = runs[0]->back();
            return;
        }


        std::sort(runs.begin(), runs.end(), [](const RunPool<ValueType>* a, const RunPool<ValueType>* b) { return
                a->size() <
                b->size(); });
        ValueVector elems1;
        ValueVector elems2(num_elements_);
        elems1.reserve(num_elements_);
        RunInfoList run_infos;
        size_t next_empty_arr_loc = 0;
        std::array<ValueVector *, 2> arrs;
        arrs[0] = &elems1;
        arrs[1] = &elems2;

        // copy sorted runs consecutively to the first ping-pong array and save the information
        // in which array it is, at which index it starts and its size to an additional list element
        for (size_t i = 0; i < runs.size(); i++) {

            size_t temp_index = next_empty_arr_loc;
            auto it = runs[i]->begin();
            auto end = runs[i]->last();
            for(; it != end; ++it) {
                elems1.push_back(*it);
                temp_index++;
            }
            elems1.push_back(*it);

            RunInfo run_info;
            run_info.array_index = 0;
            run_info.elem_index = next_empty_arr_loc;
            run_info.run_size = runs[i]->size();
            run_infos.push_back(run_info);
            next_empty_arr_loc += runs[i]->size();
        }

        auto cur_run = run_infos.begin();
        const auto beginRun = cur_run;

        // Ping-pong merge until only 2 runs are left
        while (run_infos.size() > 2) {
            auto next_run = std::next(cur_run, 1);
            if (cur_run == run_infos.end() || std::next(cur_run, 1) == run_infos.end() ||
                (cur_run->run_size + next_run->run_size) >
                (beginRun->run_size + (std::next(beginRun, 1)->run_size))) {
                cur_run = run_infos.begin();
            }
            next_run = std::next(cur_run, 1);
            if (cur_run->array_index == 0) {          // blindly merge curRun and curRuns next into Elems2
                BlindMerge(arrs, elems2, cur_run, 1);
            } else {
                BlindMerge(arrs, elems1, cur_run, 0);
            }
            cur_run->run_size += next_run->run_size;
            run_infos.erase(next_run);
            cur_run++;
        }

        // merge the last 2 runs directly to the output
        cur_run = run_infos.begin();
        BlindMerge(arrs, begin, cur_run);
    }

    // Merge 2 sorted runs into a ping-pong array
    void BlindMerge(std::array<ValueVector *, 2> &arrs, ValueVector &write,
                    typename std::list<RunInfo>::iterator cur_run, size_t arr_index) {
        auto next_run = std::next(cur_run, 1);
        size_t one = cur_run->elem_index,
                two = next_run->elem_index,
                k = cur_run->elem_index;
        ValueVector &curr_arr = *arrs[cur_run->array_index];
        ValueVector &next_arr = *arrs[next_run->array_index];

        while ((one - cur_run->elem_index) < cur_run->run_size &&
               (two - next_run->elem_index) < next_run->run_size) {

            if (curr_arr[one] < next_arr[two]) {
                write[k] = curr_arr[one];
                one++;
            } else {
                write[k] = next_arr[two];
                two++;
            }
            k++;
        }
        // if both runs don't have the same size, copy the remaining elements over
        if ((one - cur_run->elem_index) < cur_run->run_size) {
            for (size_t i = (one - cur_run->elem_index); i < cur_run->run_size; i++) {
                write[k] = curr_arr[i + cur_run->elem_index];
                k++;
            }
        } else {
            for (size_t i = (two - next_run->elem_index); i < next_run->run_size; i++) {
                write[k] = next_arr[i + next_run->elem_index];
                k++;
            }
        }
        cur_run->array_index = arr_index;
    }

    // Merge the last 2 runs directly to the output
    void BlindMerge(std::array<ValueVector *, 2> &arrs, RAI begin,
                    typename std::list<RunInfo>::iterator cur_run) {
        auto next_run = std::next(cur_run, 1);
        size_t one = cur_run->elem_index,
                two = next_run->elem_index;
        ValueVector &curr_arr = *arrs[cur_run->array_index];
        ValueVector &next_arr = *arrs[next_run->array_index];

        while ((one - cur_run->elem_index) < cur_run->run_size &&
               (two - next_run->elem_index) < next_run->run_size) {

            if (curr_arr[one] < next_arr[two]) {
                *begin = curr_arr[one];
                one++;
            } else {
                *begin = next_arr[two];
                two++;
            }
            begin++;
        }
        if ((one - cur_run->elem_index) < cur_run->run_size) {
            for (size_t i = (one - cur_run->elem_index); i < cur_run->run_size; i++) {
                *begin = curr_arr[i + cur_run->elem_index];
                begin++;
            }
        } else {
            for (size_t i = (two - next_run->elem_index); i < next_run->run_size; i++) {
                *begin = next_arr[i + next_run->elem_index];
                begin++;
            }
        }
    }

    static void SetMemSize(size_t s) {
        run_blocks_ = s;
    }

    static void Init() {
        memory_ = new RunPool<ValueType>[run_blocks_];
        next_free_ = memory_;
    }

    static void Release() {
        delete[] memory_;
        memory_ = nullptr;
    }

    // Fetch a new memory block from the pool
    static RunPool<ValueType>* Alloc() {
        RunPool<ValueType>* ret = next_free_;
        next_free_++;
        return ret;
    }

    // Counts how many elements in the input sequence are not in ascending order
    int GetOrderLevel(RAI first, RAI last) {
        int k = 0;
        last -= 1;
        for(RAI i = first; i != last; ++i) {
            k += (*i > *(i+1));
        }
        return k;
    }

    size_t GetMemPoolSize(const size_t num_elements, const size_t num_runs) {

        size_t x = num_elements;
        while(x >= 100) {
            x /= 100;
        }
        size_t y = num_elements / x;

        return num_runs * sqrt(kBlockPoolFactor * (num_elements / y));
    }
};

template <class RAI>
RunPool<typename RAI::value_type>* PatienceSorting<RAI>::memory_ = nullptr;

template <class RAI>
RunPool<typename RAI::value_type>* PatienceSorting<RAI>::next_free_ = nullptr;

template <class RAI>
size_t PatienceSorting<RAI>::run_blocks_;


// simple function that applies patience sorting in the same style as std::sort
template <class RandomAccessIterator>
void PatienceSortFunc(RandomAccessIterator begin, RandomAccessIterator end) {
    PatienceSorting<RandomAccessIterator>  ps;
    ps.Sort(begin, end);
}

#endif
