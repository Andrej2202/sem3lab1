#pragma once
#include "NTree.hpp"
#include "IBuilder.hpp"
#include "IChildSelector.hpp"
#include "Exceptions.hpp"

template <typename Container, typename T>
concept ContainerLike = requires(Container c, const T& value, size_t index) {
    c.Append(value);
    { c.Get(index) } -> std::convertible_to<T>;
    { c.GetLength() } -> std::convertible_to<size_t>;
};

template <typename T, typename ContainerType>
requires ContainerLike<ContainerType, T>
class NTreeBuilder : public ITreeBuilder<T> {
private:
    int n_;
    ContainerType* elements_;
    IChildSelector<T>* selector_;

public:
    NTreeBuilder(int n) : n_(n), elements_(nullptr), selector_(nullptr) {
        if (n <= 0) {
            throw InvalidArgumentException("NTreeBuilder", "NTreeBuilder", "n must be positive");
        }
        elements_ = new ContainerType();
    }

    ~NTreeBuilder() {
        delete elements_;
        delete selector_;
    }

    NTreeBuilder(const NTreeBuilder&) = delete;
    NTreeBuilder& operator=(const NTreeBuilder&) = delete;


    NTreeBuilder& SetSelector(const IChildSelector<T>& selector) {
        IChildSelector<T>* clone = selector.Clone();
        delete selector_;
        selector_ = clone;
        return *this;
    }

    IBuilder<T>& Add(const T& value) override {
        elements_->Append(value);
        return *this;
    }

    IBuilder<T>& AddSequence(const Sequence<T>* seq) override {
        if (seq == nullptr) {
            throw InvalidArgumentException("NTreeBuilder", "AddSequence", "Sequence is null");
        }
        for (size_t i = 0; i < seq->GetLength(); ++i) {
            elements_->Append(seq->Get(i));
        }
        return *this;
    }

    IBuilder<T>& Clear() override {
        delete elements_;
        elements_ = new ContainerType();
        return *this;
    }

    NTree<T>* Build() override {
        NTree<T>* tree = new NTree<T>(n_);
        
        if (selector_ != nullptr) {
            tree->SetChildSelector(*selector_);
        }
        
        for (size_t i = 0; i < elements_->GetLength(); ++i) {
            tree->Insert(elements_->Get(i));
        }
        
        return tree;
    }

    NTree<T>* BuildBalanced() override {
        return Build();
    }
};