#pragma once

#include <Judy.h>
#include <cstdint>
#include <cstdio>
#include <utility>

struct judy_128_set
{
    std::pair<void*, bool> insert(const std::pair<uint64_t, uint64_t>& v)
    {
        // insert or get
        void** pv = JudyLIns(&array, v.first, nullptr);

        if (pv == PJERR) {
            return {nullptr, false};
        }

        int inserted = Judy1Set(pv, v.second, nullptr);

        _size += inserted;
        
        return {nullptr, inserted == 1};
    }

    size_t size() const
    {
        return _size;
    }

    ~judy_128_set()
    {
        uint64_t index = 0;
        void** pv = JudyLFirst(array, &index, nullptr);
        while (pv != nullptr) {
            Judy1FreeArray(pv, nullptr);
            pv = JudyLNext(array, &index, nullptr);
        }
        JudyLFreeArray(&array, nullptr);
    }

    void dump()
    {
        printf("size=%lu\n", _size);
        uint64_t l1 = 0;
        void** pv = JudyLFirst(array, &l1, nullptr);
        while (pv != nullptr) {
            uint64_t l2 = 0;
            int r = Judy1First(*pv, &l2, nullptr);
            for (;;) {
                if (r) {
                    printf("0x%08lX, 0x%08lX\n", l1, l2);
                } else {
                    break;
                }
                r = Judy1Next(*pv, &l2, nullptr);
            }
            pv = JudyLNext(array, &l1, nullptr);
        }
    }

private:
    void* array = nullptr;
    size_t _size = 0;
};

