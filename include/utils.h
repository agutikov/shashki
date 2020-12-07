#pragma once

#include <vector>
#include <chrono>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <map>

#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace std::literals::chrono_literals;



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

template<typename T>
float total_seconds(T d)
{
    using fsecs = std::chrono::duration<float, std::chrono::seconds::period>;
    return std::chrono::duration_cast<fsecs>(d).count();
}

template<class clock>
struct readable_duration_t
{
    using duration = typename clock::duration;

    duration value;
    
    readable_duration_t() = default;

    readable_duration_t(const duration& d) :
        value(d)
    {}

    readable_duration_t& operator= (const duration& d)
    {
        value = d;
        return *this;
    }

    static const std::map<std::string, duration> units;

    friend std::ostream& operator<<(std::ostream &out, const readable_duration_t& d)
    {
        out << d.value.count() << 's';
        return out;
    }

    friend std::istream& operator>>(std::istream &in, readable_duration_t& d)
    {
        float v;
        std::string u;
        in >> v;
        
        if (!in.eof()) {
            in >> u;
        }

        duration unit = 1s;

        auto it = units.find(u);
        if (it != units.end()) {
            unit = it->second;
        }

        d.value = std::chrono::duration_cast<duration>(unit * v);

        return in;
    }

    static std::string all_units(const std::string& delim)
    {
        return boost::algorithm::join(units | boost::adaptors::map_keys, delim);
    }
};

template<class clock>
const std::map<std::string, typename readable_duration_t<clock>::duration>
readable_duration_t<clock>::units = 
{
    {"us", 1us},
    {"ms", 1ms},
    {"s", 1s},
    {"m", 1min},
    {"h", 1h},
    {"d", 24h}
};

