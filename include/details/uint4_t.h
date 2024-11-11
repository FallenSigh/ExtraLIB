#pragma once
#include <cstdint>
#include <iostream>
#include <ostream>

namespace exlib {
    namespace details {
        struct uint4_t {
            std::uint8_t _data;

            uint4_t() noexcept {
                _data = 0;
            }

            template<typename I>
            uint4_t(const I& i) noexcept {
                _data = static_cast<std::uint8_t>(i);
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
                _data += other._data;
                return *this;
            }

            uint4_t& operator-=(const uint4_t& other) noexcept {
                _data -= other._data;                
                return *this;
            }

            std::uint16_t value() const noexcept {
                return _data & 0xf;
            }

            uint4_t& operator*=(const uint4_t& other) noexcept {
                _data *= other._data;
                return *this;
            }

            uint4_t operator*(const uint4_t& other) const noexcept {
                uint4_t copy = *this;
                copy *= other;
                return copy;
            }

            uint4_t& operator<<=(std::size_t x) noexcept {
                _data <<= x;
                return *this;
            }

            uint4_t& operator>>=(std::size_t x) noexcept {
                _data >>= x;
                return *this;
            }

            uint4_t& operator/=(const uint4_t& other) {
                _data /= other._data;
                return *this;
            } 

            uint4_t& operator%=(const uint4_t& other) {
                _data %= other._data;
                return *this;
            } 

            template<typename I>
            bool operator==(const I& i) const noexcept {
                return _data == static_cast<std::uint16_t>(i);
            }

            template<typename I>
            bool operator!=(const I& i) const noexcept {
                return _data != static_cast<std::uint16_t>(i);
            }

            bool operator==(const uint4_t& other) const noexcept {
                return _data == other._data;
            }

            bool operator!=(const uint4_t& other) const noexcept {
                return _data != other._data;
            }

            bool operator>=(const uint4_t& other) const noexcept {
                return _data >= other._data;
            }

            bool operator<=(const uint4_t& other) const noexcept {
                return _data <= other._data;
            }

            bool operator>(const uint4_t& other) const noexcept {
                return _data > other._data;
            }

            bool operator<(const uint4_t& other) const noexcept {
                return _data < other._data;
            }

            uint4_t operator/(const uint4_t& other) noexcept {
                uint4_t copy = *this;
                copy /= other;
                return copy;
            }

            uint4_t& operator&=(const uint4_t& other) noexcept {
                _data &= other._data;
                return *this;
            }

            uint4_t operator&(const uint4_t& other) const noexcept {
                uint4_t copy = *this;
                copy &= other;
                return copy;
            }

            uint4_t& operator|=(const uint4_t& other) noexcept {
                _data |= other._data;
                return *this;
            }

            uint4_t operator|(const uint4_t& other) const noexcept {
                uint4_t copy = *this;
                copy |= other;
                return copy;
            }

            uint4_t& operator^=(const uint4_t& other) noexcept {
                _data ^= other._data;
                return *this;
            }

            uint4_t operator^(const uint4_t& other) const noexcept {
                uint4_t copy = *this;
                copy ^= other;
                return copy;
            }

            uint4_t operator&(int i) {
                uint4_t copy = *this;
                copy._data &= static_cast<std::uint8_t>(i);
                return copy;
            }

            uint4_t operator&(unsigned int i) {
                uint4_t copy = *this;
                copy._data &= static_cast<std::uint8_t>(i);
                return copy;
            }

            friend std::ostream& operator<<(std::ostream& os, const uint4_t& x) {
                os << static_cast<std::uint16_t>(x.value());
                return os;
            }
        };
    }
}
