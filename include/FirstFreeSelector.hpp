#pragma once
#include "IChildSelector.hpp"

template <typename T>
class FirstFreeSelector : public IChildSelector<T> {
public:
    int SelectChild(const T& /*parentValue*/, const T& /*value*/, int /*n*/) const override {
        return 0;
    }

    int SelectChild(const T& /*parentValue*/, const T& /*value*/, int n,
                    const Sequence<bool>* occupied) const override {
        if (occupied != nullptr) {
            for (int i = 0; i < n; ++i) {
                if (!occupied->Get(i)) return i;
            }
        }
        return 0;
    }

    IChildSelector<T>* Clone() const override {
        return new FirstFreeSelector<T>(*this);
    }
};