#pragma once
#include "IChildSelector.hpp"

template <typename T>
class CompareSelector : public IChildSelector<T> {
public:
    int SelectChild(const T& parentValue, const T& value, int n) const override {
        if (value < parentValue) return 0;
        if (value > parentValue) return n - 1;
        return n / 2;
    }
    
    IChildSelector<T>* Clone() const override {
        return new CompareSelector<T>(*this);
    }
};