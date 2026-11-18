template <typename T>
NTree<T>::NTree(int n) 
    : root_(nullptr), size_(0), n_(n), childSelector_(nullptr) {
    if (n <= 0) {
        throw InvalidArgumentException("NTree", "NTree", "n must be positive");
    }
}

template <typename T>
NTree<T>::~NTree() {
    DestroyRecursive(root_);
    delete childSelector_;
}

template <typename T>
void NTree<T>::DestroyRecursive(Node* node) {
    if (node) {
        for (int i = 0; i < node->n; ++i) {
            DestroyRecursive(node->children[i]);
        }
        delete node;
    }
}

template <typename T>
void NTree<T>::SetChildSelector(const IChildSelector<T>& selector) {
    if (size_ > 0) {
        throw InvalidArgumentException(
            "NTree", "SetChildSelector",
            "Cannot change selector of non-empty tree"
        );
    }
    IChildSelector<T>* clone = selector.Clone();
    delete childSelector_;
    childSelector_ = clone;
}

template <typename T>
IChildSelector<T>* NTree<T>::GetChildSelector() const {
    return childSelector_;
}

template <typename T>
void NTree<T>::Insert(const T& value) {
    if (childSelector_ == nullptr) {
        throw InvalidArgumentException("NTree", "Insert", "Cannot insert: child selector is not set");
    }
    InsertRecursive(root_, value);
}

template <typename T>
void NTree<T>::Add(const T& value) {
    Insert(value);
}

template <typename T>
void NTree<T>::Insert(const T& value, int childIndex) {
    if (childIndex < 0 || childIndex >= n_) {
        throw IndexOutOfRangeException("NTree", "Insert", "Invalid child index", childIndex, n_);
    }
    MutableArraySequence<int> path;
    path.Append(childIndex);
    Insert(value, &path);
}

template <typename T>
void NTree<T>::Insert(const T& value, Sequence<int>* path) {
    if (path == nullptr) {
        throw InvalidArgumentException("NTree", "Insert", "Path is null");
    }
    for (size_t i = 0; i < path->GetLength(); ++i) {
        int idx = path->Get(i);
        if (idx < 0 || idx >= n_) {
            throw IndexOutOfRangeException("NTree", "Insert", "Invalid path index", idx, n_);
        }
    }
    InsertExplicit(root_, value, path, 0);
}

template <typename T>
void NTree<T>::InsertExplicit(Node*& node, const T& value, Sequence<int>* path, size_t pathPos) {
    if (!node) {
        node = new Node(value, n_);
        size_++;
        return;
    }
    if (path != nullptr && pathPos < path->GetLength()) {
        int idx = path->Get(pathPos);
        InsertExplicit(node->children[idx], value, path, pathPos + 1);
        return;
    }
    InsertFirstFree(node, value);
}

template <typename T>
void NTree<T>::InsertFirstFree(Node*& node, const T& value) {
    if (!node) {
        node = new Node(value, n_);
        size_++;
        return;
    }
    for (int i = 0; i < n_; ++i) {
        if (!node->children[i]) {
            node->children[i] = new Node(value, n_);
            size_++;
            return;
        }
    }
    InsertFirstFree(node->children[0], value);
}

template <typename T>
void NTree<T>::InsertRecursive(Node*& node, const T& value) {
    if (!node) {
        node = new Node(value, n_);
        size_++;
        return;
    }

    MutableArraySequence<bool> occupied;
    for (int i = 0; i < n_; ++i) {
        occupied.Append(node->children[i] != nullptr);
    }

    int childIndex = childSelector_->SelectChild(node->data, value, n_, &occupied);

    if (childIndex < 0 || childIndex >= n_) {
        throw InvalidArgumentException(
            "NTree", "InsertRecursive",
            "Invalid child index", childIndex, n_
        );
    }

    InsertRecursive(node->children[childIndex], value);
}

template <typename T>
void NTree<T>::Remove(const T& value) {
    if (childSelector_ == nullptr) {
        throw InvalidArgumentException("NTree", "Remove", "Cannot remove: child selector is not set");
    }
    if (!RemoveRecursive(root_, value)) {
        throw InvalidArgumentException("NTree", "Remove", "Element not found");
    }
}

template <typename T>
bool NTree<T>::RemoveRecursive(Node*& node, const T& value) {
    if (!node) return false;

    if (node->data == value) {
        MutableArraySequence<T> descendantValues;
        for (int i = 0; i < n_; ++i) {
            CollectValues(node->children[i], &descendantValues);
        }

        const size_t removedCount = descendantValues.GetLength() + 1;
        DestroyRecursive(node);
        node = nullptr;
        size_ -= removedCount;

        for (size_t i = 0; i < descendantValues.GetLength(); ++i) {
            InsertRecursive(root_, descendantValues.Get(i));
        }
        return true;
    }

    for (int i = 0; i < n_; ++i) {
        if (RemoveRecursive(node->children[i], value)) return true;
    }
    return false;
}

template <typename T>
void NTree<T>::CollectValues(Node* node, MutableArraySequence<T>* result) const {
    if (!node) return;
    result->Append(node->data);
    for (int i = 0; i < node->n; ++i) {
        CollectValues(node->children[i], result);
    }
}

template <typename T>
void NTree<T>::Clear() {
    DestroyRecursive(root_);
    root_ = nullptr;
    size_ = 0;
}

template <typename T>
int NTree<T>::GetSize() const {
    return static_cast<int>(size_);
}

template <typename T>
bool NTree<T>::IsEmpty() const {
    return size_ == 0;
}

template <typename T>
bool NTree<T>::Contains(const T& value) const {
    return Find(value) != nullptr;
}

