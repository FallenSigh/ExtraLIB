#include "log.h"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

#define byte_size 8

// N bit signed integer
namespace zstl {
    template<std::size_t N>
    struct nints {
        struct iterator;
        struct reference;
        using word = unsigned int;
        using bit = bool;
        static inline constexpr std::size_t word_size = sizeof(word) * byte_size;

        friend struct reference;
        
        word _data[(N + word_size - 1) / word_size];
        
        nints() noexcept {
            std::fill(std::begin(_data), std::end(_data), -1);
        }

        nints(const nints<N>& other) noexcept {
            std::copy(std::begin(other._data), std::end(other._data), std::begin(_data));
        }

        nints(const nints<N>&& other) noexcept {
            if (this != &other) {
                std::move(std::begin(other._data), std::end(other._data), std::begin(_data));
            }
        }

        template<std::size_t M>
        nints(const nints<M>& other) noexcept {
            // 保留符号位截断
            std::fill(std::begin(_data), std::end(_data), (other.sign()) == 0 ? 0 : -1);

            for (std::size_t i = 0; i < std::min(N, M) - 1; i++) {
                this->_at(i) = other[i];
            }
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints(const T& x) noexcept {
            // 保留符号位截断
            std::fill(std::begin(_data), std::end(_data), (x >= 0) ? 0 : -1);
            
            for (std::size_t i = 0; i < N - 1; i++) {
                this->_at(i) = x >> i & 1;
            }
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<N>& operator=(const T& x) noexcept {
            // 保留符号位截断
            std::fill(std::begin(_data), std::end(_data), (x >= 0) ? 0 : -1);
            for (std::size_t i = 0; i < N - 1; i++) {
                this->_at(i) = x >> i & 1;
            }
            return *this;
        }

        nints<N>& operator=(nints<N>&& other) noexcept {
            if (this != &other) {
                std::move(std::begin(other._data), std::end(other._data), std::begin(_data));
            }
            return *this;
        }
        
        template <std::size_t M>
        nints<std::max(N, M)> operator+(const nints<M>& other) noexcept {
            return _bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
            requires std::is_integral_v<T>
        nints<std::max(N, sizeof(T) * byte_size)> operator+(const T& other) noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bit other_sign = (other >> (M - 1) & 1);

            return _bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other, &other_sign](std::size_t i) { return (i < M) ? (other >> i & 1) : other_sign; });
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N)> operator+(const T& lhs, const nints<N>& rhs) noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bit lhs_sign = lhs >> (M - 1) & 1;

            return _bitwise_add<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<std::size_t M>
        nints<std::max(N, M)> operator-(const nints<M>& other) noexcept {
            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N)> operator-(const T& lhs, const nints<N>& rhs) noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bit lhs_sign = lhs >> (M - 1) & 1;

