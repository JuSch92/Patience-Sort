#ifndef RUNPOOL_H
#define RUNPOOL_H

#include <bits/stl_iterator_base_types.h>


const size_t kValuesPerBlock =    800;

template <typename ValueType>
struct RunBlock {
    RunBlock *next;
    RunBlock *prev;
    int next_free_pos_;
    ValueType values[kValuesPerBlock];
    bool is_front;
    RunBlock()
            : next(NULL), prev(NULL), next_free_pos_(0), is_front(false)
    {}


};


template <typename ValueType>
class RunPool {

    class iterator
    {
    public:
        typedef iterator            self_type;
        typedef ValueType           value_type;
        typedef ValueType&          reference;
        typedef ValueType*          pointer;
        typedef size_t              difference_type;
        typedef std::bidirectional_iterator_tag iterator_category;

        iterator(RunBlock<ValueType>* block, size_t index) : block_(block), index_(index) {}

        self_type&  operator++() {      // prefix
            index_++;
            if(index_ >= kValuesPerBlock) {
                index_ -= kValuesPerBlock;
                block_ = block_->next;
                return *this;
            }
            return *this;
        }
        self_type&  operator++(int i) {     // postfix
            self_type& temp = *this;
            index_++;
            if(index_ >= kValuesPerBlock) {
                index_ -= kValuesPerBlock;
                block_ = block_->next;
                return temp;
            }
            return temp;
        }
        self_type&  operator--() {
            index_--;
            if(index_ < 0) {
                index_ += kValuesPerBlock;
                block_ = block_->prev;
            }
            return *this;
        }
        self_type&  operator--(int i) {
            self_type& temp = *this;
            index_--;
            if(index_ < 0) {
                index_ += kValuesPerBlock;
                block_ = block_->prev;
            }
            return temp;
        }
        reference   operator*() {
            return block_->values[index_];
        }
        pointer     operator->() {
            return &block_->values[index_];
        }
        bool        operator==(const self_type& rhs) {
            return &block_->values[index_] == &rhs.block_[rhs.index_];
        }
        bool        operator!=(const self_type& rhs) {
            return &block_->values[index_] != &rhs.block_->values[rhs.index_];
        }

        RunBlock<ValueType>*    block_;
        size_t                  index_;
    };


public:
    RunPool() {
        begin_back_ = Alloc();
        end_back_ = begin_back_;
        size_ = 0;
        begin_front_ = end_front_ = NULL;
        end_block_ = begin_back_;
    }


    RunPool(const RunPool&) =               delete;
    RunPool& operator=(const RunPool&) =    delete;
    RunPool(RunPool&&) =                    default;
    RunPool& operator=(RunPool&&) =         default;


    void Add(ValueType &value) {
        if(end_back_->next_free_pos_ < kValuesPerBlock) {
            end_back_->values[end_back_->next_free_pos_] = value;
        } else {
            RunBlock<ValueType>* temp = Alloc();
            temp->values[0] = value;
            temp->prev = end_back_;
            end_back_->next = temp;
            end_back_ = temp;
            end_block_ = temp;
            size_ += kValuesPerBlock;
        }
        end_back_->next_free_pos_++;
    }


    void AddFront(ValueType &value) {
        if(begin_front_ == NULL) {
            begin_front_ = Alloc();
            begin_front_->next_free_pos_ = kValuesPerBlock - 1;
            begin_front_->is_front = true;
            begin_front_->next = begin_back_;

            end_front_ = begin_front_;
            begin_back_->prev = end_front_;
        }

        if(begin_front_->next_free_pos_ < 0) {
            RunBlock<ValueType>* temp = Alloc();
            temp->is_front = true;
            temp->next = begin_front_;
            temp->values[kValuesPerBlock - 1] = value;
            temp->next_free_pos_ = kValuesPerBlock - 2;

            begin_front_->prev = temp;
            begin_front_ = temp;
            size_ += kValuesPerBlock;

        } else {
            begin_front_->values[begin_front_->next_free_pos_] = value;
            begin_front_->next_free_pos_--;
        }
    }

    size_t  size() const {
        int size_total = size_;
        size_total += end_back_->next_free_pos_;

        if(begin_front_ != NULL) {
            size_total +=  kValuesPerBlock - begin_front_->next_free_pos_ - 1;
        }
        return size_total;
    }

    ValueType& operator[](size_t t) {
        RunBlock<ValueType>* temp = begin_back_;
        while(t >= kValuesPerBlock) {
            temp = temp->next;
            t -= kValuesPerBlock;
        }
        return temp->values[t];
    }

    iterator begin() {
        if(begin_front_ == NULL) {
            return iterator(begin_back_, 0);
        } else {
            return iterator(begin_front_, begin_front_->next_free_pos_ + 1);
        }

    }

    iterator end() {        // points to the last element plus one
        if(end_block_->next_free_pos_>= kValuesPerBlock) {
            return iterator(end_block_, kValuesPerBlock);
        }
        return iterator(end_block_, end_block_->next_free_pos_);

    }

    iterator last() {       // points to the last element
        if(end_block_->next_free_pos_ > 0) {
            return iterator(end_block_, end_block_->next_free_pos_ - 1);
        } else {
            return iterator(end_block_->prev, kValuesPerBlock - 1);
        }
    }

    ValueType&back() {
        int next_free = end_block_->next_free_pos_;
        if(next_free <= 0) {
            return end_block_->prev->values[kValuesPerBlock - 1];
        } else {
            return end_block_->values[next_free - 1];
        }
    }

    static void SetMemSize(size_t s) {
        mem_blocks_ = s;
    }

    static void Init() {
        memory_ = new RunBlock<ValueType>[mem_blocks_];
        next_free_ = memory_;
    }

    static void Release() {
        delete[] memory_;
    }

    static RunBlock<ValueType>* Alloc() {
        RunBlock<ValueType>* ret = next_free_;
        next_free_++;
        return ret;
    }




private:
    RunBlock<ValueType>* begin_back_;
    RunBlock<ValueType>* end_back_;
    RunBlock<ValueType>* begin_front_;
    RunBlock<ValueType>* end_front_;
    RunBlock<ValueType>* end_block_;
    size_t size_;

    static RunBlock<ValueType>* memory_;
    static RunBlock<ValueType>* next_free_;
    static size_t mem_blocks_;
};

template <typename ValueType>
RunBlock<ValueType>* RunPool<ValueType>::memory_ = NULL;

template <typename ValueType>
RunBlock<ValueType>* RunPool<ValueType>::next_free_ = NULL;

template <typename ValueType>
size_t RunPool<ValueType>::mem_blocks_;

#endif
