#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <array>

#include "details/int_base.h"

// N bit unsigned integer
namespace exlib {
    template<std::size_t N, class Word = unsigned int, class Allocator = void>
    struct unints : public int_base<N, Word, Allocator, false> {
        static_assert(std::is_integral_v<Word>);;
        using base_class_type = int_base<N, Word, Allocator, false>;
        
        using iterator = base_class_type::iterator;
        using const_iterator = base_class_type::const_iterator;
        using bit_reference = base_class_type::bit_reference;
        using const_bit_reference = base_class_type::const_bit_reference;
        using reference = unints<N, Word, Allocator>&;
        using const_reference = const unints<N, Word, Allocator>&;
        using self_type = unints<N, Word, Allocator>;
        
        using word_type = Word;
        using bit = bool;
        using pointer = word_type*;

        std::string dec_str() const noexcept {
            constexpr std::size_t M = base_class_type::digits10 * 4;
            unints<M, uint4_t> res;
            
            for (std::size_t i = 0; i < N; i++) {
                res[0] = this->_at(N - 1 - i);

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

        std::string bin_str() const noexcept {
            std::string str;
            for (std::size_t i = this->size() - 1; ~i; i--) {
                str.push_back(this->_at(i) + '0');
            }
            return "0b" + str;
        }

        std::string hex_str() const noexcept {
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
            return "0x" + res;
        }
        
        unints() noexcept : base_class_type() {
        }

        template<typename I>
        requires std::is_integral_v<I>
        unints(const I& i) noexcept : base_class_type(i) {
        }

        unints(const base_class_type& base) : base_class_type(base) {

        }

        unints(base_class_type&& base) : base_class_type(base) {

        }

        template<typename T>
        requires std::is_integral_v<T>
        reference operator=(const T& x) noexcept {
            this->assign_integral(x);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator>
        unints<std::max(N, M), Word> operator*(const unints<M, OWord, OAllocator>& other) const noexcept {
            auto lhs = unints<std::max(N, M), Word, Allocator>(*this);

            unints<std::max(N, M), Word> res = 0;
            for (std::size_t i = 0; i < M; i++) {
                if (other[i]) {
                    res += (lhs << i);
                }
            }
            return res;
        }

        template<typename T>
        requires std::is_integral_v<T>
        unints<std::max(N, sizeof(T) * byte_size)> operator*(const T& val) const noexcept {
            return *this * unints<sizeof(T) * byte_size>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend unints<std::max(sizeof(T) * byte_size, N)> operator*(const T& lhs, const_reference rhs) noexcept {
            return unints<sizeof(T) * byte_size>(lhs) * rhs;
        }

        template<std::size_t M, class OWord, class OAllocator>
        unints<std::max(N, M), Word, Allocator> operator/(const unints<M, OWord, OAllocator>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }
            unints<std::max(N, M), Word, Allocator> quotient = 0;
            unints<std::max(N, M), Word, Allocator> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = this->_at(N - 1 - i);

                if (remainder >= other) {
                    remainder = remainder - other;
                    quotient[N - 1 - i] = 1;
                }
            }

            return quotient;
        }

        template<typename T>
        requires std::is_integral_v<T>
        unints<std::max(N, sizeof(T) * byte_size)> operator/(const T& val) const {
            return *this / unints<sizeof(T) * byte_size>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend unints<std::max(sizeof(T) * byte_size, N)> operator/(const T& lhs, const_reference rhs) noexcept {
            return unints<sizeof(T) * byte_size>(lhs) / rhs;
        }

        template<std::size_t M, class OWord, class OAllocator>
        unints<std::max(N, M), Word, Allocator> operator%(const unints<M, OWord, OAllocator>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            unints<std::max(N, M), Word, Allocator> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = this->_at(N - 1- i);

                if (remainder >= other) {
                    remainder = remainder - other;
                }
            }

            return remainder;
        }

        template<typename T>
        requires std::is_integral_v<T>
        unints<std::max(N, sizeof(T) * byte_size)> operator%(const T& val) const {
            return *this % unints<sizeof(T) * byte_size>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend unints<std::max(sizeof(T) * byte_size, N), Word, Allocator> operator%(const T& lhs, const_reference rhs) noexcept {
            return unints<sizeof(T) * byte_size, Word, Allocator>(lhs) % rhs;
        }

        template<std::size_t M, class OWord, class OAllocator>
        reference operator*=(const unints<M, OWord, OAllocator>& other) noexcept {
            unints<N, Word, Allocator> result = 0;
            for (std::size_t i = 0; i < M; ++i) {
                if (other[i]) {
                    result += ((*this) << i);
                }
            }

            *this = std::move(result);
            return *this;
        }

        template<typename T>
        requires std::is_integral_v<T>
        reference operator*=(const T& val) noexcept {
            *this *= unints<sizeof(T) * byte_size>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator>
        reference operator/=(const unints<M, OWord, OAllocator>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }
            unints<N, Word, Allocator> quotient = 0;
            unints<N, Word, Allocator> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = this->_at(N - 1 - i);

                if (remainder >= other) {
                    remainder -= other;
                    quotient[N - 1 - i] = 1;
                }
            }

            *this = std::move(quotient);
            return *this;
        }

        template<typename T>
        requires std::is_integral_v<T>
        reference operator/=(const T& val) {
            *this /= unints<sizeof(T) * byte_size>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator>
        reference operator%=(const unints<M, OWord, OAllocator>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            unints<N, Word, Allocator> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = this->_at(N - 1- i);

                if (remainder >= other) {
                    remainder = remainder - other;
                }
            }

            *this = std::move(remainder);
            return *this;
        }

        template<typename T>
        requires std::is_integral_v<T>
        reference operator%=(const T& val) {
            *this %= unints<sizeof(T) * byte_size>(val);
            return *this;
        }

        static self_type max_value() noexcept {
            static unints<N, Word, Allocator> res;
            std::fill(std::begin(res._data), std::end(res._data), -1);
            return res;
        }

        template<typename T>
        requires std::is_integral_v<T>
        operator T() const noexcept {
            T base = static_cast<T>(1);
            T res = static_cast<T>(0);

            for (std::size_t i = 0; i < N; i++) {
                res += base * this->_at(i);
                base *= 2;
            }

            return res;
        }

        constexpr bit sign() const noexcept {
            return 0;
        }

        template<std::size_t M>
        bit bitwise_add_assign(const unints<M, Word>& other) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit carry = 0;
            
            for (std::size_t i = 0; i < N; i++) {
                auto [sum, car] = _full_add((i < N) ? this->_at(i) : 0, (i < M) ? other._at(i) : 0, carry);
                carry = car;
                this->_at(i) = sum;
            }
            return carry;
        }

        template<std::size_t M>
        bit bitwise_sub_assign(const unints<M, Word>& other) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [diff, borr] = _full_sub((i < N) ? this->_at(i) : 0, (i < M) ? other._at(i) : 0, borrow);
                borrow = borr;
                if (i < N) {
                    this->_at(i) = diff;
                }
            }
            return borrow;
        }

        friend std::ostream& operator<<(std::ostream& os, const unints& val) noexcept {
            std::string str;
            for (std::size_t i = val.size() - 1; ~i; i--) {
                str.push_back(val[i] + '0');
            }
            os << str;
            return os;
        }
    };

}