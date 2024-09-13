#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <ostream>
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
    };

}