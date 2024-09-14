#pragma once
#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include "uint4_t.h"
#include "array_type.h"
#include "../log.h"

#define byte_size 8

namespace exlib {
    template<std::size_t N, class Word, class Allocator, bool Signed>
    struct integer {
        static_assert(std::is_integral_v<Word>);;

        struct iterator;
        struct const_iterator;
        struct bit_reference;
        struct const_bit_reference;
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
            std::copy(other._data.cbegin(), other._data.cbegin() + std::min(array_size, other.array_size), this->_data.begin());
        }

        template<typename I>
        requires std::is_integral_v<I>
        integer(const I& i) noexcept {
            this->assign_integral(i);
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
            for (std::size_t i = 0; i < digits10; i++) {
                src._data[i] = (i < s.size()) ? (s[s.size() - 1 - i] - '0') : 0;
            }

            for (std::size_t i = 0; i < digits10; i++) {
                *this += ((src >> (4 * i)) & 0xf) * static_cast<std::size_t>(std::pow(10, i));
            }

            if (neg) {
                *this = ~(*this) + self_type(1);
            }

            return *this;
        }

        std::string str() const noexcept {
            constexpr std::size_t M = digits10 * 4;
            integer<M, details::uint4_t, void, false> res;
            auto&& abs = this->abs();

            for (std::size_t i = 0; i < N; i++) {
                res[0] = abs._at(N - 1 - i);

                if (i != N - 1) {
                    for (size_t j = 0; j < res._data.size(); j++) {
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

            for (std::size_t i = 0; i < n; i++) {
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
        requires std::is_integral_v<I>
        reference operator=(const I& i) noexcept {
            return this->assign_integral(i);
        }
 
        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->reset(other.filling_mask());
            for (std::size_t i = 0; i < std::min(N, M); i++) {
                this->_at(i) = other._at(i);
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

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator*(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            auto&& lhs_abs = integer<std::max(N, M), Word, Allocator, Signed>(this->abs());
            auto&& rhs_abs = other.abs();

            integer<std::max(N, M), Word, Allocator, Signed> res = 0;
            for (std::size_t i = 0; i < M; i++) {
                if (rhs_abs[i]) {
                    res += (lhs_abs << i);
                }
            }
            if (this->sign() != other.sign()) {
                res = ~res + integer<std::max(N, M), Word, Allocator, Signed>(1);
            }
            return res;
        }

        template<typename T>
        requires std::is_integral_v<T>
        integer<std::max(N, sizeof(T) * byte_size), Word, Allocator, Signed> operator*(const T& val) const noexcept {
            return *this * integer<sizeof(T) * byte_size, Word, Allocator, Signed>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend integer<std::max(sizeof(T) * byte_size, N), Word, Allocator, Signed> operator*(const T& lhs, const_reference rhs) noexcept {
            return integer<sizeof(T) * byte_size, Word, Allocator, Signed>(lhs) * rhs;
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

            for (std::size_t i = 0; i < N; i++) {
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
        requires std::is_integral_v<I>
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator/(const I& val) const {
            return *this / integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        template<typename I>
        requires std::is_integral_v<I>
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

            for (std::size_t i = 0; i < N; i++) {
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

        template<typename T>
        requires std::is_integral_v<T>
        integer<std::max(N, sizeof(T) * byte_size), Word, Allocator, Signed> operator%(const T& val) const {
            return *this % integer<sizeof(T) * byte_size, Word, Allocator, Signed>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend integer<std::max(sizeof(T) * byte_size, N), Word, Allocator, Signed> operator%(const T& lhs, const_reference rhs) noexcept {
            return integer<sizeof(T) * byte_size, Word, Allocator, Signed>(lhs) % rhs;
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

        template<typename T>
        requires std::is_integral_v<T>
        reference operator*=(const T& val) noexcept {
            *this *= integer<sizeof(T) * byte_size, Word, Allocator, Signed>(val);
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

            for (std::size_t i = 0; i < N; i++) {
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

        template<typename T>
        requires std::is_integral_v<T>
        reference operator/=(const T& val) {
            *this /= integer<sizeof(T) * byte_size, Word, Allocator, Signed>(val);
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

            for (std::size_t i = 0; i < N; i++) {
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

        template<typename T>
        requires std::is_integral_v<T>
        reference operator%=(const T& val) {
            *this %= integer<sizeof(T) * byte_size, Word, Allocator, Signed>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bit bitwise_add_assign(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            bit carry = 0; 
            for (std::size_t i = 0; i < N; i++) {
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
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
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
        std::enable_if_t<!std::is_same_v<Word, OWord>, bool> _bitwise_equal(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
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
        integer<std::max(N, M), Word, Allocator, Signed> operator&(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_and(other);
        }

        template<typename I>
        requires std::is_integral_v<I>
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator&(const I& i) const noexcept {
            integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> res = *this;
            return res._bitwise_and(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i));
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator&=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->_bitwise_and_assign(other);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator|(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_or(other);
        }

        template<typename I>
        requires std::is_integral_v<I>
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator|(const I& i) const noexcept {
            integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> res = *this;
            return res._bitwise_or(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i));
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator|=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->_bitwise_or_assign(other);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> operator^(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res = *this;
            return res._bitwise_xor(other);
        }

        template<typename I>
        requires std::is_integral_v<I>
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator^(const I& i) const noexcept {
            integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> res = *this;
            return res._bitwise_xor(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i));
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        reference operator^=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            this->_bitwise_xor_assign(other);
            return *this;
        }
        

        template<std::size_t M, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> _bitwise_and(const integer<M, Word, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, OSigned> res;
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); i++) {
                res._data[i] = ((i < array_size) ? this->_data[i] : this->filling_mask()) & ((i < other.array_size) ? other._data[i] : other.filling_mask());
            }
            return res;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, integer<std::max(N, M), Word, Allocator, Signed>> _bitwise_and(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res;
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
        std::enable_if_t<!std::is_same_v<Word, OWord>, reference> _bitwise_and_assign(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) &= (i < M) ? other._at(i) : other.sign();
            }
            return *this;
        }

        template<std::size_t M, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> _bitwise_or(const integer<M, Word, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); i++) {
                res._data[i] = ((i < array_size) ? this->_data[i] : this->filling_mask()) | ((i < other.array_size) ? other._data[i] : other.filling_mask());
            }
            return res;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, integer<std::max(N, M), Word, Allocator, Signed>> _bitwise_or(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res;
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
        std::enable_if_t<!std::is_same_v<Word, OWord>, reference> _bitwise_or_assign(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) |= (i < M) ? other._at(i) : other.sign();
            }
            return *this;
        }

        template<std::size_t M, class OAllocator, bool OSigned>
        integer<std::max(N, M), Word, Allocator, Signed> _bitwise_xor(const integer<M, Word, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res;
            for (std::size_t i = 0; i < std::max(array_size, other.array_size); i++) {
                res._data[i] = ((i < array_size) ? this->_data[i] : this->filling_mask()) ^ ((i < other.array_size) ? other._data[i] : other.filling_mask());
            }
            return res;
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        std::enable_if_t<!std::is_same_v<Word, OWord>, integer<std::max(N, M), Word, Allocator, Signed>> _bitwise_xor(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            integer<std::max(N, M), Word, Allocator, Signed> res;
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
        std::enable_if_t<!std::is_same_v<Word, OWord>, reference> _bitwise_xor_assign(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) ^= (i < M) ? other._at(i) : other.sign();
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
        reference operator-=(const integer<M, OWord, OAllocator, OSigned>& other) noexcept {
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
        integer<std::max(N, M), Word, Allocator, Signed> operator+(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return this->_bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename I>
            requires std::is_integral_v<I>
        integer<std::max(N, sizeof(I) * byte_size), Word, Allocator, Signed> operator+(const I& val) const noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool val_sign = std::is_signed_v<I> ? std::signbit(val) : 0;
            return _bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<typename I>
            requires std::is_integral_v<I>
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
        requires std::is_integral_v<I>
        friend integer<std::max(sizeof(I) * byte_size, N), Word, Allocator, Signed> operator-(const I& lhs, const_reference rhs) noexcept {
            constexpr std::size_t M = sizeof(I) * byte_size;
            const bool lhs_sign = std::is_signed_v<I> ? std::signbit(lhs) : 0;
            return _bitwise_sub<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<typename T>
        requires std::is_integral_v<T>
        integer<std::max(N, sizeof(T) * byte_size), Word, Allocator, Signed> operator-(const T& val) const noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bool val_sign = std::is_signed_v<T> ? std::signbit(val) : 0;
            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator==(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return this->_bitwise_equal(other);
        }

        template<typename I>
        requires std::is_integral_v<I>
        bool operator==(const I& val) const noexcept {
            return this->_bitwise_equal(integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val));
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator!=(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
            return !(this->_bitwise_equal(other));
        }

        template<typename I>
        requires std::is_integral_v<I>
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

        reference operator>>=(auto&& x) {
            if (x < 0) {
                throw std::runtime_error("right shifted bit count is negative!");
            }
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
        bool operator<(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
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
            return *this < integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator<=(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
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
            return *this <= integer<sizeof(I) * byte_size, Word, Allocator, Signed>(val);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator>(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
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
            return *this > integer<sizeof(I) * byte_size, Word, Allocator, Signed>(i);
        }

        template<std::size_t M, class OWord, class OAllocator, bool OSigned>
        bool operator>=(const integer<M, OWord, OAllocator, OSigned>& other) const noexcept {
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
            return *this >= integer<sizeof(T) * byte_size, Word, Allocator, Signed>(val);
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

        void swap(const_reference other) noexcept {
            std::swap(_data, other._data);
        }

        auto filling_mask() const noexcept {
            if constexpr (!Signed) {
                return 0;
            }

            return sign() == static_cast<bit>(0) ? (0) : (-1);
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
        inline static integer<std::max(N, M), Word, Allocator, Signed> _bitwise_sub(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            integer<std::max(N, M), Word, Allocator, Signed> res;
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

        template<typename T>
        requires std::is_integral_v<T>
        operator T() const noexcept {
            T base = static_cast<T>(1);
            T res = static_cast<T>(0);

            for (std::size_t i = 0; i < N - Signed; i++) {
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
            using const_reference = struct const_bit_reference;
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

        struct const_bit_reference {
            const word_type* _word;
            std::size_t _b_pos;

            const_bit_reference(const integer& b, std::size_t pos) noexcept {
                _word = &b._get_word(pos);
                _b_pos = integer::_which_bit(pos);
            }

            const_bit_reference(const const_bit_reference&) noexcept = default;
            ~const_bit_reference() noexcept { }
        
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