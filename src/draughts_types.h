#pragma once

#include <array>



// 2-dimentional coordinate of draughts item on board
// each coordinate starts from 0
// coordinate can be negative - position out of board or move (position difference)
struct brd_2d_vector_t
{
    int x = -1;
    int y = -1;

    constexpr brd_2d_vector_t() = default;
    constexpr brd_2d_vector_t(int x, int y) : x(x), y(y) {}
    constexpr brd_2d_vector_t(const brd_2d_vector_t& other) : x(other.x), y(other.y) {}

    constexpr friend brd_2d_vector_t operator+(const brd_2d_vector_t& l, const brd_2d_vector_t& r)
    {
        return {l.x + r.x, l.y + r.y};
    }

    brd_2d_vector_t& operator+=(const brd_2d_vector_t& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr friend brd_2d_vector_t operator*(const brd_2d_vector_t& l, int r)
    {
        return {l.x*r, l.y*r};
    }

    explicit constexpr operator bool() const
    {
        return x >= 0 && x < 8 && y >= 0 && y < 8;
    }
};

inline const brd_2d_vector_t up_left    = {-1,  1};
inline const brd_2d_vector_t up_right   = { 1,  1};
inline const brd_2d_vector_t down_left  = {-1, -1};
inline const brd_2d_vector_t down_right = { 1, -1};



constexpr auto gen_2d_table()
{
    std::array<brd_2d_vector_t, 32> r;

    brd_2d_vector_t x(0, 0);
    for (int i = 0; i < 32; i++) {
        r[i] = x;
        x.x += 2;
        if ((x.x / 8) > 0) {
            x.y++;
            x.x %= 8;
            x.x += (x.y % 2 > 0) ? 1 : -1;
        }
    }

    return r;
}

inline const std::array<brd_2d_vector_t, 32> brd_1d_to_2d_table = gen_2d_table();



// 1-dimentional coordiante of draughts item on board
struct brd_index_t
{
    int index = -1;

    constexpr brd_index_t() = default;
    constexpr brd_index_t(int v) : index(v) {}
    explicit constexpr brd_index_t(const brd_2d_vector_t& v) : index((8*v.y + v.x) / 2) {}

    brd_index_t& operator++()
    {
        index++;
        return *this;
    }

    explicit constexpr operator bool() const
    {
        return index >= 0 && index < 32;
    }

    explicit constexpr operator brd_2d_vector_t() const
    {
        if (*this) {
            return brd_1d_to_2d_table[index];
        } else {
            return {};
        }
    }
};

// 32-bit mask representing single item on board
struct brd_item_t
{
    uint32_t mask = 0;

    constexpr brd_item_t() = default;
    //constexpr brd_item_t(uint32_t v) : mask(v) {}
    explicit constexpr brd_item_t(const brd_index_t& index) : mask(1 << index.index) {}
    explicit constexpr brd_item_t(const brd_2d_vector_t& v) : mask(1 << brd_index_t(v).index) {}

    explicit constexpr operator bool() const
    {
        return mask != 0;
    }

    bool is_on_king_row() const
    {
        return mask & (0xF << 28);
    }
};


// 32-bit mask representing multiple items on board
struct brd_map_t
{
    uint32_t mask = 0;

    constexpr brd_map_t() = default;
    constexpr brd_map_t(uint32_t v) : mask(v) {}
    explicit constexpr brd_map_t(brd_item_t item) : mask(item.mask) {}

    explicit constexpr operator bool() const
    {
        return mask != 0;
    }

    friend brd_map_t operator+(const brd_map_t& l, const brd_map_t& r)
    {
        return l.mask | r.mask;
    }

    friend brd_map_t operator+(const brd_map_t& l, const brd_item_t& item)
    {
        return l.mask | item.mask;
    }

    friend brd_map_t operator-(const brd_map_t& l, const brd_map_t& r)
    {
        return l.mask & ~r.mask;
    }

    brd_map_t& operator+=(const brd_item_t& item)
    {
        mask |= item.mask;
        return *this;
    }
    brd_map_t& operator-=(const brd_item_t& item)
    {
        mask &= ~item.mask;
        return *this;
    }

    brd_map_t& operator+=(const brd_map_t& map)
    {
        mask |= map.mask;
        return *this;
    }
    brd_map_t& operator-=(const brd_map_t& map)
    {
        mask &= ~map.mask;
        return *this;
    }

    bool exist(const brd_item_t& item) const
    {
        return (mask & item.mask) != 0;
    }

    bool exist_any(const brd_map_t& map) const
    {
        return (mask & map.mask) != 0;
    }

    brd_map_t select(const brd_map_t& items) const
    {
        return mask & items.mask;
    }
};

// https://stackoverflow.com/questions/746171/efficient-algorithm-for-bit-reversal-from-msb-lsb-to-lsb-msb-in-c
inline const uint32_t bit_reverse_table_256[] = 
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

brd_map_t reverse(const brd_map_t& m)
{
    return (bit_reverse_table_256[m.mask & 0xff] << 24) | 
           (bit_reverse_table_256[(m.mask >> 8) & 0xff] << 16) | 
           (bit_reverse_table_256[(m.mask >> 16) & 0xff] << 8) |
           (bit_reverse_table_256[(m.mask >> 24) & 0xff]);
}

//TODO: possible representations of item: bitmap, index, coordinates {x, y}, string e.g. "e5"
//TODO: conversions between each representations

//TODO: same for one-step moves: bitmap-bitmap, index-index, x,y-x,y, "e5-f6"
//TODO: complete move with captures - more complicated, use board states instead 


//TODO: magic constants, related to type size

