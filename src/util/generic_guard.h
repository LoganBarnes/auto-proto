#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace util {

template <typename Init, typename Destroy, typename... Args>
class GenericGuard;

/**
 * @brief The only way to create a GenericGuard since it has a private constructor
 */
template <typename Init, typename Destroy, typename... Args>
GenericGuard<Init, Destroy, Args...> make_guard(const Init& init_func, Destroy destroy_func, Args&&... args);

/**
 * @brief Serves as a generic way to handle push/pop style calls using RAII
 *
 * This class creates a guard that calls the init function on construction and the destroy
 * function on destruction:
 *
 *     {
 *         auto guard = carpet::make_guard(&push, &pop); // calls push() here on creation
 *         ...
 *         // other code
 *         ...
 *     } // calls pop() here when the guard goes out of scope
 *
 * If both functions accept the same arguments or one function takes arguments and the other does not,
 * the function pointers can be used directly and the arguments can be passed in after:
 *
 *     auto guard1 = carpet::make_guard(&needs_a_string, &needs_the_same_string, "a string for both functions");
 *     ...
 *     auto guard2 = carpet::make_guard(&needs_double, &needs_nothing, 3.14158265359);
 *     ...
 *     auto guard3 = carpet::make_guard(&needs_nothing, &needs_two_args, 42, "this is cool");
 *
 * If both functions need differing arguments then lambdas can be used:
 *
 *     auto guard4 = carpet::make_guard([] { push(arg1, arg2); }, [] { pop(arg3); );
 *     ...
 *     auto guard5 = carpet::make_guard([] { push(arg1, arg2); }, &pop, arg3);
 *
 * @tparam Init is the function called on creation
 * @tparam Destroy is the function called on destruction
 * @tparam Args are the optional arguments used by the Init and Destroy functions
 */
template <typename Init, typename Destroy, typename... Args>
class GenericGuard {
public:
    ~GenericGuard() { call_func(0, destroy_func_, std::make_index_sequence<sizeof...(Args)>()); }

    GenericGuard(const GenericGuard&) = delete;
    GenericGuard(GenericGuard&& that) noexcept = delete;
    GenericGuard& operator=(const GenericGuard&) = delete;
    GenericGuard& operator=(GenericGuard&&) noexcept = delete;

private:
    Destroy destroy_func_;
    std::tuple<typename std::decay<Args>::type...> arguments_;

    /*
     * GenericGuard() is private so this friend function is the only way to create a GenericGuard
     */
    friend GenericGuard make_guard<Init, Destroy, Args...>(const Init& init_func, Destroy destroy_func, Args&&... args);

    explicit GenericGuard(const Init& init_func, Destroy destroy_func, Args&&... args)
        : destroy_func_(destroy_func), arguments_(std::make_tuple(std::forward<Args>(args)...)) {
        call_func(0, init_func, std::make_index_sequence<sizeof...(Args)>());
    }

    // SFINAE function that is called when Func requires arguments
    template <typename Func, std::size_t... S>
    auto call_func(int, const Func& func, std::index_sequence<S...>)
        -> decltype(func(std::get<S>(arguments_)...), void()) {
        func(std::get<S>(arguments_)...);
    }

    // SFINAE function that is called when Func does not require arguments
    template <typename Func, std::size_t... S>
    auto call_func(long, const Func& func, std::index_sequence<S...>) -> decltype(func(), void()) {
        func();
    }
};

template <typename Init, typename Destroy, typename... Args>
GenericGuard<Init, Destroy, Args...> make_guard(const Init& init_func, Destroy destroy_func, Args&&... args) {
    return GenericGuard<Init, Destroy, Args...>(init_func, destroy_func, std::forward<Args>(args)...);
}

} // namespace util
