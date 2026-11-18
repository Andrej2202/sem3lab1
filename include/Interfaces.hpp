#pragma once

template <typename T>
class IEnumerator {
public:
    virtual ~IEnumerator() = default;
    virtual T GetCurrent() const = 0;
    virtual bool MoveNext() = 0;
    virtual void Reset() = 0;
};

template <typename T>
class IEnumerable {
public:
    virtual ~IEnumerable() = default;
    virtual IEnumerator<T>* GetEnumerator() const = 0;
};

template <typename T>
class ICollection : public IEnumerable<T> {
public:
    virtual ~ICollection() = default;
    virtual int GetSize() const = 0;
    virtual bool IsEmpty() const = 0;
    virtual bool Contains(const T& value) const = 0;
    virtual void Clear() = 0;
    virtual void Add(const T& value) = 0;
};