#pragma once
#include <iostream>
#include <cstddef>
#include "base/Sequence.hpp"
#include "base/MutableArraySequence.hpp"
#include "base/Exceptions.hpp"
#include "IChildSelector.hpp"
#include "Interfaces.hpp"
#include <concepts> 

template <typename T>
concept Comparable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
};

template <Comparable T> 
class NTree : public ICollection<T> {
private:
    struct Node {
        T data;
        Node** children;
        int n;

        Node(const T& val, int numChildren) : data(val), n(numChildren) {
            children = new Node*[n];
            for (int i = 0; i < n; ++i) children[i] = nullptr;
        }
        ~Node() { delete[] children; }
    };

    Node* root_;
    size_t size_;
    int n_;
    IChildSelector<T>* childSelector_;

    void DestroyRecursive(Node* node);
    void InsertRecursive(Node*& node, const T& value);
    void InsertExplicit(Node*& node, const T& value, Sequence<int>* path, size_t pathPos);
    void InsertFirstFree(Node*& node, const T& value);
    bool RemoveRecursive(Node*& node, const T& value);
    void CollectValues(Node* node, MutableArraySequence<T>* result) const;
    void TraverseByOrder(Node* node, Sequence<int>* order, MutableArraySequence<T>* result) const;
    void GetRangeRecursive(Node* node, const T& min, const T& max, MutableArraySequence<T>* result) const;

    static bool NodesEqual(const Node* a, const Node* b);

public:
    explicit NTree(int n);
    ~NTree();

    NTree(const NTree&) = delete;
    NTree& operator=(const NTree&) = delete;

    void SetChildSelector(const IChildSelector<T>& selector);
    IChildSelector<T>* GetChildSelector() const;

    void Insert(const T& value);
    void Insert(const T& value, int childIndex);
    void Insert(const T& value, Sequence<int>* path);
    void Remove(const T& value);
    void Clear();
    Node* Find(const T& value) const;
    // Доступ к корню для внешней визуализации
    Node* GetRoot() const { return root_; }
    Sequence<T>* GetSequence(Sequence<int>* order) const;
    Sequence<T>* GetRange(const T& min, const T& max) const;

    int GetSize() const override;
    bool IsEmpty() const override;
    bool Contains(const T& value) const override;
    void Add(const T& value) override; 

    IEnumerator<T>* GetEnumerator() const override;

    class TreeEnumerator : public IEnumerator<T> {
    private:
        Node* root_;
        int n_;
        MutableArraySequence<Node*>* stack_;
        Node* current_;

    public:
        TreeEnumerator(Node* root, int n) : root_(root), n_(n), current_(nullptr) {
            stack_ = new MutableArraySequence<Node*>();
            Reset();
        }
        ~TreeEnumerator() { delete stack_; }

        void Reset() override {
            delete stack_;
            stack_ = new MutableArraySequence<Node*>();
            current_ = nullptr;
            if (root_) stack_->Append(root_);
        }

        bool MoveNext() override {
            if (stack_->GetLength() == 0) return false;
            current_ = stack_->GetLast();
            stack_->RemoveLast();
            for (int i = current_->n - 1; i >= 0; --i) {
                if (current_->children[i]) stack_->Append(current_->children[i]);
            }
            return true;
        }

        T GetCurrent() const override {
            if (!current_) throw IndexOutOfRangeException("TreeEnumerator", "GetCurrent", "No current element");
            return current_->data;
        }
    };

    friend bool operator==(const NTree<T>& a, const NTree<T>& b) {
        return NodesEqual(a.root_, b.root_);
    }
    friend bool operator!=(const NTree<T>& a, const NTree<T>& b) {
        return !(a == b);
    }
    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const NTree<U>& tree);

    class Iterator {
    private:
        MutableArraySequence<Node*>* stack_;
    public:
        Iterator();
        Iterator(Node* root, int n);
        ~Iterator();
        Iterator(const Iterator& other);
        Iterator& operator=(const Iterator& other);
        T operator*() const;
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;
        bool operator==(const Iterator& other) const;
    };

    Iterator begin() const;
    Iterator end() const;
};

#include "NTree.tpp"