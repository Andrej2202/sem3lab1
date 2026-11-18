#pragma once
#include "base/Sequence.hpp"

template <typename T>
class IChildSelector {
public:
    virtual ~IChildSelector() = default;

    virtual int SelectChild(const T& parentValue, const T& value, int n) const = 0;

    virtual int SelectChild(const T& parentValue, const T& value, int n,
                            const Sequence<bool>* occupied) const {
        (void)occupied;
        return SelectChild(parentValue, value, n);
    }

    virtual IChildSelector<T>* Clone() const = 0;
};