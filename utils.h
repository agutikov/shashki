#pragma once

#include <vector>
#include <chrono>
#include <iostream>
#include <ctime>
#include <iomanip>

template<typename T>
std::vector<std::vector<T>> split(const std::vector<T>& v, size_t num_chunks)
{
    std::vector<std::vector<T>> r;
    r.reserve(num_chunks);

    auto beg = v.begin();
    auto en = v.end();

    while (num_chunks > 0) {
        size_t left = en - beg;
        if (left == 0) {
            break;
        }
        size_t len = left / num_chunks;
        if (len == 0) {
            break;
        }
        r.emplace_back(beg, beg + len);
        beg += len;
        num_chunks--;
    }

    return r;
}

template<class clock>
void print_tp(const typename clock::time_point& tp)
{
    auto t = clock::to_time_t(tp);
    std::cout << std::put_time(std::localtime(&t), "%F %T") << std::endl;
}
