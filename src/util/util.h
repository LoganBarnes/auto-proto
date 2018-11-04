#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace util {

template <typename T>
auto remove_by_value(std::vector<T>* vec, const T& val) {
    return vec->erase(std::remove(vec->begin(), vec->end(), val), vec->end());
}

std::string to_upper(std::string str);

std::string to_lower(std::string str);

} // namespace util