            return _bitwise_sub<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<std::max(N, sizeof(T) * byte_size)> operator-(const T& other) noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bit other_sign = (other >> (M - 1) & 1);

            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other, &other_sign](std::size_t i) { return (i < M) ? (other >> i & 1) : other_sign; });
        
        }

        template<std::size_t M>
        nints<std::max(N, M)> operator*(const nints<M>& other) const noexcept {
            auto lhs_abs = nints<std::max(N, M)>(this->abs());
            auto rhs_abs = other.abs();

            nints<std::max(N, M)> res = 0;
            for (std::size_t i = 0; i < M; i++) {
                if (rhs_abs[i]) {
                    res += (lhs_abs << i);
                }
            }
            if (this->sign() != other.sign()) {
                res = ~res + nints<std::max(N, M)>(1);
            }
            return res;
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<std::max(N, sizeof(T) * byte_size)> operator*(const T& val) const noexcept {
            return *this * nints<sizeof(T) * byte_size>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N)> operator*(const T& lhs, const nints<N>& rhs) noexcept {
            return nints<sizeof(T) * byte_size>(lhs) * rhs;
        }

        template<std::size_t M>
        nints<std::max(N, M)> operator/(const nints<M>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            auto rhs_abs = other.abs();

            nints<std::max(N, M)> quotient = 0;
            nints<std::max(N, M)> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                    quotient[N - 1 - i] = 1;
                }
            }

            if (this->sign() != other.sign()) {
                quotient = ~quotient + nints<std::max(N, M)>(1);
            }

            return quotient;
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<std::max(N, sizeof(T) * byte_size)> operator/(const T& val) const {
            return *this / nints<sizeof(T) * byte_size>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N)> operator/(const T& lhs, const nints<N>& rhs) noexcept {
            return nints<sizeof(T) * byte_size>(lhs) / rhs;
        }

        template<std::size_t M>
        nints<std::max(N, M)> operator%(const nints<M>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            auto rhs_abs = other.abs();

            nints<std::max(N, M)> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                }
            }

            if (this->sign() != other.sign()) {
                remainder = ~remainder + nints<std::max(N, M)>(1);
            }

            return remainder;
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<std::max(N, sizeof(T) * byte_size)> operator%(const T& val) const {
            return *this % nints<sizeof(T) * byte_size>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N)> operator%(const T& lhs, const nints<N>& rhs) noexcept {
            return nints<sizeof(T) * byte_size>(lhs) % rhs;
        }

        template<std::size_t M>
        nints<N>& operator+=(const nints<M>& other) noexcept {
            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<N>& operator+=(const T& val) noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bit val_sign = (val >> (M - 1) & 1);

            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M>
        nints<N>& operator-=(const nints<M>& other) noexcept {
            return _bitwise_sub_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<N>& operator-=(const T& val) noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bit val_sign = (val >> (M - 1) & 1);

            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? this->_at(i) : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M>
        nints<N>& operator*=(const nints<M>& other) noexcept {
            auto lhs_abs = this->abs();
            auto rhs_abs = other.abs();
            
            nints<N> result = 0;
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
        nints<N>& operator*=(const T& val) noexcept {
            *this *= nints<sizeof(T) * byte_size>(val);
            return *this;
        }

        template<std::size_t M>
        nints<N>& operator/=(const nints<M>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }
            auto lhs_abs = this->abs();
            auto rhs_abs = other.abs();

            nints<N> quotient = 0;
            nints<N> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                    quotient[N - 1 - i] = 1;
                }
            }

            if (this->sign() != other.sign()) {
                quotient = ~quotient + nints<std::max(N, M)>(1);
            }

            *this = std::move(quotient);
            return *this;
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<N>& operator/=(const T& val) {
            *this /= nints<sizeof(T) * byte_size>(val);
            return *this;
        }

        template<std::size_t M>
        nints<N>& operator%=(const nints<M>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            auto rhs_abs = other.abs();

            nints<N> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
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
        nints<N>& operator%=(const T& val) {
            *this %= nints<sizeof(T) * byte_size>(val);
            return *this;
        }

        nints<N> operator~() const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < N; i++) {
                copy[i] = !copy[i];
            }
            return copy;
        }

        template<std::size_t M>
        nints<N> operator&(const nints<M>& other) const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();
                if (i < N) {
                    copy[i] = lbit & rbit;
                }
            }
            return copy;
        }

        template<std::size_t M>
        nints<N>& operator&=(const nints<M>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) &= (i < M) ? other[i] : other.sign();
            }
            return *this;
        }

        template<std::size_t M>
        nints<N> operator|(const nints<M>& other) const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();
                if (i < N) {
                    copy[i] = lbit | rbit;
                }
            }
            return copy;
        }

        template<std::size_t M>
        nints<N>& operator|=(const nints<M>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) |= (i < M) ? other[i] : other.sign();
            }
            return *this;
        }

        template<std::size_t M>
        nints<N> operator^(const nints<M>& other) const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();
                if (i < N) {
                    copy[i] = lbit ^ rbit;
                }
            }
            return copy;
        }

        template<std::size_t M>
        nints<N>& operator^=(const nints<M>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                this->_at(i) ^= (i < M) ? other[i] : other.sign();
            }
            return *this;
        }

        template<std::size_t M>
        bool operator==(const nints<M>& other) const noexcept {
            if (this->sign() != other.sign()) {
                return false;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < N) ? this->_at(i) : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                if (lbit != rbit) {
                    return false;
                }
            }
            return true;
        }

        template<typename T>
        requires std::is_integral_v<T>
        bool operator==(const T& val) const noexcept {
            return *this == nints<sizeof(T) * byte_size>(val);
        }

        template<std::size_t M>
        bool operator!=(const nints<M>& other) const noexcept {
            return !(*this == other);
        }

        template<typename T>
        requires std::is_integral_v<T>
        bool operator!=(const T& val) const noexcept {
            return *this != nints<sizeof(T) * byte_size>(val);
        }

        template<std::size_t M>
        bool operator<(const nints<M>& other) const noexcept {
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

        template<typename T>
        requires std::is_integral_v<T>
        friend bool operator<(const T& lhs, const nints<N>& rhs) noexcept {
            constexpr std::size_t M = sizeof(T) * byte_size;
            const bit lhs_sign = lhs >> (M - 1) & 1;
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

        template<typename T>
        requires std::is_integral_v<T>
        bool operator<(const T& val) const noexcept {
            return *this < nints<sizeof(T) * byte_size>(val);
        }

        template<std::size_t M>
        bool operator<=(const nints<M>& other) const noexcept {
            return *this == other || *this < other;
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend bool operator<=(const T& lhs, const nints<N>& rhs) noexcept {
            return !(rhs > lhs);
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator<=(const T& val) const noexcept {
            return *this <= nints<sizeof(T) * byte_size>(val);
        }

        template<std::size_t M>
        bool operator>(const nints<M>& other) const noexcept {
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
        friend bool operator>(const T& lhs, const nints<N>& rhs) noexcept {
            return !(rhs <= lhs);
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator>(const T& val) const noexcept {
            return *this > nints<sizeof(T) * byte_size>(val);
        }

        template<std::size_t M>
        bool operator>=(const nints<M>& other) const noexcept {
            return *this == other || *this > other;
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend bool operator>=(const T& lhs, const nints<N>& rhs) noexcept {
            return !(rhs < lhs);
        }

        template<typename T>
        requires std::is_integral_v<T>
        bool operator>=(const T& val) const noexcept {
            return *this >= nints<sizeof(T) * byte_size>(val);
        }

        nints<N> operator<<(auto&& other) const noexcept {
            auto copy = *this;
            copy <<= other;
            return copy;
        }

        nints<N>& operator<<=(auto&& x) noexcept {
            if (static_cast<std::size_t>(x) >= N) {
                std::fill(std::begin(_data), std::end(_data), ((sign() == 0) ? 0 : -1));
            } else {
                for (std::size_t i = N - 1; ~i; i--) {
                    this->_at(i) = (i < static_cast<std::size_t>(x)) ? 0 : this->_at(i - static_cast<std::size_t>(x));
                }
            }
            return *this;
        }

        nints<N> operator>>(auto&& x) const noexcept {
            auto copy = *this;
            copy >>= x;
            return copy;
        }

        nints<N>& operator>>=(auto&& x) noexcept {
            if (static_cast<std::size_t>(x) >= N) {
                std::fill(std::begin(_data), std::end(_data), ((sign() == 0) ? 0 : -1));
            } else {
                for (std::size_t i = 0; i < N; i++) {
                    this->_at(i) = (i + static_cast<std::size_t>(x) < N) ? this->_at(i + static_cast<std::size_t>(x)) : sign();
                }
            }   
            return *this;
        }

        nints<N> abs() const noexcept {
            return (sign() == 0) ? *this : ~(*this) + nints<N>(1);
        }

        bit sign() const noexcept {
            return this->_at(N - 1);
        }

        constexpr std::size_t size() const noexcept {
            return N;
        }

        iterator begin() noexcept {
            return iterator(*this, 0);
        }

        iterator end() noexcept {
            return iterator(*this, N - 1);
        }
        
        reference operator[](std::size_t pos) noexcept {
            return reference(*this, pos);
        }

        bool operator[](std::size_t pos) const noexcept {
            return (this->_get_word(pos) & (1 << _which_bit(pos))) != static_cast<word>(0);
        }

        template<typename T>
        requires std::is_integral_v<T>
        operator T() const noexcept {
            T base = 1;
            T res = 0;

            for (std::size_t i = 0; i < N - 1; i++) {
                res += base * this->_at(i);
                base *= 2;
            }

            res -= base * this->_at(N - 1);
            return res;
        }

        reference _at(std::size_t pos) {
            return reference(*this, pos);
        }

        bool _at(std::size_t pos) const {
            return (this->_get_word(pos) & (1 << _which_bit(pos))) != static_cast<word>(0);
        }

        reference at(std::size_t pos) {
            if (pos >= N) [[unlikely]] {
                throw std::out_of_range("pos out of range! " + std::to_string(pos) + " / " + std::to_string(N));
            }
            return reference(*this, pos);
        }

        bool at(std::size_t pos) const {
            if (pos >= N) [[unlikely]] {
                throw std::out_of_range("pos out of range! " + std::to_string(pos) + " / " + std::to_string(N));
            }
            return (this->_get_word(pos) & (1 << _which_bit(pos))) != static_cast<word>(0);
        }

        word& _get_word(std::size_t pos) noexcept {
            return _data[_which_word(pos)];
        }

        const word& _get_word(std::size_t pos) const noexcept {
            return _data[_which_word(pos)];
        }

        static std::size_t _which_word(std::size_t pos) noexcept {
            return pos / word_size;
        }

        static std::size_t _which_bit(std::size_t pos) noexcept {
            return pos % word_size;
        }

        long long dec() const noexcept {
            long long base = 1;
            long long res = 0;

            for (std::size_t i = 0; i < N - 1; i++) {
                res += base * this->_at(i);
                base *= 2;
            }

            res -= base * this->_at(N - 1);
            return res;
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
        inline static nints<std::max(N, M)> _bitwise_add(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            nints<std::max(N, M)> res;
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [sum, car] = _full_add(lbitfunc(i), rbitfunc(i), carry);
                carry = car;
                res[i] = sum;
            }
            return res;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline auto&& _bitwise_add_assign(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [sum, car] = _full_add(lbitfunc(i), rbitfunc(i), carry);
                carry = car;
                if (i < N) {
                    this->_at(i) = sum;
                }
            }
            return *this;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline static nints<std::max(N, M)> _bitwise_sub(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            nints<std::max(N, M)> res;
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [diff, borr] = _full_sub(lbitfunc(i), rbitfunc(i), borrow);
                borrow = borr;
                res[i] = diff;
            }
            return res;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline auto&& _bitwise_sub_assign(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [diff, borr] = _full_sub(lbitfunc(i), rbitfunc(i), borrow);
                borrow = borr;
                if (i < N) {
                    this->_at(i) = diff;
                }
            }
            return *this;
        }
        
        std::string to_string() const noexcept {
            std::string str;
            for (std::size_t i = size() - 1; ~i; i--) {
                str.push_back(this->_at[i] + '0');
            }
            return str;
        }

        friend std::ostream& operator<<(std::ostream& os, const nints& val) noexcept {
            std::string str;
            for (std::size_t i = val.size() - 1; ~i; i--) {
                str.push_back(val[i] + '0');
            }
            os << str;
            return os;
        }

        struct reference {
            friend struct nints;
            
            word* _word;
            std::size_t _b_pos;

            reference(nints& sint, std::size_t pos) noexcept {
                _word = &sint._get_word(pos);
                _b_pos = nints::_which_bit(pos);
            }

            reference(const reference&) noexcept = default;
            ~reference() noexcept { }
        
            reference& operator=(bool x) noexcept {
                if (x) *_word |= 1 << _b_pos;
                else *_word &= ~(1 << _b_pos);
                return *this;
            }

            reference& operator=(const reference &other) noexcept {
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

            reference& flip() noexcept {
                *_word ^= (1 << _b_pos);
                return *this;
            }
        };

        struct iterator {
            nints& _obj;
            std::size_t _index;
            
            iterator(nints& b, std::size_t pos) noexcept
            : _obj(b), _index(pos) {}

            reference operator*() noexcept {
                return reference(_obj, _index);
            }

            const reference& operator*() const noexcept {
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

            bool operator!=(const iterator& other) const noexcept {
                return _index != other._index;
            }

            bool operator==(const iterator& other) const noexcept {
                return _index == other._index;
            }
        };
    };

}