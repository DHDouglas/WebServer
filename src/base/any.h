#pragma once

/* Any类的简单实现.
 * 利用多态机制, Any持有一个指向Base类的指针, 但实际指向的是多态派生类Derive.
 * Derive是模版类, 通过模版构造函数, 实际可支持存储任意类型.
 */

#include <memory>
#include <type_traits>

class Any {
private:
    struct Base {
        virtual ~Base() = default; 
    };

    template<typename T>
    struct Derived : Base {
        explicit Derived(T& v) : value(v) {}
        explicit Derived(T&& v) : value(std::move(v)) {}

        T value; 
    };

    std::shared_ptr<Base> data;  // 指向基类, 实现类型擦除. 

public:
    template<typename T>
    using DataType = Derived<T>;

public:
    Any() = default;

    Any(const Any& rhs) { data = rhs.data; }

    Any(Any&& rhs) { data = std::move(rhs.data); }

    template <typename T>
    Any(T&& t): data(std::make_shared<Derived<typename std::decay<T>::type>>(std::forward<T>(t))) {}

    Any& operator=(const Any& rhs) {
        if (this != &rhs) {
            data = rhs.data;
        }
        return *this;
    }

    Any& operator=(Any&& rhs) {
        if (this != &rhs) {
            data = std::move(rhs.data);
        }
        return *this;
    }

    template <typename T>
    Any& operator=(T&& t) {
        data = std::make_shared<Derived<typename std::decay<T>::type>>(std::forward<T>(t)); 
        return *this;
    }

    const std::shared_ptr<Base>& getData() const { return data; }
};


template <typename T>
T* any_cast(Any* any)  {
    auto ptr = std::dynamic_pointer_cast<Any::DataType<T>>(any->getData());
    if (ptr) {
        return &(ptr->value);
    }
    return nullptr;
}