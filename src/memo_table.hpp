#pragma once

#include "bitboard.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace minigo {

class PackedMemo {
public:
    explicit PackedMemo(std::uint64_t states)
        : data_((states * 2 + 7) / 8, 0) {}

    std::uint8_t get(std::uint64_t rank) const {
        std::uint64_t bit = rank * 2;
        std::uint64_t byte = bit >> 3;
        int shift = static_cast<int>(bit & 7);
        return (data_[byte] >> shift) & 0b11;
    }

    void set(std::uint64_t rank, std::uint8_t value) {
        std::uint64_t bit = rank * 2;
        std::uint64_t byte = bit >> 3;
        int shift = static_cast<int>(bit & 7);
        data_[byte] = static_cast<std::uint8_t>((data_[byte] & ~(0b11u << shift)) | (value << shift));
    }

    std::size_t bytes() const {
        return data_.size();
    }

private:
    std::vector<std::uint8_t> data_;
};

class SparseMemo {
public:
    explicit SparseMemo(std::uint64_t expected_entries, std::uint64_t initial_capacity = 0) {
        init_pow3_table();
        init_chunk_table();
        std::uint64_t capacity = 1;
        std::uint64_t target = initial_capacity != 0 ? initial_capacity : expected_entries * 2;
        while (capacity < target) capacity <<= 1;
        entries_.assign(capacity * ENTRY_BYTES, 0);
        mask_ = capacity - 1;
    }

    std::uint8_t get(Board32 black, Board32 white) const {
        return get_key(make_key(black, white));
    }

    std::uint8_t get_key(std::uint64_t key) const {
        std::uint64_t idx = mix(key) & mask_;
        std::uint64_t probes = 1;
        while (true) {
            std::uint64_t entry = load_entry(idx);
            std::uint8_t value = static_cast<std::uint8_t>(entry & 0b11);
            if (value == 0) {
                record_get_probes(probes);
                return 0;
            }
            if ((entry >> 2) == key) {
                record_get_probes(probes);
                return value;
            }
            ++get_collisions_;
            idx = (idx + 1) & mask_;
            ++probes;
        }
    }

    void set(Board32 black, Board32 white, std::uint8_t value) {
        set_key(make_key(black, white), value);
    }

    void set_key(std::uint64_t key, std::uint8_t value) {
        if ((filled_ + 1) * 10 >= capacity() * 7) {
            rehash(capacity() * 2);
        }

        std::uint64_t idx = mix(key) & mask_;
        std::uint64_t probes = 1;
        std::uint64_t new_entry = (key << 2) | value;
        while (true) {
            std::uint64_t entry = load_entry(idx);
            std::uint8_t old_value = static_cast<std::uint8_t>(entry & 0b11);
            if (old_value == 0) {
                store_entry(idx, new_entry);
                ++filled_;
                record_set_probes(probes);
                return;
            }
            if ((entry >> 2) == key) {
                store_entry(idx, new_entry);
                record_set_probes(probes);
                return;
            }
            ++set_collisions_;
            idx = (idx + 1) & mask_;
            ++probes;
        }
    }

    static std::uint64_t make_key(Board32 black, Board32 white) {
        return ternary_key(black, white);
    }

    std::uint64_t filled_count(std::uint64_t) const {
        return filled_;
    }

    std::size_t bytes() const {
        return entries_.size();
    }

    std::uint64_t capacity() const {
        return entries_.size() / ENTRY_BYTES;
    }

    std::uint64_t get_calls() const { return get_calls_; }
    std::uint64_t set_calls() const { return set_calls_; }
    std::uint64_t get_collisions() const { return get_collisions_; }
    std::uint64_t set_collisions() const { return set_collisions_; }
    std::uint64_t get_probes() const { return get_probes_; }
    std::uint64_t set_probes() const { return set_probes_; }
    std::uint64_t max_get_probe() const { return max_get_probe_; }
    std::uint64_t max_set_probe() const { return max_set_probe_; }

private:
    static constexpr std::uint64_t ENTRY_BYTES = 7;

    static void init_pow3_table() {
        if (pow3_table_[0] != 0) return;
        pow3_table_[0] = 1;
        for (int i = 1; i <= 32; ++i) {
            pow3_table_[i] = pow3_table_[i - 1] * 3ULL;
        }
    }

