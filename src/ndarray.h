#pragma once
#include <algorithm>
#include <format>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <array>
#include <type_traits>

#include "details/ndarray.h"
#include "details/print_tuple.h"
#include "log.h"

#define UNUSED(t)

namespace exlib {
    namespace details {
        template <typename ...Args>
        consteval std::size_t mul(Args... args) {
            return (args * ...);
        }
    }
}

namespace exlib {
    template <class Backend, std::size_t N, std::size_t ...Shape>
    struct ndarray {
        using backend_type = Backend;
        using one_dim = std::false_type;
        using array_type = std::array<ndarray<Backend, Shape...>, N>;
        using data_type = ndarray<Backend, Shape...>;
        
        using iterator = typename array_type::iterator;
        using const_iterator = typename array_type::const_iterator;
        using reverse_iterator = typename array_type::reverse_iterator;
        using const_reverse_iterator = typename array_type::const_reverse_iterator;
        
        using self_type = ndarray<Backend, N, Shape...>;
        using reference = ndarray<Backend, N, Shape...>&;
        using const_reference = const ndarray<Backend, N, Shape...>&;
        
        inline static constexpr std::tuple<std::size_t, std::decay_t<decltype(Shape)>...> shape = {N, Shape...};
        inline static constexpr std::size_t block_size = details::mul(Shape...);

        array_type data;

        ndarray() noexcept {

        }

        ndarray(std::initializer_list<data_type> s) noexcept {
            for (std::size_t i = 0; i < s.size() && i < N; i++) {
                data[i] = *(s.begin() + i);
            }
        }

        reference operator+=(const data_type& other) noexcept {
            std::ranges::transform(data, data.begin(), [&other](auto& elem) {
                elem += other;
                return elem;
            });
            return *this;
        }

        reference operator-=(const data_type& other) noexcept {
            std::ranges::transform(data, data.begin(), [&other](auto& elem) {
                elem -= other;
                return elem;
            });
            return *this;
        }

        reference operator*=(const data_type& other) noexcept {
            std::ranges::transform(data, data.begin(), [&other](auto& elem) {
                elem *= other;
                return elem;
            });
            return *this;
        }

        reference operator/=(const data_type& other) noexcept {
            std::ranges::transform(data, data.begin(), [&other](auto& elem) {
                elem /= other;
                return elem;
            });
            return *this;
        }

        reference operator+=(const_reference other) noexcept {
            std::ranges::transform(data, other.data, data.begin(), [](auto& lhs, auto& rhs){
                lhs += rhs;
                return lhs;
            });
            return *this;
        }

        reference operator-=(const_reference other) noexcept {
            std::ranges::transform(data, other.data, data.begin(), [](auto& lhs, auto& rhs){
                lhs -= rhs;
                return lhs;
            });
            return *this;
        }

        reference operator*=(const_reference other) noexcept {
            std::ranges::transform(data, other.data, data.begin(), [](auto& lhs, auto& rhs){
                lhs *= rhs;
                return lhs;
            });
            return *this;
        }

        reference operator/=(const_reference other) noexcept {
            std::ranges::transform(data, other.data, data.begin(), [](auto& lhs, auto& rhs){
                lhs /= rhs;
                return lhs;
            });
            return *this;
        }

        self_type operator+(const_reference other) noexcept {
            self_type copy = *this;
            copy += other;
            return copy;
        }

        self_type operator-(const_reference other) noexcept {
            self_type copy = *this;
            copy -= other;
            return copy;
        }

        self_type operator*(const_reference other) noexcept {
            self_type copy = *this;
            copy *= other;
            return copy;
        }

        self_type operator/(const_reference other) noexcept {
            self_type copy = *this;
            copy /= other;
            return copy;
        }

        const_iterator cbegin() const noexcept {
            return data.cbegin();
        }

        const_iterator cend() const noexcept {
            return data.cend();
        }

        const_reverse_iterator crbegin() const noexcept {
            return data.crbegin();
        }

        const_reverse_iterator crend() const noexcept {
            return data.crend();
        }

        iterator begin() noexcept {
            return data.begin();
        }

        iterator end() noexcept {
            return data.end();
        }

        reverse_iterator rbegin() noexcept {
            return data.rbegin();
        }

        reverse_iterator rend() noexcept {
            return data.rend();
        }

        data_type& at(std::size_t idx) noexcept {
            if (idx >= N) {
                throw std::out_of_range("index out of range!");
            }
            return data[idx];
        }

        const data_type& at(std::size_t idx) const noexcept {
            if (idx >= N) {
                throw std::out_of_range("index out of range!");
            }
            return data[idx];
        }

        data_type& operator[](std::size_t idx) noexcept {
            return data[idx];
        }

        const data_type& operator[](std::size_t idx) const noexcept {
            return data[idx];
        }

        friend std::ostream& operator<<(std::ostream& os, const_reference val) noexcept {
            std::stringstream ss;
            val.print(ss);
            os << ss.str();
            return os;
        }

        void print(std::stringstream& ss) const noexcept {
            ss << "[";
            for (std::size_t i = 0; i < N - 1; i++) {
                data[i].print(ss);
                ss << ", ";
            }
            if (N > 0) data[N - 1].print(ss);
            ss << "]";
        }
    };
}

template <typename T>
requires exlib::is_ndarray_v<T>
struct std::formatter<T> : std::formatter<std::string> {
    auto format(const auto& arr, auto& ctx) const {
        std::stringstream ss;
        arr.print(ss);
        return std::format_to(ctx.out(), "{}", ss.str());
    }
};
