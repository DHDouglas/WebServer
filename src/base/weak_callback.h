#include <memory>
#include <functional>

/* 对于由shared_ptr<T>管理的对象, WeakCallback用于将其成员函数绑定到weak_ptr<T>
 * 避免意外延长目标生命周期的同时, 也防止回调发生时对象已析构从而导致访问无效内存.
 */

template <typename T, typename... ARGS>
class WeakCallback { 
public:
    WeakCallback(const std::weak_ptr<T>& object,
                 const std::function<void(T*, ARGS...)> function)
        : obj_(object),
        func_(function) {}
    
    void operator()(ARGS&&... args) {
        if (auto sptr = obj_.lock()) {
            func_(sptr.get(), std::forward<ARGS>(args)...); 
        }
    }

private:
    std::weak_ptr<T> obj_;
    std::function<void(T*, ARGS...)> func_;
};


template <typename T, typename... ARGS>
WeakCallback<T, ARGS...> makeWeakCallback(
    const std::shared_ptr<std::remove_reference_t<T>>& obj,
    void (T::*func)(ARGS...))
{
    return WeakCallback<T, ARGS...>(obj, std::move(func));
}

template <typename T, typename... ARGS>
WeakCallback<T, ARGS...> makeWeakCallback(
    const std::shared_ptr<std::remove_reference_t<T>>& obj,
    void (T::*func)(ARGS...) const)
{
    return WeakCallback<T, ARGS...>(obj, std::move(func));
}