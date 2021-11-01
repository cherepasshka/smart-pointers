#pragma once

#include <utility>
#include <type_traits>

template <class T, std::size_t I, bool = std::is_empty_v<T> && !std::is_final_v<T>>
struct Element {
    T value;
    Element() : value(T()) {
    }
    template<class El>
    Element(El&& data) : value(std::forward<El>(data)) {
    }
    T& GetElement() {
        return value;
    }
    const T& GetElement() const {
        return value;
    }
};

template <class T, std::size_t I>
struct Element<T, I, true> : T {
    Element() : T() {
    }
    template <class El>
    Element(El&& data) : T(std::forward<El>(data)) {
    }
    const T& GetElement() const {
        return *this;
    }
    T& GetElement() {
        return *this;
    }
};

template <typename F, typename S>
class CompressedPair : Element<F, 0>, Element<S, 1> {
public:
    CompressedPair() : Element<F, 0>(), Element<S, 1>() {
    }
    template <class Fs, class Sc>
    CompressedPair(Fs&& first, Sc&& second)
        : Element<F, 0>(std::forward<Fs>(first)), Element<S, 1>(std::forward<Sc>(second)) {
    }

    F& GetFirst() {
        return Element<F, 0>::GetElement();
    }
    const F& GetFirst() const {
        return Element<F, 0>::GetElement();
    }
    S& GetSecond() {
        return Element<S, 1>::GetElement();
    };
    const S& GetSecond() const {
        return Element<S, 1>::GetElement();
    };
};
