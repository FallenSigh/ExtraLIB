#pragma once
#include <array>
#include <ostream>

namespace exlib {
    struct uint4_t {
        std::array<bool, 4> _data;

        uint4_t() noexcept {
            _data.fill(0);
        }

        template<typename I>
        uint4_t(const I& i) noexcept {
            for (int j = 0; j < 4; j++) {
                _data[j] = (i >> j & 1);
            }
        }
        uint4_t operator+(const uint4_t& other) const noexcept {
            uint4_t copy = *this;
            copy += other;
            return copy;
        }

        uint4_t operator-(const uint4_t& other) const noexcept {
            uint4_t copy = *this;
            copy -= other;
            return copy;
        }

        uint4_t& operator+=(const uint4_t& other) noexcept {
            bool carry = 0;
            for (int i = 0; i < 4; i++) {
                auto [sum, car] = _full_add(_data[i], other._data[i], carry);
                _data[i] = sum;
                carry = car;
            }

            return *this;
        }

        uint4_t& operator-=(const uint4_t& other) noexcept {
            bool borrow = 0;
            for (int i = 0; i < 4; i++) {
                auto [diff, bor] = _full_sub(_data[i], other._data[i], borrow);
                _data[i] = diff;
                borrow = bor;
            }
            
            return *this;
        }

        static std::pair<bool, bool> _full_sub(bool x, bool y, bool b) noexcept {
            bool diff = x ^ y ^ b;
            bool borrow = ((!x) & (y | b)) | (y & b);
            return {diff, borrow};
        }

        static std::pair<bool, bool> _full_add(bool x, bool y, bool c) noexcept {
            bool sum = x ^ y ^ c;
            bool carry = (x & y) | ((x ^ y) & c);
            return {sum, carry};
        }

        template<typename I>
        I value() const noexcept {
            I res = 0;
            for (int i = 0; i < 4; i++) {
                res += _data[i] * (1 << i);
            }
            return res;
        }

        uint4_t& operator*=(const uint4_t& other) noexcept {
            for (int i = 0; i < 4; i++) {
                if (other._data[i]) {
                    *this += (1 << i);
                }
            }

            return *this;
        }

        uint4_t operator*(const uint4_t& other) const noexcept {
            uint4_t copy = *this;
            copy *= other;
            return copy;
        }

        uint4_t& operator<<=(std::size_t x) noexcept {
            if (x >= 4) {
                _data.fill(0);
                return *this;
            }

            for (int i = 3; i >= 0; i--) {
                _data[i] = (static_cast<std::size_t>(i) < x) ? 0 : _data[i - x];
            }
            
            return *this;
        }

        uint4_t& operator>>=(std::size_t x) noexcept {
            if (x >= 4) {
                _data.fill(0);
                return *this;
            }

            for (int i = 0; i < 4; i++) {
                _data[i] = (i + x < 4) ? _data[i + x] : 0;
            }

            return *this;
        }

        uint4_t& operator/=(const uint4_t& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            uint4_t quotient = 0;
            uint4_t remainder = 0;
            for (int i = 0; i < 4; i++) {
                remainder <<= 1;
                remainder._data[0] = _data[3 - i];

                if (remainder >= other) {
                    remainder -= other;
                    quotient._data[3 - i] = true;
                }
            }
            *this = std::move(quotient);
            return *this;
        } 

        uint4_t& operator%=(const uint4_t& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            uint4_t remainder = 0;
            for (int i = 0; i < 4; i++) {
                remainder <<= 1;
                remainder._data[0] = _data[3 - i];

                if (remainder >= other) {
                    remainder -= other;
                }
            }
            *this = std::move(remainder);
            return *this;
        } 

        template<typename I>
        bool operator==(const I& i) const noexcept {
            return i == this->value<I>();
        }

        template<typename I>
        bool operator!=(const I& i) const noexcept {
            return i != this->value<I>();
        }

        bool operator==(const uint4_t& other) const noexcept {
            return _data == other._data;
        }

        bool operator!=(const uint4_t& other) const noexcept {
            return _data != other._data;
        }

        bool operator>=(const uint4_t& other) const noexcept {
            for (int i = 3; i >= 0; i--) {
                if (_data[i] != other._data[i]) {
                    return _data[i] >= other._data[i];
                }
            }
            return true;
        }

        bool operator<=(const uint4_t& other) const noexcept {
            for (int i = 3; i >= 0; i--) {
                if (_data[i] != other._data[i]) {
                    return _data[i] <= other._data[i];
                }
            }
            return true;
        }

        bool operator>(const uint4_t& other) const noexcept {
            return !(*this <= other);
        }

        bool operator<(const uint4_t& other) const noexcept {
            return !(*this >= other);
        }

        uint4_t operator/(const uint4_t& other) noexcept {
            uint4_t copy = *this;
            copy /= other;
            return copy;
        }

        uint4_t& operator&=(const uint4_t& other) noexcept {
            for (int i = 0; i < 4; i++) {
                _data[i] &= other._data[i];
            }
            return *this;
        }

        uint4_t operator&(const uint4_t& other) const noexcept {
            uint4_t copy = *this;
            copy &= other;
            return copy;
        }

        uint4_t& operator|=(const uint4_t& other) noexcept {
            for (int i = 0; i < 4; i++) {
                _data[i] |= other._data[i];
            }
            return *this;
        }

        uint4_t operator|(const uint4_t& other) const noexcept {
            uint4_t copy = *this;
            copy |= other;
            return copy;
        }

        uint4_t& operator^=(const uint4_t& other) noexcept {
            for (int i = 0; i < 4; i++) {
                _data[i] ^= other._data[i];
            }
            return *this;
        }

        uint4_t operator^(const uint4_t& other) const noexcept {
            uint4_t copy = *this;
            copy ^= other;
            return copy;
        }

        uint4_t operator&(int i) {
            uint4_t copy = *this;
            for (int j = 0; j < 4; j++) {
                copy._data[j] &= (i >> j & 1);
            }
            return copy;
        }

        uint4_t operator&(unsigned int i) {
            uint4_t copy = *this;
            for (int j = 0; j < 4; j++) {
                copy._data[j] &= (i >> j & 1);
            }
            return copy;
        }

        friend std::ostream& operator<<(std::ostream& os, const uint4_t& x) {
            os << x.value<int>();
            return os;
        }

    };
}

namespace std {
    template<>
    struct is_integral<exlib::uint4_t> : public true_type {};
}