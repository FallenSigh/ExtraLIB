#pragma once
#include <algorithm>
#include <array>
#include <initializer_list>
#include <sstream>
#include <type_traits>

namespace exlib {
    template <class Backend, std::size_t N, std::size_t ...Shape>
    struct ndarray;

    template <typename T>
    struct is_ndarray : std::false_type {};

    template <class Backend, std::size_t N, std::size_t ...Shape>
    struct is_ndarray<ndarray<Backend, N, Shape...>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_ndarray_v = is_ndarray<T>::value;
}

namespace exlib {

    template<class Backend, std::size_t N>
    struct ndarray<Backend, N> {
        using backend_type = Backend;
        using one_dim = std::true_type;
        using array_type = std::array<Backend, N>;
        using data_type = Backend;
        
        using iterator = typename array_type::iterator;
        using const_iterator = typename array_type::const_iterator;
        using reverse_iterator = typename array_type::reverse_iterator;
        using const_reverse_iterator = typename array_type::const_reverse_iterator;
        
        using self_type = ndarray<Backend, N>;
        using reference = ndarray<Backend, N>&;
        using const_reference = const ndarray<Backend, N>&;

        inline static constexpr std::tuple<std::size_t> shape = {N};
        inline static constexpr std::size_t block_size = 1;

        array_type data;

        ndarray() noexcept {
            data.fill(static_cast<Backend>(0));
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

        iterator begin() noexcept {
            return data.begin();
        }

        iterator end() noexcept {
            return data.end();
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
                ss << data[i] << ", ";
            }
            if (N > 0) ss << data[N - 1];
            ss << "]";
        }
    };
}

