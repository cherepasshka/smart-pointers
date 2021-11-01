#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
private:
    T* ptr_;
    ControlBlockBase* block_;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() {
        ptr_ = nullptr;
        block_ = nullptr;
    }
    template <class U>
    WeakPtr(const WeakPtr<U>& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++block_->weak_cnt;
        }
    }
    WeakPtr(const WeakPtr& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++block_->weak_cnt;
        }
    }
    WeakPtr(WeakPtr&& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    template <class U>
    WeakPtr(const SharedPtr<U>& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++block_->weak_cnt;
        }
    }
    WeakPtr(const SharedPtr<T>& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++block_->weak_cnt;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }
        if (block_) {
            --block_->weak_cnt;
            if (Expired() && block_->weak_cnt <= 0) {
                delete block_;
            }
        }
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            ++block_->weak_cnt;
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        if (this == &other) {
            return *this;
        }
        if (block_) {
            --block_->weak_cnt;
            if (Expired() && block_->weak_cnt == 0) {
                delete block_;
            }
        }
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (block_) {
            if (Expired() && block_->weak_cnt == 1) {
                delete block_;
            } else {
                --block_->weak_cnt;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            --block_->weak_cnt;
            if (Expired() && block_->weak_cnt <= 0) {
                delete block_;
            }
        }
        ptr_ = nullptr;
        block_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->strong_cnt;
    }
    bool Expired() const {
        return UseCount() == 0;
    }
    SharedPtr<T> Lock() const {
        SharedPtr<T> res;
        if (Expired()) {
            return res;
        }
        delete res.block_;
        res.block_ = block_;
        res.ptr_ = ptr_;
        if (block_) {
            ++block_->strong_cnt;
        }
        return res;
    }

    template <class U>
    friend class SharedPtr;

    template <class U>
    friend class WeakPtr;
};
