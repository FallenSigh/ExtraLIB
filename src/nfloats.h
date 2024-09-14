#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "log.h"
#include "integer.h"

namespace exlib {
    template<std::size_t n_mantissa, class Exponent>
    struct nfloats;

    template<typename F>
    struct mantissa_bit {    };

    template<>
    struct mantissa_bit<float> : public std::integral_constant<std::size_t, 23> {};

    template<>
    struct mantissa_bit<double> : public std::integral_constant<std::size_t, 52> {};

    template<std::size_t n_mantissa, class Exponent>
    struct nfloats {
        using reference = nfloats<n_mantissa, Exponent>&;
        using const_reference = const nfloats<n_mantissa, Exponent>&;
        using self_type = nfloats<n_mantissa, Exponent>;
        using exponent_type = Exponent;
        inline static constexpr std::size_t extra_bits = 8;
        using mantissa_type = unints<n_mantissa + extra_bits>;
        inline static std::size_t mantissa_size = n_mantissa + extra_bits;
        inline static exponent_type min_exponent_limits = std::numeric_limits<exponent_type>::min() + 2 * static_cast<exponent_type>(n_mantissa);
        inline static exponent_type max_exponent_limits = std::numeric_limits<exponent_type>::max() - 2 * static_cast<exponent_type>(n_mantissa);
        inline static exponent_type exponent_zero = max_exponent_limits + 1;
        inline static exponent_type exponent_nan = max_exponent_limits + 2;
        inline static exponent_type exponent_inf = max_exponent_limits + 3;

        bool _sign;
        mantissa_type _mantissa;
        exponent_type _exponent;

        template<typename F>
        requires std::is_floating_point_v<F>
        nfloats(const F& f) noexcept {
            _sign = std::signbit(f);
            int exp;
            F frac = std::frexp(f, &exp);

            frac *= 2;
            exp -= 1;

            _exponent = static_cast<exponent_type>(exp);
            _mantissa = *(std::uint64_t*)(&frac);
            if (n_mantissa > mantissa_bit<F>::value) {
                _mantissa <<= (n_mantissa - mantissa_bit<F>::value);
            }
            _mantissa <<= extra_bits - 1;
            _mantissa[mantissa_size - 1] = 1;
        }

        self_type operator+(const_reference other) const noexcept {
            self_type res = *this;
            res += other;
            return res;
        }

        self_type operator-(const_reference other) const noexcept {
            self_type res = *this;
            res -= other;
            return res;
        }

        reference operator*=(const_reference other) noexcept {
            _mantissa *= other._mantissa;
            _exponent *= other._exponent;
            _sign *= other._sign;
            return *this;
        }

        reference operator-=(const_reference other) noexcept {
            exponent_type diff = _exponent - other._exponent;
            if (_sign != other._sign) {
                bool carry = 0;
                auto copy = other._mantissa;
                if (diff <= 0) {
                    _mantissa >>= std::abs(diff);
                    _exponent = other._exponent;
                } else {
                    copy >>= diff;
                }
                carry = _mantissa.bitwise_add_assign(copy);

                if (carry) {
                    _mantissa >>= 1;
                    _mantissa[mantissa_size - 1] = 1;
                    _exponent++;
                }
            } else {
                bool borrow = 0;
                auto copy = other._mantissa;
                if (diff <= 0) {
                    _mantissa >>= std::abs(diff);
                    _exponent = other._exponent;
                } else {
                    copy >>= std::abs(diff);
                }
                if (_mantissa < copy) {
                    _mantissa.swap(copy);
                    _sign ^= !other._sign;
                }
                borrow = _mantissa.bitwise_sub_assign(copy);
            
                if (borrow) {
                    log_debug("TODO: borrow!!!");
                }
            }

            return *this;
        }

        reference operator+=(const_reference other) noexcept {
            exponent_type diff = _exponent - other._exponent;
            if (_sign == other._sign) {
                bool carry = 0;
                auto copy = other._mantissa;
                if (diff <= 0) {
                    _mantissa >>= std::abs(diff);
                    _exponent = other._exponent;
                } else {
                    copy >>= diff;
                }
                carry = _mantissa.bitwise_add_assign(copy);

                if (carry) {
                    _mantissa >>= 1;
                    _mantissa[mantissa_size - 1] = 1;
                    _exponent++;
                }
            } else {
                bool borrow = 0;
                auto copy = other._mantissa;
                if (diff <= 0) {
                    _mantissa >>= std::abs(diff);
                    _exponent = other._exponent;
                } else {
                    copy >>= std::abs(diff);
                }
                if (_mantissa < copy) {
                    _mantissa.swap(copy);
                    _sign = other._sign;
                }
                borrow = _mantissa.bitwise_sub_assign(copy);
            
                if (borrow) {
                    log_debug("TODO: borrow!!!");
                }
            }

            return *this;
        }

        std::string to_string() const noexcept {
            std::string res;
            res.push_back(_sign + '0');
            res += " ";
            res += std::bitset<sizeof(exponent_type) * 8>(_exponent).to_string();
            res += " ";
            res += _mantissa.bin_str();
            return res;
        }

        template<typename F>
        F convert_to() const noexcept {
            return (_sign == 0 ? 1 : -1) * (static_cast<F>(_mantissa.to_ullong()) / std::pow(2.0, n_mantissa + extra_bits - 1 - _exponent));
        }


    };
}