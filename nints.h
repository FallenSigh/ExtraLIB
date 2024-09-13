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