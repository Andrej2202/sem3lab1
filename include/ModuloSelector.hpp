#pragma once
#include <concepts>
#include "IChildSelector.hpp"

template <std::integral T>
class ModuloSelector : public IChildSelector<T> {
public:
    int SelectChild(const T& parentValue, const T& value, int n) const override {
        long long diff = static_cast<long long>(value) - static_cast<long long>(parentValue);
        int index = static_cast<int>(diff % n);
        return index < 0 ? index + n : index;
    }

    IChildSelector<T>* Clone() const override {
        return new ModuloSelector<T>(*this);
    }
};