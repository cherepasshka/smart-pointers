#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <iostream>
#include <memory>

struct Slug {};

// Primary template
template <typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
private:
    CompressedPair<T*, Deleter> pair_;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    explicit UniquePtr(T* ptr = nullptr) noexcept {
        pair_.GetFirst() = ptr;
    }
    UniquePtr(T* ptr, const Deleter& deleter) noexcept : pair_(ptr, deleter) {
    }
    template <class D>
    UniquePtr(T* ptr, D&& deleter) noexcept : pair_(ptr, std::forward<D>(deleter)) {
    }
    UniquePtr(UniquePtr&& other) noexcept {
        pair_.GetFirst() = other.Release();
        pair_.GetSecond() = std::move(other.GetDeleter());
    }

    UniquePtr(const UniquePtr& other) = delete;

    template <class U, class D>
    UniquePtr(UniquePtr<U, D>&& other) {
        pair_.GetFirst() = other.Release();
    }
    /////////////////////////////////tr///////////////////////////////////////////////////////////////
    // `operator=`-s
    UniquePtr& operator=(const UniquePtr& other) = delete;
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        GetDeleter()(pair_.GetFirst());
        pair_.GetFirst() = other.Release();
        pair_.GetSecond() = std::forward<Deleter>(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        GetDeleter()(pair_.GetFirst());
        pair_.GetFirst() = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* res = pair_.GetFirst();
        pair_.GetFirst() = nullptr;
        return res;
    }
    void Reset(T* ptr = nullptr) {
        T* old_ptr = pair_.GetFirst();
        pair_.GetFirst() = ptr;
        if (old_ptr) {
            GetDeleter()(old_ptr);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(other.pair_, pair_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pair_.GetFirst();
    }
    Deleter& GetDeleter() {
        return pair_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return pair_.GetSecond();
    }
    explicit operator bool() const {
        return pair_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        return *pair_.GetFirst();
    }
    T* operator->() const {
        return pair_.GetFirst();
    }
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
private:
    CompressedPair<T*, Deleter> pair_;

public:
    UniquePtr(T* ptr) {
        pair_.GetFirst() = ptr;
    }
    ~UniquePtr() {
        GetDeleter()(pair_.GetFirst());
    }
    void Reset(T* ptr = nullptr) {
        T* old_ptr = pair_.GetFirst();
        pair_.GetFirst() = ptr;
        if (old_ptr) {
            GetDeleter()(old_ptr);
        }
    }
    Deleter& GetDeleter() {
        return pair_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return pair_.GetSecond();
    }
    T& operator[](size_t ind) {
        return pair_.GetFirst()[ind];
    }
    void Swap(UniquePtr& other) {
        std::swap(other.pair_, pair_);
    }
};
