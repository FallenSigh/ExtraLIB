#pragma once
#include <algorithm>
#include <format>
#include <initializer_list>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "details/print_tuple.h"
#include "details/ndarray.h"
#include "log.h"

namespace exlib {
    template <typename ShapeType, class Backend, class Allocator>
    struct ndarray {
        static_assert(is_shape_v<ShapeType>);

        inline static constexpr auto shape = ShapeType::value;
        inline static constexpr std::size_t block_size = ShapeType::block_size;
        inline static constexpr std::size_t N = ShapeType::first;
        
        using shape_type = ShapeType;
        using backend_type = Backend;
        using one_dim = std::false_type;
        using data_type = ndarray<typename ShapeType::next_shape, Backend>;
        using array_type = std::conditional_t<std::is_void_v<Allocator>, details::static_array<data_type, N>, details::dynamic_array<data_type, N, typename details::rebind<data_type, Allocator>::type>>;

        using iterator = typename array_type::iterator;
        using const_iterator = typename array_type::const_iterator;
        using reverse_iterator = typename array_type::reverse_iterator;
        using const_reverse_iterator = typename array_type::const_reverse_iterator;
        
        using self_type = ndarray<ShapeType, Backend>;
        using reference = ndarray<ShapeType, Backend>&;
        using const_reference = const ndarray<ShapeType, Backend>&;

        array_type data;

        ndarray() noexcept {

        }

        ndarray(std::initializer_list<data_type> s) noexcept {
            for (std::size_t i = 0; i < s.size() && i < N; i++) {
                data[i] = *(s.begin() + i);
            }
        }

        template <std::ranges::input_range Range>
        ndarray(Range in) noexcept {
            std::size_t chunk_size = std::min(in.size(), block_size);
            std::size_t M = std::min(in.size(), shape_type::size);
            auto it = in.begin();
            std::size_t total_size = 0;
            for (std::size_t i = 0; i < N; i++) {
                data[i] = std::ranges::subrange(it, it + chunk_size);
                it += chunk_size;
                total_size += chunk_size;
                chunk_size = std::min(block_size, M - total_size);
                if (chunk_size == 0) {
                    break;
                }
            }
        }

        void _flatten(std::vector<backend_type>& v) const noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i]._flatten(v);
            }
        }

        ndarray<exlib::shape<shape_type::size>> flatten() const noexcept {
            ndarray<exlib::shape<shape_type::size>> res;
            std::vector<backend_type> t;
            this->_flatten(t);
            res = t;
            return res;
        }

        template <typename Shape>
        requires is_shape_v<Shape>
        ndarray<Shape, Backend> reshape() const noexcept {
            static_assert(N * block_size == Shape::size);
            ndarray<Shape, Backend> res;
            std::vector<backend_type> t;
            this->_flatten(t);
            res = t;
            return res;
        };

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

        self_type operator+(const data_type& other) noexcept {
            self_type copy = *this;
            copy += other;
            return copy;
        }

        self_type operator-(const data_type& other) noexcept {
            self_type copy = *this;
            copy -= other;
            return copy;
        }

        self_type operator*(const data_type& other) noexcept {
            self_type copy = *this;
            copy *= other;
            return copy;
        }

        self_type operator/(const data_type& other) noexcept {
            self_type copy = *this;
            copy /= other;
            return copy;
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