    static std::uint64_t mix(std::uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    static void init_chunk_table() {
        static bool initialized = false;
        if (initialized) return;
        for (int chunk = 0; chunk < 4; ++chunk) {
            for (int black_byte = 0; black_byte < 256; ++black_byte) {
                for (int white_byte = 0; white_byte < 256; ++white_byte) {
                    std::uint64_t value = 0;
                    for (int bit = 0; bit < 8; ++bit) {
                        int digit = (black_byte & (1 << bit)) ? 1 : (white_byte & (1 << bit)) ? 2 : 0;
                        value += static_cast<std::uint64_t>(digit) * pow3_table_[chunk * 8 + bit];
                    }
                    chunk_rank_[chunk][(black_byte << 8) | white_byte] = value;
                }
            }
        }
        initialized = true;
    }

    static std::uint64_t ternary_key(Board32 black, Board32 white) {
        return chunk_rank_[0][((black & 0xffu) << 8) | (white & 0xffu)]
             + chunk_rank_[1][(((black >> 8) & 0xffu) << 8) | ((white >> 8) & 0xffu)]
             + chunk_rank_[2][(((black >> 16) & 0xffu) << 8) | ((white >> 16) & 0xffu)]
             + chunk_rank_[3][(((black >> 24) & 0xffu) << 8) | ((white >> 24) & 0xffu)];
    }

    std::uint64_t load_entry(std::uint64_t idx) const {
        const std::uint8_t* p = entries_.data() + idx * ENTRY_BYTES;
        return static_cast<std::uint64_t>(p[0])
             | (static_cast<std::uint64_t>(p[1]) << 8)
             | (static_cast<std::uint64_t>(p[2]) << 16)
             | (static_cast<std::uint64_t>(p[3]) << 24)
             | (static_cast<std::uint64_t>(p[4]) << 32)
             | (static_cast<std::uint64_t>(p[5]) << 40)
             | (static_cast<std::uint64_t>(p[6]) << 48);
    }

    void store_entry(std::uint64_t idx, std::uint64_t entry) {
        std::uint8_t* p = entries_.data() + idx * ENTRY_BYTES;
        p[0] = static_cast<std::uint8_t>(entry);
        p[1] = static_cast<std::uint8_t>(entry >> 8);
        p[2] = static_cast<std::uint8_t>(entry >> 16);
        p[3] = static_cast<std::uint8_t>(entry >> 24);
        p[4] = static_cast<std::uint8_t>(entry >> 32);
        p[5] = static_cast<std::uint8_t>(entry >> 40);
        p[6] = static_cast<std::uint8_t>(entry >> 48);
    }

    void record_get_probes(std::uint64_t probes) const {
        ++get_calls_;
        get_probes_ += probes;
        if (probes > max_get_probe_) max_get_probe_ = probes;
    }

    void record_set_probes(std::uint64_t probes) {
        ++set_calls_;
        set_probes_ += probes;
        if (probes > max_set_probe_) max_set_probe_ = probes;
    }

    void rehash(std::uint64_t new_capacity) {
        std::vector<std::uint8_t> old_entries = std::move(entries_);
        std::uint64_t old_capacity = old_entries.size() / ENTRY_BYTES;
        entries_.assign(new_capacity * ENTRY_BYTES, 0);
        mask_ = new_capacity - 1;
        filled_ = 0;
        for (std::uint64_t i = 0; i < old_capacity; ++i) {
            const std::uint8_t* p = old_entries.data() + i * ENTRY_BYTES;
            std::uint64_t entry = static_cast<std::uint64_t>(p[0])
                | (static_cast<std::uint64_t>(p[1]) << 8)
                | (static_cast<std::uint64_t>(p[2]) << 16)
                | (static_cast<std::uint64_t>(p[3]) << 24)
                | (static_cast<std::uint64_t>(p[4]) << 32)
                | (static_cast<std::uint64_t>(p[5]) << 40)
                | (static_cast<std::uint64_t>(p[6]) << 48);
            std::uint8_t value = static_cast<std::uint8_t>(entry & 0b11);
            if (value == 0) continue;
            set_key(entry >> 2, value);
        }
    }

    std::vector<std::uint8_t> entries_;
    std::uint64_t mask_ = 0;
    std::uint64_t filled_ = 0;
    mutable std::uint64_t get_calls_ = 0;
    std::uint64_t set_calls_ = 0;
    mutable std::uint64_t get_collisions_ = 0;
    std::uint64_t set_collisions_ = 0;
    mutable std::uint64_t get_probes_ = 0;
    std::uint64_t set_probes_ = 0;
    mutable std::uint64_t max_get_probe_ = 0;
    std::uint64_t max_set_probe_ = 0;

    inline static std::uint64_t pow3_table_[33] = {};
    inline static std::uint64_t chunk_rank_[4][65536] = {};
};

} // namespace minigo
