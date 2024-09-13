#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <type_traits>

#include "uint4_t.h"
#include "array_type.h"
#include "../log.h"

#define byte_size 8

namespace exlib {
    template<std::size_t N, class Word, class Allocator, bool Signed>
    struct int_base {
        static_assert(std::is_integral_v<Word>);;

        struct iterator;
        struct const_iterator;
        struct bit_reference;
        struct const_bit_reference;
        friend struct bit_reference;
        using reference = int_base<N, Word, Allocator, Signed>&;
        using const_reference = const int_base<N, Word, Allocator, Signed>&;
        using self_type = int_base<N, Word, Allocator, Signed>;

        inline static constexpr bool is_signed_v = Signed;
        inline static constexpr std::size_t word_size = std::is_same_v<uint4_t, Word> ? 4 : sizeof(Word) * byte_size; 
        inline static constexpr std::size_t array_size = (N + word_size - 1) / word_size;
        inline static constexpr std::size_t digits = N;
        inline static constexpr std::size_t digits10 = std::floor(std::log10(2) * (N - 1));

        using word_type = Word;
        using array_type = std::conditional_t<std::is_void_v<Allocator>, details::static_array<word_type, array_size>, details::dynamic_array<word_type, N, Allocator>>;
        using bit = bool;
        using pointer = word_type*;

        array_type _data;
        int_base() noexcept {
            
        }

        int_base(const_reference other) noexcept {
            std::copy(other._data.begin(), other._data.end(), _data.begin());
        }

        int_base(self_type&& other) noexcept {
            if (&other != this) {
                std::move(other._data.begin(), other._data.end(), _data.begin());
            }
        }

        template<typename I>
        requires std::is_integral_v<I>
        int_base(const I& i) noexcept {
            this->assign_integral(i);
        }

        std::string str() const noexcept {
            constexpr std::size_t M = digits10 * 4;
            int_base<M, uint4_t, Allocator, false> res;
            auto abs = this->abs();

            for (std::size_t i = 0; i < N; i++) {
                res[0] = abs._at(N - 1 - i);

                if (i != N - 1) {
                    for (size_t j = 0; j < res._data.size(); j++) {
                        if (res._data[j] >= 5) {
                            res._data[j] += 3;
                        }
                    }
                }

                if (i != N - 1) {
                    res <<= 1;
                }
            }

            std::stringstream ss;
            if (sign()) {
                ss << "-";
            }

            bool leading_zero = true;
            for (std::size_t i = res.array_size - 1; ~i; i--) {
                if (leading_zero && res._data[i] == 0) {
                    continue;
                } else {
                    leading_zero = false;
                    ss << res._data[i];
                }
            }
            return ss.str();
        }

        template<typename I>
        requires std::is_integral_v<I>
        reference operator=(const I& i) noexcept {
            return this->assign_integral(i);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        int_base(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            std::copy(other._data.cbegin(), other._data.cbegin() + std::min(array_size, other.array_size), this->_data.begin());
        }
 
        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator=(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->reset(other.filling_mask());
            for (std::size_t i = 0; i < std::min(N, M); i++) {
                this->_at(i) = other[i];
            }
            return *this;
        }

        reference operator=(const_reference other) noexcept {
            std::copy(other._data.cbegin(), other._data.cend(), this->_data.begin());
            return *this;
        }

        template<typename I>
        requires std::is_integral_v<I>
        reference assign_integral(I i) noexcept {
            using type = std::conditional_t<Signed, std::make_signed_t<I>, std::make_unsigned_t<I>>;
            type x = static_cast<type>(i);
            constexpr std::size_t M = sizeof(x) * byte_size;
            const bool x_sign = std::signbit(x);
            for (std::size_t j = 0; j < N; j++) {
                this->_at(j) = (j < M) ? (x >> j & 1) : x_sign;
            }
            return *this;
        }

        template<std::size_t M, class OAllocator, bool OSigned>
        bool _bitwise_equal(const int_base<M, Word, OAllocator, OSigned>& other) const noexcept {
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); i++) {
                Word lword = (i < array_size) ? _data[i] : this->filling_mask();
                Word rword = (i < other.array_size) ? other._data[i] : other.filling_mask();
                if (lword != rword) {
                    return false;
                }
            }
            return true;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, bool> _bitwise_equal(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other._at(i) : other.sign();
                if (lbit != rbit) {
                    return false;
                }
            }
            return true;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> operator&(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_and(other);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator&=(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->_bitwise_and_assign(other);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> operator|(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_or(other);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator|=(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->_bitwise_or_assign(other);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> operator^(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_xor(other);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator^=(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->_bitwise_xor_assign(other);
            return *this;
        }
        

        template<std::size_t M, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> _bitwise_and(const int_base<M, Word, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, OSigned> res;
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); i++) {
                res._data[i] = ((i < array_size) ? this->_data[i] : this->filling_mask()) & ((i < other.array_size) ? other._data[i] : other.filling_mask());
            }
            return res;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, int_base<std::max(N, M), Word, Allocator, Signed>> _bitwise_and(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                res[i] = ((i < N) ? this->_at(i) : this->sign()) & ((i < M) ? other._at(i) : other.sign());
            }
            return res;
        }

        template<std::size_t M>
        reference _bitwise_and_assign(const_reference other) noexcept {
            for (std::size_t i = 0; i < array_size; i++) {
                this->_data[i] &= (i < other.array_size) ? other._data[i] : other.filling_mask();
            }
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, reference> _bitwise_and_assign(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) &= (i < M) ? other._at(i) : other.sign();
            }
            return *this;
        }

        template<std::size_t M, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> _bitwise_or(const int_base<M, Word, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); i++) {
                res._data[i] = ((i < array_size) ? this->_data[i] : this->filling_mask()) | ((i < other.array_size) ? other._data[i] : other.filling_mask());
            }
            return res;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, int_base<std::max(N, M), Word, Allocator, Signed>> _bitwise_or(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other._at(i) : other.sign();
                res._at(i) = lbit | rbit;
            }
            return res;
        }

