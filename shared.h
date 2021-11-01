#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <type_traits>
#include <algorithm>

class EnableSharedFromThisBase {};

template <typename T>
class EnableSharedFromThis;

class ControlBlockBase {
public:
    int strong_cnt, weak_cnt;
    virtual ~ControlBlockBase() = default;
    virtual void DestructObject() {
    }
};
template <class U>
class ControlBlockPointer : public ControlBlockBase {
public:
    U* ptr_;
    ControlBlockPointer() {
        ptr_ = nullptr;
        strong_cnt = 0;
        weak_cnt = 0;
    }
    ControlBlockPointer(U* ptr) {
        ptr_ = ptr;
        strong_cnt = 1;
        weak_cnt = 0;
    }
    void DestructObject() override {
        delete ptr_;
        ptr_ = nullptr;
    }
    ~ControlBlockPointer() override {
        DestructObject();
    }
};
template <class U>
class ControlBlockStorage : public ControlBlockBase {
private:
    U* ptr_;

public:
    alignas(U) char storage_[sizeof(U)];
    template <class... Args>
    ControlBlockStorage(Args&&... args) {
        ptr_ = new (storage_) U(std::forward<Args>(args)...);
        strong_cnt = 1;
        weak_cnt = 0;
    }
    U* GetPtr() {
        return ptr_;
    }
    void DestructObject() override {
        if (ptr_ != nullptr) {
            ptr_->~U();
        }
        ptr_ = nullptr;
    }
    ~ControlBlockStorage() override {
        DestructObject();
    }
};
// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
private:
    T* ptr_;
    ControlBlockBase* block_;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    SharedPtr(T* ptr, ControlBlockBase* base) {
        block_ = base;
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr->self_ = *this;
        }
    }
    SharedPtr() : ptr_(nullptr), block_(nullptr) {
    }
    SharedPtr(std::nullptr_t) : ptr_(nullptr), block_(nullptr) {
    }
    template <class U>
    explicit SharedPtr(U* ptr) : ptr_(ptr), block_(new ControlBlockPointer<U>(ptr)) {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr->self_ = *this;
        }
    }
    template <class U>
    SharedPtr(const SharedPtr<U>& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++(block_->strong_cnt);
        }
    }

    SharedPtr(const SharedPtr& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++(block_->strong_cnt);
        }
    }
    template <class U>
    SharedPtr(SharedPtr<U>&& other) : ptr_(other.ptr_), block_(other.block_) {
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    }
    SharedPtr(SharedPtr&& other) : ptr_(other.ptr_), block_(other.block_) {
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        ptr_ = ptr;
        block_ = other.block_;
        if (block_) {
            ++(block_->strong_cnt);
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++block_->strong_cnt;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (&other == this) {
            return *this;
        }
        if (block_ != nullptr) {
            --(block_->strong_cnt);
        }
        if (UseCount() == 0) {
            delete block_;
        }
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            ++(block_->strong_cnt);
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (&other == this) {
            return *this;
        }
        if (block_ != nullptr) {
            --(block_->strong_cnt);
        }
        if (UseCount() == 0) {
            delete block_;
        }
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        other.block_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_ != nullptr) {
            if (UseCount() == 1) {
                block_->DestructObject();
                block_->strong_cnt = 0;
                if (block_->weak_cnt == 0) {
                    delete block_;
                }
            } else {
                --(block_->strong_cnt);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (UseCount() == 1) {
            block_->DestructObject();
            block_->strong_cnt = 0;
            if (block_->weak_cnt == 0) {
                delete block_;
            }
        } else if (block_) {
            --(block_->strong_cnt);
        }
        block_ = nullptr;
        ptr_ = nullptr;
    }
    template <class U>
    void Reset(U* ptr) {
        if (UseCount() == 1) {
            block_->DestructObject();
            block_->strong_cnt = 0;
            if (block_->weak_cnt == 0) {
                delete block_;
            }
        } else if (block_) {
            --(block_->strong_cnt);
        }
        block_ = new ControlBlockPointer<U>(ptr);
        ptr_ = ptr;
    }
    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return (block_->strong_cnt >= 0 ? block_->strong_cnt : 0);
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }

    template <class U>
    friend class SharedPtr;
    template <class U>
    friend class WeakPtr;
    template <typename U, typename... Args>
    friend SharedPtr MakeShared(Args&&... args);
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto block = new ControlBlockStorage<T>(std::forward<Args>(args)...);
    SharedPtr<T> res(block->GetPtr(), block);
    return res;
}

// Look for usage examples in tests

template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<T> SharedFromThis() {
        return self_.Lock();
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(self_.Lock());
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return self_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(self_);
    }
    WeakPtr<T> self_;
};
