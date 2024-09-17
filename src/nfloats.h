#pragma once
#include <bitset>
#include <cmath>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <type_traits>

#include "log.h"
#include "integer.h"

namespace exlib {
    template<std::size_t n_mantissa, class Exponent = int>
    struct nfloats;

    template<typename F>
    struct mantissa_bit {    };

    template<>
    struct mantissa_bit<float> : public std::integral_constant<std::size_t, 23> {};

    template<>
    struct mantissa_bit<double> : public std::integral_constant<std::size_t, 52> {};

    template<typename T>
    struct is_floating : std::false_type {};

    template<std::size_t N, class Exponent>
    struct is_floating<nfloats<N, Exponent>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_floating_v = is_integer<T>::value;
}

namespace exlib {
    template<std::size_t n_fraction, class Exponent>
    struct nfloats {
        static_assert(std::is_integral_v<Exponent>);;
        
        using reference = nfloats<n_fraction, Exponent>&;
        using const_reference = const nfloats<n_fraction, Exponent>&;
        using self_type = nfloats<n_fraction, Exponent>;
        using exponent_type = Exponent;
        inline static constexpr std::size_t extra_bits = 4;
        using mantissa_type = unints<n_fraction + extra_bits>;
        inline static constexpr std::size_t mantissa_size = n_fraction + extra_bits;
        inline static constexpr std::size_t digits10 = std::floor(std::log10(2) * n_fraction);
        inline static exponent_type min_exponent_limits = std::numeric_limits<exponent_type>::min() + 2 * static_cast<exponent_type>(n_fraction);
        inline static exponent_type max_exponent_limits = std::numeric_limits<exponent_type>::max() - 2 * static_cast<exponent_type>(n_fraction);
        inline static exponent_type exponent_zero = max_exponent_limits + 1;
        inline static exponent_type exponent_nan = max_exponent_limits + 2;
        inline static exponent_type exponent_inf = max_exponent_limits + 3;

        bool _sign;
        mantissa_type _mantissa;
        exponent_type _exponent;

        inline static const auto& nan() noexcept {
            static nfloats res;
            res._exponent = exponent_nan;
            return res;
        }

        inline static const auto& inf() noexcept {
            static nfloats res;
            res._exponent = exponent_inf;
            return res;
        }

        inline static const auto& zero() noexcept {
            static nfloats res;
            res._exponent = exponent_zero;
            return res;
        }

        nfloats() noexcept {
            _sign = static_cast<bool>(0);
            _exponent = static_cast<exponent_type>(0);
            _mantissa = 0;
        }

        template <class I>
        requires exlib::is_integer_v<I>
        nfloats(const I& i) noexcept {
            this->assign_exint(i);
        }

        template <typename F>
        requires std::is_floating_point_v<F>
        nfloats(const F& f) noexcept {
            this->assign_float(f);
        }

        nfloats(const_reference other) noexcept {
            _sign = other._sign;
            _exponent = other._exponent;
            _mantissa = other._mantissa;
        }

        template <typename F>
        requires std::is_floating_point_v<F>
        reference operator=(const F& f) noexcept {
            return assign_float(f);
        }

        template <typename I>
        requires std::is_integral_v<I>
        reference operator=(const I& i) noexcept {
            return assign_int(i);
        }

        template <typename I>
        requires exlib::is_integer_v<I>
        reference operator=(const I& i) noexcept {
            return assign_exint(i);
        }

        reference operator=(const_reference other) noexcept {
            _sign = other._sign;
            _exponent = other._exponent;
            _mantissa = other._mantissa;
            return *this;
        }

