#pragma once

#include "bitboard.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
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
    explicit SparseMemo(int n, std::uint64_t expected_entries, std::uint64_t initial_capacity = 0)
        : n_(n),
          suffix_((static_cast<std::size_t>(n_) + 1) * SYMBOLS, 0),
          chunk_rank_(CHUNKS * SYMBOLS * CHUNK_KEYS) {
        init_suffix_counts();
        init_chunk_rank();
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

    static constexpr std::uint64_t entry_bytes() {
        return ENTRY_BYTES;
    }

    std::uint64_t make_key(Board32 black, Board32 white) const {
        std::uint64_t rank = 0;
        int prev = EMPTY;
        for (int chunk = 0; chunk < CHUNKS; ++chunk) {
            std::uint32_t black_byte = (black >> (chunk * CHUNK_BITS)) & 0xffu;
            std::uint32_t white_byte = (white >> (chunk * CHUNK_BITS)) & 0xffu;
            const ChunkRank& cr = chunk_rank(chunk, prev, (black_byte << 8) | white_byte);
            if (cr.next < 0) {
                throw std::logic_error("attempted to memoize a board with adjacent black and white stones");
            }
            rank += cr.rank;
            prev = cr.next;
        }
        return rank;
    }

private:
    static constexpr int EMPTY = 0;
    static constexpr int BLACK = 1;
    static constexpr int WHITE = 2;
    static constexpr int SYMBOLS = 3;
    static constexpr int CHUNK_BITS = 8;
    static constexpr int CHUNKS = 4;
    static constexpr int CHUNK_KEYS = 1 << 16;
    static constexpr std::uint64_t ENTRY_BYTES = 6;

    struct ChunkRank {
        std::uint64_t rank = 0;
        int8_t next = -1;
    };

    static std::uint64_t mix(std::uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    void init_suffix_counts() {
        for (int prev = 0; prev < SYMBOLS; ++prev) {
            suffix(n_, prev) = 1;
        }
        for (int pos = n_ - 1; pos >= 0; --pos) {
            for (int prev = 0; prev < SYMBOLS; ++prev) {
                std::uint64_t total = 0;
                for (int symbol = 0; symbol < SYMBOLS; ++symbol) {
                    if (is_contact(prev, symbol)) continue;
                    total += suffix(pos + 1, symbol);
                }
                suffix(pos, prev) = total;
            }
        }
    }

    void init_chunk_rank() {
        for (int chunk = 0; chunk < CHUNKS; ++chunk) {
            int start = chunk * CHUNK_BITS;
            int end = std::min(n_, start + CHUNK_BITS);
            for (int prev = 0; prev < SYMBOLS; ++prev) {
                for (int black_byte = 0; black_byte < 256; ++black_byte) {
                    for (int white_byte = 0; white_byte < 256; ++white_byte) {
                        ChunkRank cr{};
                        int current = prev;
                        bool valid = true;
                        for (int pos = start; pos < end; ++pos) {
                            int bit = pos - start;
                            bool has_black = (black_byte & (1 << bit)) != 0;
                            bool has_white = (white_byte & (1 << bit)) != 0;
                            if (has_black && has_white) {
                                valid = false;
                                break;
                            }
                            int actual = has_black ? BLACK : has_white ? WHITE : EMPTY;
                            for (int symbol = 0; symbol < actual; ++symbol) {
                                if (is_contact(current, symbol)) continue;
                                cr.rank += suffix(pos + 1, symbol);
                            }
                            if (is_contact(current, actual)) {
                                valid = false;
                                break;
                            }
                            current = actual;
                        }
                        cr.next = valid ? static_cast<int8_t>(current) : -1;
                        chunk_rank(chunk, prev, (black_byte << 8) | white_byte) = cr;
                    }
                }
            }
        }
    }

    static bool is_contact(int left, int right) {
        return (left == BLACK && right == WHITE) || (left == WHITE && right == BLACK);
    }

    std::uint64_t& suffix(int pos, int prev) {
        return suffix_[static_cast<std::size_t>(pos) * SYMBOLS + prev];
    }

    std::uint64_t suffix(int pos, int prev) const {
        return suffix_[static_cast<std::size_t>(pos) * SYMBOLS + prev];
    }

    ChunkRank& chunk_rank(int chunk, int prev, int key) {
        return chunk_rank_[(static_cast<std::size_t>(chunk) * SYMBOLS + prev) * CHUNK_KEYS
                         + static_cast<std::size_t>(key)];
    }

    const ChunkRank& chunk_rank(int chunk, int prev, int key) const {
        return chunk_rank_[(static_cast<std::size_t>(chunk) * SYMBOLS + prev) * CHUNK_KEYS
                         + static_cast<std::size_t>(key)];
    }

    std::uint64_t load_entry(std::uint64_t idx) const {
        const std::uint8_t* p = entries_.data() + idx * ENTRY_BYTES;
        return static_cast<std::uint64_t>(p[0])
             | (static_cast<std::uint64_t>(p[1]) << 8)
             | (static_cast<std::uint64_t>(p[2]) << 16)
             | (static_cast<std::uint64_t>(p[3]) << 24)
             | (static_cast<std::uint64_t>(p[4]) << 32)
             | (static_cast<std::uint64_t>(p[5]) << 40);
    }

    void store_entry(std::uint64_t idx, std::uint64_t entry) {
        std::uint8_t* p = entries_.data() + idx * ENTRY_BYTES;
        p[0] = static_cast<std::uint8_t>(entry);
        p[1] = static_cast<std::uint8_t>(entry >> 8);
        p[2] = static_cast<std::uint8_t>(entry >> 16);
        p[3] = static_cast<std::uint8_t>(entry >> 24);
        p[4] = static_cast<std::uint8_t>(entry >> 32);
        p[5] = static_cast<std::uint8_t>(entry >> 40);
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
                | (static_cast<std::uint64_t>(p[5]) << 40);
            std::uint8_t value = static_cast<std::uint8_t>(entry & 0b11);
            if (value == 0) continue;
            set_key(entry >> 2, value);
        }
    }

    std::vector<std::uint8_t> entries_;
    int n_;
    std::vector<std::uint64_t> suffix_;
    std::vector<ChunkRank> chunk_rank_;
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

};

