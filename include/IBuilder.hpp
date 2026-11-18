template <typename T>
class IBuilder {
public:
    virtual IBuilder<T>& Add(const T& value) = 0;
    virtual IBuilder<T>& AddSequence(const Sequence<T>* seq) = 0;
    virtual IBuilder<T>& Clear() = 0;
    virtual ~IBuilder() = default;
};

template <typename T>
class ITreeBuilder : public IBuilder<T> {
public:
    virtual NTree<T>* Build() = 0;
    virtual NTree<T>* BuildBalanced() = 0; 
};