        template <typename I>
        requires std::is_integral_v<I>
        reference assign_int(I i) noexcept {
            _sign = std::signbit(i);
            if (_sign) {
                i = std::abs(i);
            }   
            constexpr std::size_t M = sizeof(i) * byte_size;
            std::size_t len = 0;
            for (std::size_t j = M - 1; ~j; j--) {
                if (i >> j & 1) {
                    len = j;
                    break;
                }
            }

            _exponent = len;
            if (M >= mantissa_size) {
                if (len >= mantissa_size) {
                    _mantissa = (i >> (len + 1- mantissa_size));
                } else {
                    _mantissa = (i << (mantissa_size - len - 1));
                }
            } else {
                _mantissa = mantissa_type(i) << (mantissa_size - len - 1);
            } 
            return *this;
        }

        template<typename I>
        requires exlib::is_integer_v<I>
        reference assign_exint(I i) noexcept {
            _sign = i.template sign();
            if (_sign) {
                i = i.abs();
            }

            std::size_t len = 0;
            for (std::size_t j = i.template size() - 1; ~j; j--) {
                if (i[j]) {
                    len = j;
                    break;
                }
            }

            _exponent = len;
            if (i.template size() >= mantissa_size) {
                if (len >= mantissa_size) {
                    _mantissa = (i >> (len + 1- mantissa_size));
                } else {
                    _mantissa = (i << (mantissa_size - len - 1));
                }
            } else {
                _mantissa = mantissa_type(i) << (mantissa_size - len - 1);
            } 
            return *this;
        }

        template <typename F>
        requires std::is_floating_point_v<F>
        reference assign_float(F f) noexcept {
            _sign = std::signbit(f);

            if (std::isinf(f)) {
                _exponent = exponent_inf;
            } else if (std::isnan(f)) {
                _exponent = exponent_nan;
            } else if (f == static_cast<F>(0)) {
                _exponent = exponent_zero;
            } else {
                int exp;
                F frac = std::frexp(f, &exp);

                frac *= 2;
                exp -= 1;

                _exponent = static_cast<exponent_type>(exp);
                _mantissa = *(std::uint64_t*)(&frac);
                if (n_fraction > mantissa_bit<F>::value) {
                    _mantissa <<= (n_fraction - mantissa_bit<F>::value);
                }
                _mantissa <<= extra_bits - 1;
                _mantissa[mantissa_size - 1] = 1;
            }
            return *this;
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

        self_type operator-() const noexcept {
            self_type res = *this;
            res._sign = !res._sign;
            return res;
        }

        reference operator*=(const_reference other) noexcept {
            if ((isnan() || other.isnan()) || (isinf() && other.iszero()) || (other.isinf() && iszero())) {
                *this = nan();
                return *this;
            }
            if (isinf() || other.isinf()) {
                bool neg = _sign != other._sign;
                *this = inf();
                this->_sign = neg;
                return *this;
            }

            if (iszero() || other.iszero()) {
                *this = zero();
                return *this;
            }

            _mantissa = (unints<mantissa_size * 2>(_mantissa) * other._mantissa) >> (mantissa_size - 1);
            _exponent += other._exponent;
            _sign ^= other._sign;

            if (_exponent >= max_exponent_limits) {
                *this = inf();
            }

            if (_exponent <= min_exponent_limits) {
                *this = zero();
            }
            return *this;
        }

        reference operator/=(const_reference other) noexcept {
            if (iszero()) {
                if (other.isnan()) {
                    *this = other;
                    return *this;
                } else if (other.iszero()) {
                    *this = nan();
                    return *this;
                }
            }
            
            _sign ^= other._sign;
            _exponent -= other._exponent;
            _mantissa = ((unints<mantissa_size * 2>(_mantissa) << (mantissa_size - 1)) / other._mantissa);
            
            if (_exponent >= max_exponent_limits) {
                *this = inf();
            }

            if (_exponent <= min_exponent_limits) {
                *this = zero();
            }

            return *this;
        }

        reference operator-=(const_reference other) noexcept {
            *this += -other;
            return *this;
        }

        reference operator+=(const_reference other) noexcept {
            if (isnan()) {
                return *this;
            }

            if (isinf()) {
                if (other.isinf() && _sign != other._sign) {
                    *this = nan();
                }
                return *this;
            }

            if (iszero()) {
                *this = other;
                return *this;
            }

            if (other.isnan() || other.isinf()) {
                *this = other;
                return *this;
            }
            
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
                    bool back = _mantissa[0];
                    _mantissa >>= 1;
                    if (back && _mantissa[0]) { 
                        _mantissa++;
                    } else if (back && !_mantissa[0]) {
                        _mantissa[0] = 1;
                    }
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
                    log_warning("TODO: borrow!!!");
                }
            }

