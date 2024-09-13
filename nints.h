#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "details/int_base.h"

// N bit signed integer
namespace exlib {
    template<std::size_t N, class Word = unsigned char, class Allocator = void>
    struct nints : public int_base<N, Word, Allocator, true>{
        static_assert(std::is_unsigned_v<Word>);;
        using base_class_type = int_base<N, Word, Allocator, true>;
        
        using iterator = base_class_type::iterator;
        using const_iterator = base_class_type::const_iterator;
        using bit_reference = base_class_type::bit_reference;
        using const_bit_reference = base_class_type::const_bit_reference;
        using reference = nints<N, Word, Allocator>&;
        using const_reference = const nints<N, Word, Allocator>&;
        using self_type = nints<N, Word, Allocator>;

        using word_type = Word;
        using bit = bool;
        using pointer = word_type*;

        nints() noexcept {
        }

        nints(const base_class_type& base) : base_class_type(base) {

        }

        nints(base_class_type&& base) : base_class_type(base) {

        }

        template<std::size_t M>
        nints(const nints<M>& other) noexcept {
            // 保留符号位截断
            this->reset(other.sign() == 0 ? 0 : -1);
            for (std::size_t i = 0; i < std::min(N, M) - 1; i++) {
                this->_at(i) = other[i];
            }
        }

        template<typename I>
        requires std::is_integral_v<I>
        nints(const I& i) noexcept {
            this->assign_integral(i);
        }

        template<typename I>
        requires std::is_integral_v<I>
        reference operator=(const I& i) noexcept {
            this->assign_integral(i);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator>
        nints<std::max(N, M), Word, Allocator> operator*(const nints<M, OWord, OAllocator>& other) const noexcept {
            auto lhs_abs = nints<std::max(N, M), Word, Allocator>(this->abs());
            auto rhs_abs = other.abs();

            nints<std::max(N, M), Word, Allocator> res = 0;
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
        nints<std::max(N, sizeof(T) * byte_size), Word, Allocator> operator*(const T& val) const noexcept {
            return *this * nints<sizeof(T) * byte_size, Word, Allocator>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N), Word, Allocator> operator*(const T& lhs, const nints<N>& rhs) noexcept {
            return nints<sizeof(T) * byte_size, Word, Allocator>(lhs) * rhs;
        }

        template<std::size_t M, class OWord, class OAllocator>
        nints<std::max(N, M), Word, Allocator> operator/(const nints<M, OWord, OAllocator>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            nints<M, OWord, OAllocator> rhs_abs = other.abs();

            nints<std::max(N, M), Word, Allocator> quotient = 0;
            nints<std::max(N, M), Word, Allocator> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                    quotient[N - 1 - i] = 1;
                }
            }

            if (this->sign() != other.sign()) {
                quotient = ~quotient + nints<std::max(N, M), Word, Allocator>(1);
            }

            return quotient;
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<std::max(N, sizeof(T) * byte_size), Word, Allocator> operator/(const T& val) const {
            return *this / nints<sizeof(T) * byte_size, Word, Allocator>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N), Word, Allocator> operator/(const T& lhs, const nints<N>& rhs) noexcept {
            return nints<sizeof(T) * byte_size, Word, Allocator>(lhs) / rhs;
        }

        template<std::size_t M, class OWord, class OAllocator>
        nints<std::max(N, M), Word, Allocator> operator%(const nints<M, OWord, OAllocator>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            nints<M, OWord, OAllocator> rhs_abs = other.abs();

            nints<std::max(N, M), Word, Allocator> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder -= rhs_abs;
                }
            }

            if (this->sign() != remainder.sign()) {
                remainder = ~remainder + nints<std::max(N, M), Word, Allocator>(1);
            }

            return remainder;
        }

        template<typename T>
        requires std::is_integral_v<T>
        nints<std::max(N, sizeof(T) * byte_size), Word, Allocator> operator%(const T& val) const {
            return *this % nints<sizeof(T) * byte_size, Word, Allocator>(val);
        }

        template<typename T>
        requires std::is_integral_v<T>
        friend nints<std::max(sizeof(T) * byte_size, N), Word, Allocator> operator%(const T& lhs, const nints<N>& rhs) noexcept {
            return nints<sizeof(T) * byte_size, Word, Allocator>(lhs) % rhs;
        }


        template<std::size_t M, class OWord, class OAllocator>
        reference operator*=(const nints<M, OWord, OAllocator>& other) noexcept {
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
        nints<N>& operator*=(const T& val) noexcept {
            *this *= nints<sizeof(T) * byte_size, Word, Allocator>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator>
        reference operator/=(const nints<M, OWord, OAllocator>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }
            auto lhs_abs = this->abs();
            nints<M, OWord, OAllocator> rhs_abs = other.abs();

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
                quotient = ~quotient + nints<std::max(N, M), Word, Allocator>(1);
            }

            *this = std::move(quotient);
            return *this;
        }

        template<typename T>
        requires std::is_integral_v<T>
        reference operator/=(const T& val) {
            *this /= nints<sizeof(T) * byte_size, Word, Allocator>(val);
            return *this;
        }

        template<std::size_t M, class OWord, class OAllocator>
        reference operator%=(const nints<M, OWord, OAllocator>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = this->abs();
            nints<M, OWord, OAllocator> rhs_abs = other.abs();

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
            *this %= nints<sizeof(T) * byte_size, Word, Allocator>(val);
            return *this;
        }

        template<typename T>
        requires std::is_integral_v<T>
        operator T() const noexcept {
            T base = static_cast<T>(1);
            T res = static_cast<T>(0);

            for (std::size_t i = 0; i < N - 1; i++) {
                res += base * this->_at(i);
                base *= 2;
            }

            res -= base * this->_at(N - 1);
            return res;
        }
    };

}