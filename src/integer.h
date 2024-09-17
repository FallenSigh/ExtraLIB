#pragma once
#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include "details/uint4_t.h"
#include "details/array_type.h"

#define byte_size 8

namespace exlib {
    // declaration
    template<std::size_t N, class Word, class Allocator, bool Signed>
    struct integer;

    template<std::size_t N, class Word = unsigned int, class Allocator = std::allocator<Word>>
    using nints = integer<N, Word, Allocator, true>;
    
    template<std::size_t N, class Word = unsigned int, class Allocator = std::allocator<Word>>
    using unints = integer<N, Word, Allocator, false>;

    // type traits
    template<typename T>
    struct is_integer : std::false_type {};

    template<std::size_t N, class Word, class Allocator, bool Signed>
    struct is_integer<integer<N, Word, Allocator, Signed>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_integer_v = is_integer<T>::value;
}

namespace exlib {
    template<std::size_t N, class Word, class Allocator, bool Signed>
    struct integer {
        static_assert(std::is_integral_v<Word>);;

        struct iterator;
        struct const_iterator;
        struct bit_reference;
        struct bit_const_reference;
        friend struct bit_reference;
        using reference = integer<N, Word, Allocator, Signed>&;
        using const_reference = const integer<N, Word, Allocator, Signed>&;
        using self_type = integer<N, Word, Allocator, Signed>;

        inline static constexpr bool is_signed_v = Signed;
        inline static constexpr std::size_t word_size = std::is_same_v<details::uint4_t, Word> ? 4 : sizeof(Word) * byte_size; 
        inline static constexpr std::size_t array_size = (N + word_size - 1) / word_size;
        inline static constexpr std::size_t digits = N;
        inline static constexpr std::size_t digits10 = std::floor(std::log10(2) * N) + 1;

        using word_type = Word;
        using array_type = std::conditional_t<std::is_void_v<Allocator>, details::static_array<word_type, array_size>, details::dynamic_array<word_type, N, Allocator>>;
        using bit = bool;
        using pointer = word_type*;

        array_type _data;

        integer() noexcept {
        }

        integer(const_reference other) noexcept {
            this->assign(other);
        }