template <typename T>
IEnumerator<T>* NTree<T>::GetEnumerator() const {
    return new TreeEnumerator(root_, n_);
}

template <typename T>
typename NTree<T>::Node* NTree<T>::Find(const T& value) const {
    auto findRec = [&](auto self, Node* n) -> Node* {
        if (!n) return nullptr;
        if (n->data == value) return n;
        for (int i = 0; i < n->n; ++i) {
            Node* res = self(self, n->children[i]);
            if (res) return res;
        }
        return nullptr;
    };
    return findRec(findRec, root_);
}

template <typename T>
void NTree<T>::TraverseByOrder(Node* node, Sequence<int>* order, 
                                MutableArraySequence<T>* result) const {
    if (!node) return;

    for (size_t i = 0; i < order->GetLength(); ++i) {
        int idx = order->Get(i);

        if (idx == 0) {
            result->Append(node->data);
        } else if (idx >= 1 && idx <= node->n) {
            TraverseByOrder(node->children[idx - 1], order, result);
        } else {
            throw InvalidArgumentException(
                "NTree", "TraverseByOrder",
                "Invalid order index", idx, node->n
            );
        }
    }
}

template <typename T>
Sequence<T>* NTree<T>::GetSequence(Sequence<int>* order) const {
    if (order == nullptr) {
        throw InvalidArgumentException("NTree", "GetSequence", "Order sequence is null");
    }
    if (order->GetLength() != static_cast<size_t>(n_ + 1)) {
        throw InvalidArgumentException(
            "NTree", "GetSequence",
            "Order size must be n+1", order->GetLength(), n_ + 1
        );
    }

    MutableArraySequence<T>* result = new MutableArraySequence<T>();
    TraverseByOrder(root_, order, result);
    return result;
}

template <typename T>
Sequence<T>* NTree<T>::GetRange(const T& min, const T& max) const {
    if (min > max) {
        throw InvalidArgumentException("NTree", "GetRange", "min > max");
    }
    MutableArraySequence<T>* result = new MutableArraySequence<T>();
    GetRangeRecursive(root_, min, max, result);
    return result;
}

template <typename T>
void NTree<T>::GetRangeRecursive(Node* node, const T& min, const T& max, 
                                  MutableArraySequence<T>* result) const {
    if (!node) return;
    for (int i = 0; i < node->n; ++i) {
        GetRangeRecursive(node->children[i], min, max, result);
    }
    if (node->data >= min && node->data <= max) {
        result->Append(node->data);
    }
}

template <typename T>
bool NTree<T>::NodesEqual(const Node* a, const Node* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a->data != b->data) return false;
    if (a->n != b->n) return false;
    for (int i = 0; i < a->n; ++i) {
        if (!NodesEqual(a->children[i], b->children[i])) return false;
    }
    return true;
}

template <typename U>
std::ostream& operator<<(std::ostream& os, const NTree<U>& tree) {
    IEnumerator<U>* enumerator = tree.GetEnumerator();
    os << "[ ";
    while (enumerator->MoveNext()) {
        os << enumerator->GetCurrent() << " ";
    }
    os << "]";
    delete enumerator;
    return os;
}

template <typename T>
NTree<T>::Iterator::Iterator() {
    stack_ = new MutableArraySequence<Node*>();
}

template <typename T>
NTree<T>::Iterator::Iterator(Node* root, int n) {
    stack_ = new MutableArraySequence<Node*>();
    if (root) stack_->Append(root);
}

template <typename T>
NTree<T>::Iterator::~Iterator() {
    delete stack_;
}

template <typename T>
NTree<T>::Iterator::Iterator(const Iterator& other) {
    stack_ = new MutableArraySequence<Node*>();
    for (size_t i = 0; i < other.stack_->GetLength(); ++i) {
        stack_->Append(other.stack_->Get(i));
    }
}

template <typename T>
typename NTree<T>::Iterator& NTree<T>::Iterator::operator=(const Iterator& other) {
    if (this != &other) {
        delete stack_;
        stack_ = new MutableArraySequence<Node*>();
        for (size_t i = 0; i < other.stack_->GetLength(); ++i) {
            stack_->Append(other.stack_->Get(i));
        }
    }
    return *this;
}

template <typename T>
T NTree<T>::Iterator::operator*() const {
    if (stack_->GetLength() == 0) {
        throw IndexOutOfRangeException("NTree::Iterator", "operator*", "Empty stack");
    }
    return stack_->GetLast()->data;
}

template <typename T>
typename NTree<T>::Iterator& NTree<T>::Iterator::operator++() {
    if (stack_->GetLength() == 0) {
        throw IndexOutOfRangeException("NTree::Iterator", "operator++", "Empty stack");
    }

    Node* current = stack_->GetLast();
    stack_->RemoveLast();

    for (int i = current->n - 1; i >= 0; --i) {
        if (current->children[i]) {
            stack_->Append(current->children[i]);
        }
    }
    return *this;
}

template <typename T>
bool NTree<T>::Iterator::operator==(const Iterator& other) const {
    size_t len1 = stack_->GetLength();
    size_t len2 = other.stack_->GetLength();

    if (len1 == 0 && len2 == 0) return true;
    if (len1 == 0 || len2 == 0) return false;

    return stack_->GetLast() == other.stack_->GetLast();
}

template <typename T>
bool NTree<T>::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

template <typename T>
typename NTree<T>::Iterator NTree<T>::begin() const {
    return Iterator(root_, n_);
}

template <typename T>
typename NTree<T>::Iterator NTree<T>::end() const {
    return Iterator(nullptr, n_);
}