        template<std::size_t M>
        reference _bitwise_or_assign(const_reference other) noexcept {
            for (std::size_t i = 0; i < array_size; i++) {
                this->_data[i] |= (i < other.array_size) ? other._data[i] : other.filling_mask();
            }
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, reference> _bitwise_or_assign(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) |= (i < M) ? other._at(i) : other.sign();
            }
            return *this;
        }

        template<std::size_t M, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> _bitwise_xor(const int_base<M, Word, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); i++) {
                res._data[i] = ((i < array_size) ? this->_data[i] : this->filling_mask()) ^ ((i < other.array_size) ? other._data[i] : other.filling_mask());
            }
            return res;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, int_base<std::max(N, M), Word, Allocator, Signed>> _bitwise_xor(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            int_base<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                res[i] = ((i < N) ? this->_at(i) : this->sign()) ^ ((i < M) ? other._at(i) : other.sign());
            }
            return res;
        }

        template<std::size_t M>
        reference _bitwise_xor_assign(const_reference other) noexcept {
            for (std::size_t i = 0; i < array_size; i++) {
                this->_data[i] ^= (i < other.array_size) ? other._data[i] : other.filling_mask();
            }
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, reference> _bitwise_xor_assign(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) ^= (i < M) ? other._at(i) : other.sign();
            }
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator+=(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
        requires std::is_integral_v<I>
        reference operator+=(const I& x) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            auto val = static_cast<std::conditional_t<Signed, std::make_signed_t<I>, std::make_unsigned_t<I>>>(x);
            const bool val_sign = std::signbit(val);
            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator-=(const int_base<M, OWord, OAllocator, OSigned>& other) noexcept {
            return _bitwise_sub_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
        requires std::is_integral_v<I>
        reference operator-=(const I& x) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            auto val = static_cast<std::conditional_t<Signed, std::make_signed_t<I>, std::make_unsigned_t<I>>>(x);
            const bool val_sign = std::signbit(val);
            return _bitwise_sub_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template <std::size_t M, class OWord, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> operator+(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return this->_bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
            requires std::is_integral_v<I>
        int_base<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator+(const I& val) const noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool val_sign = std::is_signed_v<I> ? std::signbit(val) : 0;
            return _bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<typename I>
            requires std::is_integral_v<I>
        friend int_base<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator+(const I& lhs, const_reference rhs) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool lhs_sign = std::is_signed_v<I> ? std::signbit(lhs) : 0;
            return _bitwise_add<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        int_base<std::max(N, M), Word, Allocator, Signed> operator-(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
        requires std::is_integral_v<I>
        friend int_base<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator-(const I& lhs, const_reference rhs) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool lhs_sign = std::is_signed_v<I> ? std::signbit(lhs) : 0;
            return _bitwise_sub<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<typename T>
        requires std::is_integral_v<T>
        int_base<std::max(N, sizeof(T) * byte_size), Word, Allocator, Signed> operator-(const T& val) const noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bool val_sign = std::is_signed_v<T> ? std::signbit(val) : 0;
            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator==(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return this->_bitwise_equal(other);
        }

        template<typename I>
        requires std::is_integral_v<I>
        bool operator==(const I& val) const noexcept {
            return this->_bitwise_equal(int_base<sizeof(I) * byte_size, Word, Allocator, Signed>(val));
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator!=(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return !(this->_bitwise_equal(other));
        }

        template<typename I>
        requires std::is_integral_v<I>
        bool operator!=(const I& val) const noexcept {
            return !(this->_bitwise_equal(int_base<sizeof(I) * byte_size, Word, Allocator, Signed>(val)));
        }

        self_type operator<<(auto&& other) const noexcept {
            auto copy = *this;
            copy <<= other;
            return copy;
        }

        reference operator<<=(auto&& x) noexcept {
            if (x >= N) {
                this->reset(0);
            } else {
                for (std::size_t i = N - 1; ~i; i--) {
                    this->_at(i) = (i < static_cast<std::size_t>(x)) ? 0 : this->_at(i - static_cast<std::size_t>(x));
                }
            }
            return *this;
        }

        self_type operator>>(auto&& x) const noexcept {
            auto copy = *this;
            copy >>= x;
            return copy;
        }

        reference operator>>=(auto&& x) noexcept {
            if (x >= N) {
                this->reset(this->filling_mask());
            } else {
                for (std::size_t i = 0; i < N; i++) {
                    this->_at(i) = (i + static_cast<std::size_t>(x) < N) ? this->_at(i + static_cast<std::size_t>(x)) : sign();
                }
            }   
            return *this;
        }

        self_type operator~() const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < N; i++) {
                copy[i].flip();
            }
            return copy;
        }

        self_type operator++() noexcept {
            auto copy = *this;
            copy += 1;
            return copy;
        }

        reference operator++(int) noexcept {
            *this += 1;
            return *this;
        }

        self_type operator--() noexcept {
            auto copy = *this;
            copy -= 1;
            return copy;
        }

        reference operator--(int) noexcept {
            *this -= 1;
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator<(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            if (this->sign() != other.sign()) {
                return this->sign() == 1;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                if (lbit != rbit) {
                    return lbit < rbit;
                }
            }
            return false;
        }

        template<typename I>
        requires std::is_integral_v<I>
        friend bool operator<(const I& lhs, const_reference rhs) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bit lhs_sign = std::is_signed_v<I> ? std::signbit(lhs) : 0;
            if (lhs_sign != rhs.sign()) {
                return lhs_sign == 1;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < M) ? (lhs >> i & 1) : lhs_sign;
                bit rbit = (i < N) ? rhs[i] : rhs.sign();

                if (lbit != rbit) {
                    return lbit < rbit;
                }
            }
            return false;
        }

        template<typename I>
        requires std::is_integral_v<I>
        bool operator<(const I& i) const noexcept {
            return *this < int_base<sizeof(I) * byte_size, Word, Allocator, Signed>(i);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator<=(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return *this == other || *this < other;
        }

        template<typename I>
        requires std::is_integral_v<I>
        friend bool operator<=(const I& lhs, const_reference rhs) noexcept {
            return !(rhs > lhs);
        }

        template<typename I>
            requires std::is_integral_v<I>
        bool operator<=(const I& val) const noexcept {
            return *this <= int_base<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator>(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            if (this->sign() != other.sign()) {
                return this->sign() == 0;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < N) ? this->_at(i) : sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                if (lbit != rbit) {
                    return lbit > rbit;
                }
            }
            return false;
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend bool operator>(const T& lhs, const_reference rhs) noexcept {
            return !(rhs <= lhs);
        }

        template<typename I>
            requires std::is_integral_v<I>
        bool operator>(const I& i) const noexcept {
            return *this > int_base<sizeof(I) * byte_size, Word, Allocator, Signed>(i);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator>=(const int_base<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return *this == other || *this > other;
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend bool operator>=(const T& lhs, const_reference rhs) noexcept {
            return !(rhs < lhs);
        }

        template<typename T>
        requires std::is_integral_v<T>
        bool operator>=(const T& val) const noexcept {
            return *this >= int_base<sizeof(T) * byte_size, Word, Allocator, Signed>(val);
        }

        self_type abs() const noexcept {
            if (!Signed) return *this;

            return (sign() == 0) ? *this : ~(*this) + self_type(1);
        }

        bit sign() const noexcept {
            if (!Signed) return 0;
            return this->_at(N - 1);
        }

        inline void reset(word_type value = 0) noexcept {
            this->_data.reset(value);
        }

        void swap(const_reference other) noexcept {
            std::swap(_data, other._data);
        }

        word_type filling_mask() const noexcept {
            if constexpr (!Signed) {
                return 0;
            }

            return sign() == static_cast<bit>(0) ? static_cast<word_type>(0) : static_cast<word_type>(-1);
        }

        void setsign(bit sign) noexcept {
            if (!Signed) return;

            if (sign != this->sign()) {
                log_info("TODO: setsign");
            }
        }

        constexpr std::size_t size() const noexcept {
            return N;
        }

        inline bit_reference _at(std::size_t pos) {
            return bit_reference(*this, pos);
        }

        inline bool _at(std::size_t pos) const {
            return (this->_get_word(pos) & (1 << _which_bit(pos))) != static_cast<word_type>(0);
        }

        bit_reference at(std::size_t pos) {
            if (pos >= N) [[unlikely]] {
                throw std::out_of_range("pos out of range! " + std::to_string(pos) + " / " + std::to_string(N));
            }
            return bit_reference(*this, pos);
        }

        bool at(std::size_t pos) const {
            if (pos >= N) [[unlikely]] {
                throw std::out_of_range("pos out of range! " + std::to_string(pos) + " / " + std::to_string(N));
            }
            return (this->_get_word(pos) & (1 << _which_bit(pos))) != static_cast<word_type>(0);
        }

        iterator begin() noexcept {
            return iterator(*this, 0);
        }

        iterator end() noexcept {
            return iterator(*this, N);
        }

        const_iterator cbegin() const noexcept {
            return const_iterator(*this, 0);
        }

        const_iterator cend() const noexcept {
            return const_iterator(*this, N);
        }
        
        bit_reference operator[](std::size_t pos) noexcept {
            return bit_reference(*this, pos);
        }

        bool operator[](std::size_t pos) const noexcept {
            return (this->_get_word(pos) & (1 << _which_bit(pos))) != static_cast<word_type>(0);
        }

        word_type& _get_word(std::size_t pos) noexcept {
            return _data[_which_word(pos)];
        }

        const word_type& _get_word(std::size_t pos) const noexcept {
            return _data[_which_word(pos)];
        }

        static std::size_t _which_word(std::size_t pos) noexcept {
            return pos / word_size;
        }

        static std::size_t _which_bit(std::size_t pos) noexcept {
            return pos % word_size;
        }

        static std::pair<bit, bit> _full_add(bit x, bit y, bit c) noexcept {
            bit sum = x ^ y ^ c;
            bit carry = (x & y) | ((x ^ y) & c);
            return {sum, carry};
        }

        static std::pair<bit, bit> _full_sub(bit x, bit y, bit b) noexcept {
            bit diff = x ^ y ^ b;
            bit borrow = ((!x) & (y | b)) | (y & b);
            return {diff, borrow};
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline static int_base<std::max(N, M), Word, Allocator, Signed> _bitwise_add(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            int_base<std::max(N, M), Word, Allocator, Signed> res;
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [sum, car] = _full_add(lbitfunc(i), rbitfunc(i), carry);
                carry = car;
                res[i] = sum;
            }
            return res;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline reference _bitwise_add_assign(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit carry = 0;
            
            for (std::size_t i = 0; i < N; i++) {
                auto [sum, car] = _full_add(lbitfunc(i), rbitfunc(i), carry);
                carry = car;
                if (i < N) {
                    this->_at(i) = sum;
                }
            }
            return *this;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline static int_base<std::max(N, M), Word, Allocator, Signed> _bitwise_sub(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            int_base<std::max(N, M), Word, Allocator, Signed> res;
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [diff, borr] = _full_sub(lbitfunc(i), rbitfunc(i), borrow);
                borrow = borr;
                res[i] = diff;
            }
            return res;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline reference _bitwise_sub_assign(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit borrow = 0;
            
            for (std::size_t i = 0; i < N; i++) {
                auto [diff, borr] = _full_sub(lbitfunc(i), rbitfunc(i), borrow);
                borrow = borr;
                if (i < N) {
                    this->_at(i) = diff;
                }
            }
            return *this;
        }

        std::bitset<N> to_std_bitset() const noexcept {
            std::bitset<N> res; 
            res.reset();
            for (std::size_t i = 0; i < N; i++) {
                res[i] = this->_at(i);
            }
            return res;
        }

        struct iterator {
            int_base& _obj;
            std::size_t _index;
            using value_type = struct bit_reference;
            using reference = struct bit_reference;
            using difference_type = std::size_t;
            using pointer = reference*;
            using iterator_category = std::random_access_iterator_tag;
            
            iterator(int_base& b, std::size_t pos) noexcept
            : _obj(b), _index(pos) {}

            reference operator*() noexcept {
                return reference(_obj, _index);
            }

            iterator operator++() noexcept {
                auto copy = *this;
                _index++;            
                return copy;
            }

            iterator& operator++(int) noexcept {
                _index++;
                return *this;
            }

            iterator& operator+=(difference_type n) {
                _index += n;
                return *this;
            }

            iterator operator+(difference_type x) const noexcept {
                auto copy = *this;
                copy._index += x;
                return copy;
            }

            iterator operator-(difference_type x) const noexcept {
                auto copy = *this;
                copy._index -= x;
                return copy;
            }

            difference_type operator-(iterator other) const noexcept {
                return _index - other._index;
            }

            bool operator!=(const iterator& other) const noexcept {
                return _index != other._index;
            }

            bool operator==(const iterator& other) const noexcept {
                return _index == other._index;
            }
        };

        struct const_iterator {
            const int_base& _obj;
            std::size_t _index;
            using value_type = struct bit_reference;
            using reference = struct bit_reference;
            using const_reference = struct const_bit_reference;
            using difference_type = std::size_t;
            using pointer = reference*;
            using iterator_category = std::random_access_iterator_tag;
            
            const_iterator(const int_base& b, std::size_t pos) noexcept
            : _obj(b), _index(pos) {}

            const_reference operator*() const noexcept {
                return const_reference(_obj, _index);
            }

            const_iterator operator++() noexcept {
                auto copy = *this;
                _index++;            
                return copy;
            }

            const_iterator& operator++(int) noexcept {
                _index++;
                return *this;
            }

            const_iterator operator+(difference_type x) noexcept {
                auto copy = *this;
                copy._index += x;
                return copy;
            }

            const_iterator operator-(difference_type x) const noexcept {
                auto copy = *this;
                copy._index -= x;
                return copy;
            }

            difference_type operator-(const_iterator other) const noexcept {
                return _index - other._index;
            }

            bool operator!=(const const_iterator& other) const noexcept {
                return _index != other._index;
            }

            bool operator==(const const_iterator& other) const noexcept {
                return _index == other._index;
            }
        };

        struct bit_reference {
            friend struct int_base;
            
            word_type* _word;
            std::size_t _b_pos;

            bit_reference(int_base& b, std::size_t pos) noexcept {
                _word = &(b._get_word(pos));
                _b_pos = int_base::_which_bit(pos);
            }

            bit_reference(const bit_reference&) noexcept = default;
            ~bit_reference() noexcept { }

            bit_reference& operator&=(bool x) noexcept {
                return *this = (this->value()) & x;
            }

            bit_reference& operator|=(bool x) noexcept {
                return *this = (this->value()) | x;
            }

            bit_reference& operator^=(bool x) noexcept {
                return *this = (this->value()) ^ x;
            }

            bit_reference& operator=(bool x) noexcept {
                if (x) *_word |= 1 << _b_pos;
                else *_word &= ~(1 << _b_pos);
                return *this;
            }

            bit_reference& operator=(const bit_reference &other) noexcept {
                if ((*other._word) & (1 << (other._b_pos))) 
                    *_word |= (1 << _b_pos);
                else 
                    *_word &= ~(1 << _b_pos);
                return *this;
            }

            bool operator~() const noexcept {
                return ((*_word) & (1 << _b_pos)) == 0;
            }

            operator bool() const noexcept {
                return ((*_word) & (1 << _b_pos)) != 0; 
            }

            bool value() const noexcept {
                return ((*_word) & (1 << _b_pos)) != 0; 
            }

            bit_reference& flip() noexcept {
                *_word ^= (1 << _b_pos);
                return *this;
            }
        };

        struct const_bit_reference {
            const word_type* _word;
            std::size_t _b_pos;

            const_bit_reference(const int_base& b, std::size_t pos) noexcept {
                _word = &b._get_word(pos);
                _b_pos = int_base::_which_bit(pos);
            }

            const_bit_reference(const const_bit_reference&) noexcept = default;
            ~const_bit_reference() noexcept { }
        
            bool operator~() const noexcept {
                return ((*_word) & (1 << _b_pos)) == 0;
            }

            operator bool() const noexcept {
                return ((*_word) & (1 << _b_pos)) != 0; 
            }
        };

        friend std::ostream& operator<<(std::ostream& os, const int_base& val) noexcept {
            os << val.str();
            return os;
        }
    };
    
}