            if (_exponent >= max_exponent_limits) {
                *this = inf();
            }
            if (_exponent <= min_exponent_limits) {
                *this = zero();
            }

            return *this;
        }

        self_type operator/(const_reference other) noexcept {
            auto copy = *this;
            copy /= other;
            return copy;
        }

        self_type operator*(const_reference other) const noexcept {
            auto copy = *this;
            copy *= other;
            return copy;
        }

        bool operator==(const_reference other) const noexcept {
            return _sign == other._sign && _exponent == other._exponent && _mantissa == other._mantissa;
        }

        self_type abs() const noexcept {
            self_type res = *this;
            res._sign = 0;
            return res;
        }

        bool operator<(const_reference other) const noexcept {
            if (_sign != other._sign) {
                return _sign == 0;
            }

            if (_exponent != other._exponent) {
                return _exponent < other._exponent;
            }

            return _mantissa < other._mantissa;
        }

        bool operator>(const_reference other) const noexcept {
            if (_sign != other._sign) {
                return _sign == 0;
            }

            if (_exponent != other._exponent) {
                return _exponent > other._exponent;
            }

            return _mantissa > other._mantissa;
        }

        bool operator<=(const_reference other) const noexcept {
            return *this < other || *this == other;
        }
        
        bool operator>=(const_reference other) const noexcept {
            return *this > other || *this == other;
        }

        nints<mantissa_size> trunc() const noexcept {
            std::string s = _mantissa.bin();
            std::string i = s.substr(0, _exponent + 1);
            if (_sign) i = "-" + i;
            return nints<mantissa_size>().template rd_bin_string(i);
        }

        std::string str(std::size_t precision = 0) const noexcept {
            std::string s = _mantissa.bin();
            std::string i = s.substr(0, _exponent + 1);
            std::string m = s.substr(_exponent + 1, s.size() - (_exponent + 1));
            
            unints<n_fraction + extra_bits> ii, mm;
            ii.rd_bin_string(i);
            mm.rd_bin_string(m);
            mm <<= (n_fraction + extra_bits - m.size());

            std::string ipart = ii.str();
            std::string fpart = mm.as_mantissa_str();
            std::string neg = (_sign == 0) ? "" : "-";
            if (precision != 0) {
                fpart = fpart.substr(0, std::min(precision, fpart.size()));
            }
            return neg + ipart + "." + fpart;
        }

        std::string bin() const noexcept {
            std::string res;
            res.push_back(_sign + '0');
            res += " ";
            res += std::bitset<sizeof(exponent_type) * byte_size>(_exponent).to_string();
            res += " ";
            res += _mantissa.bin();
            return res;
        }

        double to_double() const noexcept {
            return (_sign == 0 ? 1 : -1) * (static_cast<double>(static_cast<std::uint64_t>(_mantissa)) / std::pow(2.0, n_fraction + extra_bits - 1 - _exponent));
        }

        bool isinf() const noexcept {
            return _exponent == exponent_inf;
        }

        bool isnan() const noexcept {
            return _exponent == exponent_nan;
        }

        bool iszero() const noexcept {
            return _exponent == exponent_zero;
        }

        friend std::ostream& operator<<(std::ostream& os, const nfloats& val) noexcept {
            os << val.str();
            return os;
        }
    };
}

template<std::size_t N, class Exponent>
struct std::formatter<exlib::nfloats<N, Exponent>> : std::formatter<std::string> {
    auto format(const auto& f, auto& ctx) const {
        return std::format_to(ctx.out(), "{}", f.str());
    }
};