class SparseMemo64 {
public:
    using Key = unsigned __int128;

    explicit SparseMemo64(int n, std::uint64_t expected_entries, std::uint64_t initial_capacity = 0)
        : n_(n),
          suffix_((static_cast<std::size_t>(n_) + 1) * SYMBOLS, 0),
          chunk_rank_(CHUNKS * SYMBOLS * CHUNK_KEYS) {
        init_suffix_counts();
        init_chunk_rank();
        std::uint64_t capacity = 1;
        std::uint64_t target = initial_capacity != 0 ? initial_capacity : expected_entries * 2;
        while (capacity < target) capacity <<= 1;
        entries_.assign(capacity * ENTRY_BYTES, 0);
        mask_ = capacity - 1;
    }

    std::uint8_t get_key(Key key) const {
        std::uint64_t idx = mix(key) & mask_;
        std::uint64_t probes = 1;
        while (true) {
            Key entry = load_entry(idx);
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

    void set_key(Key key, std::uint8_t value) {
        if ((filled_ + 1) * 10 >= capacity() * 7) {
            rehash(capacity() * 2);
        }

        std::uint64_t idx = mix(key) & mask_;
        std::uint64_t probes = 1;
        Key new_entry = (key << 2) | value;
        while (true) {
            Key entry = load_entry(idx);
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

    static constexpr std::uint64_t entry_bytes() {
        return ENTRY_BYTES;
    }

    Key make_key(Board64 black, Board64 white) const {
        Key rank = 0;
        int prev = EMPTY;
        for (int chunk = 0; chunk < CHUNKS; ++chunk) {
            std::uint32_t black_byte = static_cast<std::uint32_t>((black >> (chunk * CHUNK_BITS)) & 0xffULL);
            std::uint32_t white_byte = static_cast<std::uint32_t>((white >> (chunk * CHUNK_BITS)) & 0xffULL);
            const ChunkRank& cr = chunk_rank(chunk, prev, (black_byte << 8) | white_byte);
            if (cr.next < 0) {
                throw std::logic_error("attempted to memoize a board with adjacent black and white stones");
            }
            rank += cr.rank;
            prev = cr.next;
        }
        return rank;
    }

private:
    static constexpr int EMPTY = 0;
    static constexpr int BLACK = 1;
    static constexpr int WHITE = 2;
    static constexpr int SYMBOLS = 3;
    static constexpr int CHUNK_BITS = 8;
    static constexpr int CHUNKS = 8;
    static constexpr int CHUNK_KEYS = 1 << 16;
    static constexpr std::uint64_t ENTRY_BYTES = 11;

    struct ChunkRank {
        Key rank = 0;
        int8_t next = -1;
    };

    static std::uint64_t mix(Key x) {
        std::uint64_t low = static_cast<std::uint64_t>(x);
        std::uint64_t high = static_cast<std::uint64_t>(x >> 64);
        std::uint64_t y = low ^ (high + 0x9e3779b97f4a7c15ULL + (low << 6) + (low >> 2));
        y += 0x9e3779b97f4a7c15ULL;
        y = (y ^ (y >> 30)) * 0xbf58476d1ce4e5b9ULL;
        y = (y ^ (y >> 27)) * 0x94d049bb133111ebULL;
        return y ^ (y >> 31);
    }

    void init_suffix_counts() {
        for (int prev = 0; prev < SYMBOLS; ++prev) {
            suffix(n_, prev) = 1;
        }
        for (int pos = n_ - 1; pos >= 0; --pos) {
            for (int prev = 0; prev < SYMBOLS; ++prev) {
                Key total = 0;
                for (int symbol = 0; symbol < SYMBOLS; ++symbol) {
                    if (is_contact(prev, symbol)) continue;
                    total += suffix(pos + 1, symbol);
                }
                suffix(pos, prev) = total;
            }
        }
    }

    void init_chunk_rank() {
        for (int chunk = 0; chunk < CHUNKS; ++chunk) {
            int start = chunk * CHUNK_BITS;
            int end = std::min(n_, start + CHUNK_BITS);
            for (int prev = 0; prev < SYMBOLS; ++prev) {
                for (int black_byte = 0; black_byte < 256; ++black_byte) {
                    for (int white_byte = 0; white_byte < 256; ++white_byte) {
                        ChunkRank cr{};
                        int current = prev;
                        bool valid = true;
                        for (int pos = start; pos < end; ++pos) {
                            int bit = pos - start;
                            bool has_black = (black_byte & (1 << bit)) != 0;
                            bool has_white = (white_byte & (1 << bit)) != 0;
                            if (has_black && has_white) {
                                valid = false;
                                break;
                            }
                            int actual = has_black ? BLACK : has_white ? WHITE : EMPTY;
                            for (int symbol = 0; symbol < actual; ++symbol) {
                                if (is_contact(current, symbol)) continue;
                                cr.rank += suffix(pos + 1, symbol);
                            }
                            if (is_contact(current, actual)) {
                                valid = false;
                                break;
                            }
                            current = actual;
                        }
                        cr.next = valid ? static_cast<int8_t>(current) : -1;
                        chunk_rank(chunk, prev, (black_byte << 8) | white_byte) = cr;
                    }
                }
            }
        }
    }

    static bool is_contact(int left, int right) {
        return (left == BLACK && right == WHITE) || (left == WHITE && right == BLACK);
    }

    Key& suffix(int pos, int prev) {
        return suffix_[static_cast<std::size_t>(pos) * SYMBOLS + prev];
    }

    Key suffix(int pos, int prev) const {
        return suffix_[static_cast<std::size_t>(pos) * SYMBOLS + prev];
    }

    ChunkRank& chunk_rank(int chunk, int prev, int key) {
        return chunk_rank_[(static_cast<std::size_t>(chunk) * SYMBOLS + prev) * CHUNK_KEYS
                         + static_cast<std::size_t>(key)];
    }

    const ChunkRank& chunk_rank(int chunk, int prev, int key) const {
        return chunk_rank_[(static_cast<std::size_t>(chunk) * SYMBOLS + prev) * CHUNK_KEYS
                         + static_cast<std::size_t>(key)];
    }

    Key load_entry(std::uint64_t idx) const {
        const std::uint8_t* p = entries_.data() + idx * ENTRY_BYTES;
        Key value = 0;
        for (int i = ENTRY_BYTES - 1; i >= 0; --i) {
            value = (value << 8) | p[i];
        }
        return value;
    }

    void store_entry(std::uint64_t idx, Key entry) {
        std::uint8_t* p = entries_.data() + idx * ENTRY_BYTES;
        for (int i = 0; i < static_cast<int>(ENTRY_BYTES); ++i) {
            p[i] = static_cast<std::uint8_t>(entry >> (i * 8));
        }
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
            Key entry = 0;
            for (int j = ENTRY_BYTES - 1; j >= 0; --j) {
                entry = (entry << 8) | p[j];
            }
            std::uint8_t value = static_cast<std::uint8_t>(entry & 0b11);
            if (value == 0) continue;
            set_key(entry >> 2, value);
        }
    }

    std::vector<std::uint8_t> entries_;
    int n_;
    std::vector<Key> suffix_;
    std::vector<ChunkRank> chunk_rank_;
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
};

} // namespace minigo
