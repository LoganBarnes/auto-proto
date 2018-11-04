#pragma once

#include <proj/proj_config.h>

#include <algorithm>
#include <string>
#include <vector>

#ifdef PROJ_ARM_SERVER_ONLY
#include <memory>
#include <tuple>
#endif

namespace util {

#ifdef PROJ_ARM_SERVER_ONLY
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace detail {

template <std::size_t...>
struct index_sequence {};

template <std::size_t N, std::size_t... S>
struct make_index_sequence : make_index_sequence<N - 1, N - 1, S...> {};

template <std::size_t... S>
struct make_index_sequence<0, S...> {
    typedef index_sequence<S...> type;
};

template <class F, class Tuple, std::size_t... I>
constexpr auto apply_impl(F f, Tuple&& t, index_sequence<I...>) {
    return f(std::get<I>(std::forward<Tuple>(t))...);
}
} // namespace detail

template <class F, typename... Args>
constexpr auto apply(F&& f, const std::tuple<Args...>& t) {
    return detail::apply_impl(std::forward<F>(f), t, typename detail::make_index_sequence<sizeof...(Args)>::type());
}

#endif

template <typename T>
auto remove_by_value(std::vector<T>* vec, const T& val) {
    return vec->erase(std::remove(vec->begin(), vec->end(), val), vec->end());
}

std::string to_upper(std::string str);

std::string to_lower(std::string str);

} // namespace util