        integer(self_type&& other) noexcept {
            if (&other != this) {
                std::move(other._data.begin(), other._data.end(), _data.begin());
            }
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->reset(other.filling_mask());
            for (std::size_t i = 0; i < N; ++i) {
                this->_at(i) = (i < M) ? other._at(i) : other.sign();
            }
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer(const I& i) noexcept {
            this->assign_integral(i);
        }

        integer(const std::string& str) noexcept {
            this->rd_string(str);
        }

        reference rd_bin_string(std::string s) noexcept {
            this->reset(0);
            bool neg = false;
            if (s[0] == '-') {
                neg = true;
                s = s.substr(1, s.size() - 1);
            }
            for (std::size_t i = 0; i < N; ++i) {
                this->_at(i) = (i < s.size()) ? (s[s.size() - 1 - i] - '0'): 0;
            }

            if (neg) {
                *this = ~(*this) + 1;
            }
            return *this;
        }

        reference rd_string(std::string s) noexcept {
            this->reset(0);
            bool neg = false;
            if (s[0] == '-') {
                neg = true;
                s = s.substr(1, s.size() - 1);
            }
            
            constexpr std::size_t M = digits10 * 4;
            integer<M, details::uint4_t, void, false> src;
            for (std::size_t i = 0; i < digits10; ++i) {
                src._data[i] = (i < s.size()) ? (s[s.size() - 1 - i] - '0') : 0;
            }

            // Reverse double dabble
            for (std::size_t i = 0; i < N; ++i) {
                this->_at(N - 1) = src[0];
                src >>= 1;
                if (i != N - 1) {
                    for (std::size_t j = 0; j < src.array_size; ++j) {
                        if (src._data[j] >= 8) {
                            src._data[j] -= 3;
                        }
                    }
                    *this >>= 1;
                }
            }

            if (neg) {
                *this = ~(*this) + self_type(1);
            }

            return *this;
        }

        std::string as_mantissa_str() const noexcept {
            constexpr std::size_t M = std::size_t(N / std::log2(10)) * 4;
            integer<M, details::uint4_t, void, false> res = 0;
            
            for (std::size_t i = 0; i < N; ++i) {
                res[M - 1] = this->_at(i);

                for (std::size_t j = 0; j < res.array_size; ++j) {
                    if (res._data[j] >= 8u) {
                        res._data[j] -= 3u;
                    }
                }
                if (i != N - 1) {
                    res >>= 1;
                }
            }

            std::stringstream ss;
            for (std::size_t i = res.array_size - 1; ~i; i--) {
                ss << res._data[i].template value<int>();
            }
            return ss.str();
        } 

        std::string str() const noexcept {
            constexpr std::size_t M = digits10 * 4;
            integer<M, details::uint4_t, void, false> res;
            auto&& abs = this->abs();

            // double dabble
            for (std::size_t i = 0; i < N; ++i) {
                res[0] = abs._at(N - 1 - i);

                if (i != N - 1) {
                    for (size_t j = 0; j < res._data.size(); ++j) {
                        if (res._data[j] >= 5) {
                            res._data[j] += 3;
                        }
                    }

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

            if (ss.str().empty()) {
                ss << "0";
            }
            return ss.str();
        }

        std::string bin() const noexcept {
            std::string res;
            for (std::size_t i = N - 1; ~i; i--) {
                res += ('0' + this->_at(i));
            }
            return res;
        }

        std::string hex() const noexcept {
            std::stringstream ss;
            std::size_t n = N / 4;
            std::size_t r = N % 4;

            for (std::size_t i = 0; i < n; ++i) {
                std::uint32_t x = this->_at(i * 4) + (this->_at(i * 4 + 1) * 2) + (this->_at(i * 4 + 2) * 4) + (this->_at(i * 4 + 3)) * 8;
                ss << std::hex << x;                
            }
            uint32_t x = ((r >= 0) ? this->_at(n * 4) : 0) + ((r >= 1) ? this->_at(n * 4 + 1) * 2 : 0) + ((r >= 2) ? this->_at(n * 4 + 2) * 4 : 0);
            ss << std::hex << x;

            std::string res;
            ss >> res;
            std::reverse(res.begin(), res.end());
            return res;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        reference operator=(const I& i) noexcept {
            return this->assign_integral(i);
        }
 
        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->reset(other.filling_mask());
            for (std::size_t i = 0; i < N; ++i) {
                this->_at(i) = (i < M) ? other._at(i) : other.sign();
            }
            
            return *this;
        }

        reference operator=(const_reference other) noexcept {
            return assign(other);
        }

        reference operator=(self_type&& other) noexcept {
            if (&other != this) {
                std::move(other._data.begin(), other._data.end(), _data.begin());
            }
            return *this;
        }

        reference assign(const_reference other) noexcept {
            std::copy(other._data.cbegin(), other._data.cend(), this->_data.begin());
            return *this;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        reference assign_integral(I i) noexcept {
            using type = std::conditional_t<Signed, std::make_signed_t<I>, std::make_unsigned_t<I>>;
            type x = static_cast<type>(i);
            constexpr std::size_t M = sizeof(x) * byte_size;
            const bool x_sign = std::signbit(x);
            for (std::size_t j = 0; j < N; ++j) {
                this->_at(j) = (j < M) ? (x >> j & 1) : x_sign;
            }
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator*(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            auto&& lhs_abs = integer<std::max(N, M), Word, Allocator, Signed>(this->abs());
            auto&& rhs_abs = other.abs();

            integer<std::max(N, M), Word, Allocator, Signed> res = 0;
            for (std::size_t i = 0; i < M; ++i) {
                if (rhs_abs[i]) {
                    res += (lhs_abs << i);
                }
            }
            if (this->sign() != other.sign()) {
                res = ~res + integer<std::max(N, M), Word, Allocator, Signed>(1);
            }
            return res;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator*(const I& val) const noexcept {
            return *this * integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend integer<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator*(const I& lhs, const_reference rhs) noexcept {
            return integer<sizeof(I) * byte_size, Word, Allocator, Signed>(lhs) * rhs;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator/(const integer<M, OWord, OAllocator, OSigned>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto&& lhs_abs = this->abs();
            auto&& rhs_abs = other.abs();

            integer<std::max(N, M), Word, Allocator, Signed> quotient = 0;
            integer<std::max(N, M), Word, Allocator, Signed> remainder = 0;

            for (std::size_t i = 0; i < N; ++i) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                    quotient[N - 1 - i] = 1;
                }
            }

            if (this->sign() != other.sign()) {
                quotient = ~quotient + integer<std::max(N, M), Word, Allocator, Signed>(1);
            }

            return quotient;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator/(const I& val) const {
            return *this / integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend integer<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator/(const I& lhs, const_reference rhs) noexcept {
            return integer<sizeof(I) * byte_size, Word, Allocator, Signed>(lhs) / rhs;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator%(const integer<M, OWord, OAllocator, OSigned>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            integer<M, OWord, OAllocator, OSigned> rhs_abs = other.abs();

            integer<std::max(N, M), Word, Allocator, Signed> remainder = 0;

            for (std::size_t i = 0; i < N; ++i) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder -= rhs_abs;
                }
            }

            if (this->sign() != remainder.sign()) {
                remainder = ~remainder + integer<std::max(N, M), Word, Allocator, Signed>(1);
            }

            return remainder;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator%(const I& val) const {
            return *this % integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend integer<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator%(const I& lhs, const_reference rhs) noexcept {
            return integer<sizeof(I) * byte_size, Word, Allocator, Signed>(lhs) % rhs;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator*=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            auto lhs_abs = this->abs();
            auto rhs_abs = other.abs();
            
            self_type result = 0;
            for (std::size_t i = 0; i < M; ++i) {
                if (rhs_abs[i]) {
                    result += (lhs_abs << i);
                }
            }

            if (this->sign() != other.sign()) {
                result = ~result + 1;
            }
            *this = std::move(result);
            return *this;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        reference operator*=(const I& val) noexcept {
            *this *= integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator/=(const integer<M, OWord, OAllocator, OSigned>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }
            auto lhs_abs = this->abs();
            integer<M, OWord, OAllocator, OSigned> rhs_abs = other.abs();

            self_type quotient = 0;
            self_type remainder = 0;

            for (std::size_t i = 0; i < N; ++i) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                    quotient[N - 1 - i] = 1;
                }
            }

            if (this->sign() != other.sign()) {
                quotient = ~quotient + integer<std::max(N, M), Word, Allocator, Signed>(1);
            }

            *this = std::move(quotient);
            return *this;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        reference operator/=(const I& val) {
            *this /= integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator%=(const integer<M, OWord, OAllocator, OSigned>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            integer<M, OWord, OAllocator, OSigned> rhs_abs = other.abs();

            self_type remainder = 0;

            for (std::size_t i = 0; i < N; ++i) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder -= rhs_abs;
                }
            }

            if (this->sign() != remainder.sign()) {
                remainder = ~remainder + 1;
            }

            *this = std::move(remainder);
            return *this;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        reference operator%=(const I& val) {
            *this %= integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bit bitwise_add_assign(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            bit carry = 0;
            for (std::size_t i = 0; i < N; ++i) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other._at(i) : other.sign();
                auto [sum, car] = _full_add(lbit, rbit, carry);
                carry = car;
                this->_at(i) = sum;
            }
            return carry;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bit bitwise_sub_assign(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); ++i) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other._at(i) : other.sign();
                auto [diff, borr] = _full_sub(lbit, rbit, borrow);
                borrow = borr;
                if (i < N) {
                    this->_at(i) = diff;
                }
            }
            return borrow;
        }

        template<std::size_t M, class OAllocator, bool OSigned>
        bool _bitwise_equal(const integer<M, Word, OAllocator, OSigned>& other) const noexcept {
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); ++i) {
                Word lword = (i < array_size) ? _data[i] : this->filling_mask();
                Word rword = (i < other.array_size) ? other._data[i] : other.filling_mask();
                if (lword != rword) {
                    return false;
                }
            }
            return true;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        requires (!std::is_same_v<Word, OWord>)
        bool _bitwise_equal(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            for (std::size_t i = 0; i < std::max(N, M); ++i) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other._at(i) : other.sign();
                if (lbit != rbit) {
                    return false;
                }
            }
            return true;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator&(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_ops(other, [](auto& l, auto& r){ return l & r; });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator&(const I& i) const noexcept {
            integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> res = *this;
            return res._bitwise_ops(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i), [](const auto& l, const auto& r){ return l & r; });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator&=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->_bitwise_ops_assign(other, [](auto &l, const auto& r){ l &= r; });
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator|(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_ops(other, [](const auto& l, const auto& r){ return l | r; });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator|(const I& i) const noexcept {
            integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> res = *this;
            return res._bitwise_ops(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i), [](const auto& l, const auto& r){ return l | r; });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator|=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            return _bitwise_ops_assign(other, [](auto& l, const auto& r){ l |= r;});
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator^(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_ops(other, [](const auto&l, const auto& r){ return l ^ r; });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator^(const I& i) const noexcept {
            integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> res = *this;
            return res._bitwise_ops(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i), [](const auto& l, const auto& r) { return l ^ r; });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator^=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            return _bitwise_ops_assign(other, [](auto& l, const auto& r) { l ^= r; });
        }
        
        template<std::size_t M, class OAllocator, bool OSigned, class Func>
        integer<std::max(N, M), Word, Allocator, Signed> 
        _bitwise_ops(const integer<M, Word, OAllocator, OSigned>& other, Func op) const noexcept {
            integer<std::max(N, M), Word, Allocator, OSigned> res;
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); ++i) {
                auto lword = ((i < array_size) ? this->_data[i] : this->filling_mask());
                auto rword = ((i < other.array_size) ? other._data[i] : other.filling_mask());
                res._data[i] = op(lword, rword);
            }
            return res;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned, class Func>
        std::enable_if_t<!std::is_same_v<Word, OWord>, integer<std::max(N, M), Word, Allocator, Signed>> 
        _bitwise_ops(const integer<M, OWord, OAllocator, OSigned>& other, Func op) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(N, M); ++i) {
                bit lbit = ((i < N) ? this->_at(i) : this->sign());
                bit rbit = ((i < M) ? other._at(i) : other.sign());
                res[i] = op(lbit, rbit);
            }
            return res;
        }

        template<std::size_t M, class OAllocator, bool OSigned, class Func>
        reference _bitwise_ops_assign(const integer<M, Word, OAllocator, OSigned>& other, Func op) noexcept {
            for (std::size_t i = 0; i < array_size; ++i) {
                word_type rword = ((i < other.array_size) ? other._data[i] : other.filling_mask());
                op(_data[i], rword);
            }
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned, class Func>
        requires (!std::is_same_v<Word, OWord>)
        reference _bitwise_ops_assign(const integer<M, OWord, OAllocator, OSigned>& other, Func op) noexcept {
            for (std::size_t i = 0; i < N; ++i) {
                auto rbit = ((i < M) ? other._at(i) : other.sign());
                auto lbit = this->_at(i);
                op(lbit, rbit);
            }
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator+=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        reference operator+=(const I& x) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            auto val = static_cast<std::conditional_t<Signed, std::make_signed_t<I>, std::make_unsigned_t<I>>>(x);
            const bool val_sign = std::signbit(val);
            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator-=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            return _bitwise_sub_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        reference operator-=(const I& x) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            auto val = static_cast<std::conditional_t<Signed, std::make_signed_t<I>, std::make_unsigned_t<I>>>(x);
            const bool val_sign = std::signbit(val);
            return _bitwise_sub_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template <std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator+(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return this->_bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
            requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator+(const I& val) const noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool val_sign = std::is_signed_v<I> ? std::signbit(val) : 0;
            return _bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<typename I>
            requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend integer<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator+(const I& lhs, const_reference rhs) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool lhs_sign = std::is_signed_v<I> ? std::signbit(lhs) : 0;
            return _bitwise_add<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator-(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend integer<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator-(const I& lhs, const_reference rhs) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool lhs_sign = std::is_signed_v<I> ? std::signbit(lhs) : 0;
            return _bitwise_sub<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator-(const I& val) const noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool val_sign = std::is_signed_v<I> ? std::signbit(val) : 0;
            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator==(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return this->_bitwise_equal(other);
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        bool operator==(const I& val) const noexcept {
            return this->_bitwise_equal(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val));
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator!=(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return !(this->_bitwise_equal(other));
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        bool operator!=(const I& val) const noexcept {
            return !(this->_bitwise_equal(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val)));
        }

        self_type operator<<(auto&& other) const noexcept {
            auto copy = *this;
            copy <<= other;
            return copy;
        }

        reference operator<<=(auto&& x) {
            if (x < 0) {
                throw std::runtime_error("left shifted bit count is negative!");
            }
            if (static_cast<std::size_t>(x) >= N) {
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

        reference operator>>=(auto&& x) {
            if (x < 0) {
                throw std::runtime_error("right shifted bit count is negative!");
            }
            if (static_cast<std::size_t>(x) >= N) {
                this->reset(this->filling_mask());
            } else {
                for (std::size_t i = 0; i < N; ++i) {
                    this->_at(i) = (i + static_cast<std::size_t>(x) < N) ? this->_at(i + static_cast<std::size_t>(x)) : sign();
                }
            }   
            return *this;
        }

        self_type operator~() const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < N; ++i) {
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

        template<typename I, class Func>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        static bool _bitwise_compare(const I& lhs, const_reference rhs, Func op) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bit lhs_sign = std::is_signed_v<I> ? std::signbit(lhs) : 0;
            if (lhs_sign != rhs.sign()) {
                return lhs_sign == 1;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < M) ? (lhs >> i & 1) : lhs_sign;
                bit rbit = (i < N) ? rhs[i] : rhs.sign();

                if (lbit != rbit) {
                    return op(lbit, rbit);
                }
            }
            return false;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned, class Func>
        bool _bitwise_compare(const integer<M, OWord, OAllocator, OSigned>& other, Func op) const noexcept {
            if (this->sign() != other.sign()) {
                return this->sign() == 0;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < N) ? this->_at(i) : sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                if (lbit != rbit) {
                    return op(lbit, rbit);
                }
            }
            return false;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator<(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return _bitwise_compare(other, [](const auto& l, const auto& r){ return l < r; });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend bool operator<(const I& lhs, const_reference rhs) noexcept {
            return _bitwise_compare(lhs, rhs, [](const auto& l, const auto& r){ return l < r; });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        bool operator<(const I& i) const noexcept {
            return *this < integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator<=(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return *this == other || *this < other;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend bool operator<=(const I& lhs, const_reference rhs) noexcept {
            return !(rhs > lhs);
        }

        template<typename I>
            requires std::is_integral_v<I> && (!is_integer_v<I>)
        bool operator<=(const I& val) const noexcept {
            return *this <= integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator>(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return _bitwise_compare(other, [](const auto& l, const auto& r){ return l > r; });
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend bool operator>(const I& lhs, const_reference rhs) noexcept {
            return _bitwise_compare(lhs, rhs, [](const auto& l, const auto& r) { return l > r; });
        }

        template<typename I>
            requires std::is_integral_v<I> && (!is_integer_v<I>)
        bool operator>(const I& i) const noexcept {
            return *this > integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator>=(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return *this == other || *this > other;
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        friend bool operator>=(const I& lhs, const_reference rhs) noexcept {
            return !(rhs < lhs);
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        bool operator>=(const I& val) const noexcept {
            return *this >= integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        self_type abs() const noexcept {
            if constexpr (!Signed) return *this;

            return (sign() == 0) ? *this : ~(*this) + self_type(1);
        }

        bit sign() const noexcept {
            if constexpr (!Signed) return 0;
            return this->_at(N - 1);
        }

        inline void reset(word_type value = 0) noexcept {
            this->_data.reset(value);
        }

        void swap(reference other) noexcept {
            std::swap(_data, other._data);
        }

        auto filling_mask() const noexcept {
            if constexpr (!Signed) {
                return 0;
            }

            return sign() == static_cast<bit>(0) ? (0) : (-1);
        }

        inline constexpr static std::size_t size() noexcept {
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

        inline word_type& _get_word(std::size_t pos) noexcept {
            return _data[_which_word(pos)];
        }

        inline const word_type& _get_word(std::size_t pos) const noexcept {
            return _data[_which_word(pos)];
        }

        inline static std::size_t _which_word(std::size_t pos) noexcept {
            return pos / word_size;
        }

        inline static std::size_t _which_bit(std::size_t pos) noexcept {
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

        static self_type max_value() noexcept {
            static integer<N, Word, Allocator, Signed> res;
            std::fill(std::begin(res._data), std::end(res._data), -1);
            if (Signed) res._at(N - 1) = 0;
            return res;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline static integer<std::max(N, M), Word, Allocator, Signed> _bitwise_add(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            integer<std::max(N, M), Word, Allocator, Signed> res;
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); ++i) {
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
            
            for (std::size_t i = 0; i < N; ++i) {
                auto [sum, car] = _full_add(lbitfunc(i), rbitfunc(i), carry);
                carry = car;
                if (i < N) {
                    this->_at(i) = sum;
                }
            }
            return *this;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline static integer<std::max(N, M), Word, Allocator, Signed> _bitwise_sub(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            integer<std::max(N, M), Word, Allocator, Signed> res;
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); ++i) {
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
            
            for (std::size_t i = 0; i < N; ++i) {
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
            for (std::size_t i = 0; i < N; ++i) {
                res[i] = this->_at(i);
            }
            return res;
        }

        operator bool() const noexcept {
            return (*this != 0);
        }

        template<typename I>
        requires std::is_integral_v<I> && (!is_integer_v<I>)
        operator I() const noexcept {
            I base = static_cast<I>(1);
            I res = static_cast<I>(0);

            for (std::size_t i = 0; i < N - Signed; ++i) {
                res += base * this->_at(i);
                base *= 2;
            }

            if (Signed) {
                res -= base * this->sign();
            }
            return res;
        }

        struct iterator {
            integer& _obj;
            std::size_t _index;
            using value_type = struct bit_reference;
            using reference = struct bit_reference;
            using difference_type = std::size_t;
            using pointer = reference*;
            using iterator_category = std::random_access_iterator_tag;
            
            iterator(integer& b, std::size_t pos) noexcept
            : _obj(b), _index(pos) {}

            reference operator*() noexcept {
                return reference(_obj, _index);
            }

            inline iterator operator++() noexcept {
                auto copy = *this;
                _index++;            
                return copy;
            }

            inline iterator& operator++(int) noexcept {
                _index++;
                return *this;
            }

            inline iterator& operator+=(difference_type n) {
                _index += n;
                return *this;
            }

            inline iterator operator+(difference_type x) const noexcept {
                auto copy = *this;
                copy._index += x;
                return copy;
            }

            inline iterator operator-(difference_type x) const noexcept {
                auto copy = *this;
                copy._index -= x;
                return copy;
            }

            inline difference_type operator-(iterator other) const noexcept {
                return _index - other._index;
            }

            inline bool operator!=(const iterator& other) const noexcept {
                return _index != other._index;
            }

            inline bool operator==(const iterator& other) const noexcept {
                return _index == other._index;
            }
        };

        struct const_iterator {
            const integer& _obj;
            std::size_t _index;
            using value_type = struct bit_reference;
            using reference = struct bit_reference;
            using const_reference = struct bit_const_reference;
            using difference_type = std::size_t;
            using pointer = reference*;
            using iterator_category = std::random_access_iterator_tag;
            
            const_iterator(const integer& b, std::size_t pos) noexcept
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
            friend struct integer;
            
            word_type* _word;
            std::size_t _b_pos;

            bit_reference(integer& b, std::size_t pos) noexcept {
                _word = &(b._get_word(pos));
                _b_pos = integer::_which_bit(pos);
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

            inline bool operator~() const noexcept {
                return ((*_word) & (1 << _b_pos)) == 0;
            }

            inline operator bool() const noexcept {
                return ((*_word) & (1 << _b_pos)) != 0; 
            }

            inline bool value() const noexcept {
                return ((*_word) & (1 << _b_pos)) != 0; 
            }

            bit_reference& flip() noexcept {
                *_word ^= (1 << _b_pos);
                return *this;
            }
        };

        struct bit_const_reference {
            const word_type* _word;
            std::size_t _b_pos;

            bit_const_reference(const integer& b, std::size_t pos) noexcept {
                _word = &b._get_word(pos);
                _b_pos = integer::_which_bit(pos);
            }

            bit_const_reference(const bit_const_reference&) noexcept = default;
            ~bit_const_reference() noexcept { }
        
            inline bool operator~() const noexcept {
                return ((*_word) & (1 << _b_pos)) == 0;
            }

            inline operator bool() const noexcept {
                return ((*_word) & (1 << _b_pos)) != 0; 
            }
        };

        friend std::ostream& operator<<(std::ostream& os, const integer& val) noexcept {
            os << val.str();
            return os;
        }
    };
    
}

// std
// type traits
template<class Int, typename I>
requires (std::is_integral_v<I> && (!exlib::is_integer_v<I>)) && (exlib::is_integer_v<Int>)
struct std::common_type<I, Int> {
    static constexpr std::size_t n = Int::size();
    static constexpr std::size_t m = sizeof(I) * byte_size;
    using type = std::conditional_t<(n > m), Int, I>;
};

template<class Int, typename I>
requires (std::is_integral_v<I> && (!exlib::is_integer_v<I>)) && (exlib::is_integer_v<Int>)
struct std::common_type<Int, I> {
    static constexpr std::size_t n = Int::size();
    static constexpr std::size_t m = sizeof(I) * byte_size;
    using type = std::conditional_t<(n > m), Int, I>;
};

template<class Int1, class Int2>
requires exlib::is_integer_v<Int1> && exlib::is_integer_v<Int2>
struct std::common_type<Int1, Int2> {
    static constexpr std::size_t n = Int1::size();
    static constexpr std::size_t m = Int2::size();
    using type = std::conditional_t<(n > m), Int1, Int2>;
};

template<std::size_t N, class Word, class Allocator, bool Signed>
struct std::is_integral<exlib::integer<N, Word, Allocator, Signed>> : public std::true_type {};

// formatter
template <class Int>
requires exlib::is_integer_v<Int>
struct std::formatter<Int> : std::formatter<std::string> {
    auto format(const auto& integer, auto& ctx) const {
        return std::format_to(ctx.out(), "{}", integer.str());
    }
};

namespace std {
    template<class Int1, class Int2>
    requires std::is_integral_v<Int1> && std::is_integral_v<Int2>
    std::common_type_t<Int1, Int2> pow(Int1 a, Int2 b) {
        std::common_type_t<Int1, Int2> res = std::common_type_t<Int1, Int2>(1);
        while (b) {
            if (b & 1) res *= a;
            a *= a;
            b >>= 1;
        }
        return res;
    }

    template<class Int1, class Int2>
    requires std::is_integral_v<Int1> && std::is_integral_v<Int1>
    std::common_type_t<Int1, Int2> gcd(Int1 a, Int2 b) {
        if (b == 0) {
            return a;
        }
        return gcd(b, a % b);
    }

    template<class Int1, class Int2>
    requires std::is_integral_v<Int1> && std::is_integral_v<Int1>
    std::common_type_t<Int1, Int2> lcm(Int1 a, Int2 b) {
        auto t = a / gcd(a, b);
        return a * b;
    }
}