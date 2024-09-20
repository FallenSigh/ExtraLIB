#pragma once
#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <initializer_list>
#include <optional>
#include <queue>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "details/print_tuple.h"
#include "details/ndarray.h"
#include "log.h"

namespace exlib {
    template <typename Shape, class DType, class Allocator>
    struct ndarray {
        static_assert(is_shape_v<Shape>);

        inline static constexpr auto shape = Shape::value;
        inline static constexpr std::size_t block_size = Shape::block_size;
        inline static constexpr std::size_t N = Shape::first;
        
        using shape_type = Shape;
        using dtype = DType;
        using one_dim = std::false_type;
        using data_type = ndarray<typename Shape::next_shape_type, DType>;
        using value_type = data_type;
        using array_type = std::conditional_t<std::is_void_v<Allocator>, details::static_array<data_type, N>, details::dynamic_array<data_type, N, typename details::rebind<data_type, Allocator>::type>>;

        using iterator = typename array_type::iterator;
        using const_iterator = typename array_type::const_iterator;
        using reverse_iterator = typename array_type::reverse_iterator;
        using const_reverse_iterator = typename array_type::const_reverse_iterator;
        
        using self_type = ndarray<Shape, DType>;
        using reference = ndarray<Shape, DType>&;
        using const_reference = const ndarray<Shape, DType>&;

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
            this->assign(in);
        }

        ndarray(const data_type& other) noexcept {
            data.fill(other);
        }

        ndarray(self_type&& other) noexcept {
            if (&other != this) {
                std::move(other.data.begin(), other.data.end(), data.begin());
            }
        }

        ndarray(const_reference other) noexcept {
            std::copy(other.data.begin(), other.data.end(), data.begin());
        }

        template <std::ranges::input_range Range>
        requires (!is_ndarray_v<Range>)
        reference operator=(Range in) noexcept {
            return this->assign(in);
        }

        reference operator=(self_type&& other) noexcept {
            if (&other != this) {
                std::move(other.data.begin(), other.data.end(), data.begin());
            }
            return *this;
        }

        reference operator=(const_reference other) noexcept {
            std::copy(other.data.begin(), other.data.end(), data.begin());
            return *this;
        }

        template <std::ranges::input_range Range>
        requires (!is_ndarray_v<Range>)
        reference assign(Range in) noexcept {
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
            return *this;
        }

        void _flatten(std::vector<dtype>& v) const noexcept {
            std::ranges::for_each(data, [&v](auto& elem){ elem._flatten(v); });
        }

        ndarray<exlib::shape<shape_type::size>> flatten() const noexcept {
            ndarray<exlib::shape<shape_type::size>> res;
            std::vector<dtype> t;
            this->_flatten(t);
            res = t;
            return res;
        }

        auto operator[](const ndarray<shape_type, bool>& cond) noexcept {
            std::vector<dtype> res;
            for (std::size_t i = 0; i < N; i++) {
                auto t = data[i][cond.data[i]];
                res.insert(res.end(), t.begin(), t.end());
            }
            return res;
        }

        template <typename T = dtype>
        T sum() const noexcept {
            T res = 0;      
            for (std::size_t i = 0; i < N; i++) {
                res += data[i].sum();
            }
            return res;
        }

        reference pow(const dtype& value) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i].pow(value);
            }
            return *this;
        }

        reference exp() noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i].exp();
            }
            return *this;
        }

        reference log() noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i].log();
            }
            return *this;
        }

        template <typename T = dtype>
        T mean() const noexcept {
            return sum<T>() / static_cast<T>(shape_type::size);
        }

        void fill(const dtype& value) noexcept {
            std::ranges::for_each(data, [&value](auto& elem){ elem.fill(value); });
        }

        template <typename OShape, class ODtype = dtype>
        requires is_shape_v<OShape>
        ndarray<OShape, ODtype> reshape() const noexcept {
            static_assert(N * block_size == OShape::size);
            ndarray<OShape, ODtype> res;
            std::vector<dtype> t;
            this->_flatten(t);
            res = t;
            return res;
        };

        static constexpr std::size_t size() noexcept {
            return N * block_size;
        }

        ndarray<shape_type, bool> operator>(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] > (val));
            }
            return res;
        }

        ndarray<shape_type, bool> operator<(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] < (val));
            }
            return res;
        }

        ndarray<shape_type, bool> operator==(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] == (val));
            }
            return res;
        }
        
        ndarray<shape_type, bool> operator!=(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] != (val));
            }
            return res;
        }

        ndarray<shape_type, bool> operator>=(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] >= (val));
            }
            return res;
        }

        ndarray<shape_type, bool> operator<=(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] <= (val));
            }
            return res;
        }

        self_type put_mask(const ndarray<shape_type, bool>& cond, const dtype& value) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i].put_mask(cond.data[i], value);
            }
            return *this;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        reference operator+=(const T& val) noexcept {
            std::ranges::for_each(data, [&val](auto& elem){ elem += val; });
            return *this;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        reference operator-=(const T& val) noexcept {
            std::ranges::for_each(data, [&val](auto& elem){ elem -= val; });
            return *this;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        reference operator*=(const T& val) noexcept {
            std::ranges::for_each(data, [&val](auto& elem){ elem *= val; });
            return *this;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        reference operator/=(const T& val) noexcept {
            std::ranges::for_each(data, [&val](auto& elem){ elem /= val; });
            return *this;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        reference operator%=(const T& val) noexcept {
            std::ranges::for_each(data, [&val](auto& elem){ elem %= val; });
            return *this;
        }

        void left_ops(auto& lhs, auto&& op) {
            for (std::size_t i = 0; i < N; i++) {
                data[i].left_ops(lhs, op);
            }
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        friend auto operator+(const T& lhs, const_reference rhs) noexcept {
            self_type res = rhs;
            using type = std::common_type_t<std::decay_t<T>, dtype>;
            res.left_ops(lhs, std::plus<type>());
            return res;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        friend auto operator-(const T& lhs, const_reference rhs) noexcept {
            self_type res = rhs;
            using type = std::common_type_t<std::decay_t<T>, dtype>;
            res.left_ops(lhs, std::minus<type>());
            return res;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        friend auto operator/(const T& lhs, const_reference rhs) noexcept {
            self_type res = rhs;
            using type = std::common_type_t<std::decay_t<T>, dtype>;
            res.left_ops(lhs, std::divides<type>());
            return res;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        friend auto operator*(const T& lhs, const_reference rhs) noexcept {
            self_type res = rhs;
            using type = std::common_type_t<std::decay_t<T>, dtype>;
            res.left_ops(lhs, std::multiplies<type>());
            return res;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        friend auto operator%(const T& lhs, const_reference rhs) noexcept {
            self_type res = rhs;
            using type = std::common_type_t<std::decay_t<T>, dtype>;
            res.left_ops(lhs, std::modulus<type>());
            return res;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        auto operator+(const T& val) noexcept {
            self_type copy = *this;
            copy += val;
            return copy;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        auto operator-(const T& val) noexcept {
            self_type copy = *this;
            copy -= val;
            return copy;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        auto operator*(const T& val) noexcept {
            self_type copy = *this;
            copy *= val;
            return copy;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        auto operator/(const T& val) noexcept {
            self_type copy = *this;
            copy /= val;
            return copy;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        self_type operator%(const T& val) noexcept {
            self_type copy = *this;
            copy %= val;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T>
        reference broadcast_op(const T& other, auto&& func) noexcept {
            if constexpr (std::is_same_v<shape_type, typename T::shape_type>) {
                // same type: add directly
                std::ranges::transform(data, other.data, data.begin(), [&func](auto& l, auto& r){ return func(l, r); });
            } else if constexpr (N == T::N) {
                // same size in dim: try to broadcast
                std::ranges::transform(data, other.data, data.begin(), [&func](auto& l, auto& r){ return func(l, r); });
            } else if constexpr (T::N == 1) {
                // other size == 1: try to broadcast
                std::ranges::for_each(data, [&other, &func](auto& elem){ func(elem, other[0]); });
            } else {
                // recursion
                std::ranges::for_each(data, [&other, &func](auto& elem){ func(elem, other); });
            }
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T>
        reference operator+=(const T& other) noexcept {
            return broadcast_op(other, [](auto&& l, auto&& r) {
                return l += r;
            });
        }

        template <typename T>
        requires is_ndarray_v<T>
        reference operator-=(const T& other) noexcept {
            return broadcast_op(other, [](auto&& l, auto&& r) {
                return l -= r;
            });
        }

        template <typename T>
        requires is_ndarray_v<T>
        reference operator*=(const T& other) noexcept {
            return broadcast_op(other, [](auto&& l, auto&& r) {
                return l *= r;
            });
        }

        template <typename T>
        requires is_ndarray_v<T>
        reference operator/=(const T& other) noexcept {
            return broadcast_op(other, [](auto&& l, auto&& r) {
                return l /= r;
            });
        }

        template <typename T>
        requires is_ndarray_v<T>
        reference operator%=(const T& other) noexcept {
            return broadcast_op(other, [](auto&& l, auto&& r) {
                return l %= r;
            });
        }

        template <typename T>
        requires is_ndarray_v<T> 
        auto operator+(const T& other) noexcept {
            static_assert(T::size() <= size());
            self_type copy = *this;
            copy += other;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> 
        auto operator-(const T& other) noexcept {
            static_assert(T::size() <= size());
            self_type copy = *this;
            copy -= other;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> 
        auto operator*(const T& other) noexcept {
            static_assert(T::size() <= size());
            self_type copy = *this;
            copy *= other;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> 
        auto operator/(const T& other) noexcept {
            static_assert(T::size() <= size());
            self_type copy = *this;
            copy /= other;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> 
        auto operator%(const T& other) noexcept {
            static_assert(T::size() <= size());
            self_type copy = *this;
            copy %= other;
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

        data_type& at(int idx) noexcept {
            if (std::abs(idx) >= N) {
                throw std::out_of_range("index out of range!");
            }
            return data[(idx + N) % N];
        }

        const data_type& at(int idx) const noexcept {
            if (std::abs(idx) >= N) {
                throw std::out_of_range("index out of range!");
            }
            return data[(idx + N) % N];
        }

        data_type& operator[](int idx) noexcept {
            return data[(idx + N) % N];
        }

        const data_type& operator[](int idx) const noexcept {
            return data[(idx + N) % N];
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

    template <typename T>
    requires is_ndarray_v<T>
    auto exp(const T& arr) noexcept {
        auto copy = arr;
        copy.exp();
        return copy;
    }

    template <typename T>
    requires is_ndarray_v<T>
    auto log(const T& arr) noexcept {
        auto copy = arr;
        copy.log();
        return copy;
    }

    auto sum(const auto& arr) noexcept {
        return arr.sum();
    }

    auto mean(const auto& arr) noexcept {
        return arr.mean();
    }

    template <std::size_t M, class dtype = double>
    auto eye() noexcept {
        ndarray<shape<M, M>, dtype> res;
        for (std::size_t i = 0; i < M; i++) {
            res[i][i] = static_cast<dtype>(1);
        }
        return res;
    }

    template <auto start, auto stop, std::size_t num>
    auto linspace() noexcept {
        ndarray<shape<num>, std::decay_t<std::common_type_t<decltype(start), decltype(stop)>>> res;
        for (std::size_t i = 0; i != num; i++) {
            res[i] = start + (stop - start) / num * i;
        }
        return res;
    }

    template <auto start, auto stop, auto step>
    auto arange() noexcept {
        constexpr std::size_t M = (stop - start + step - 1) / step;
        ndarray<shape<M>, std::decay_t<std::common_type_t<decltype(start), decltype(stop)>>> res;
        for (auto i = 0; start + step * i < stop; i++) {
            res[i] = start + i * step;
        }
        return res;
    }

    template <class Shape>
    requires is_shape_v<Shape>
    auto zeros() noexcept {
        ndarray<Shape> res;
        return res;
    }

    template <class Shape>
    requires is_shape_v<Shape>
    auto ones() noexcept {
        ndarray<Shape> res;
        res.fill(static_cast<ndarray<Shape>::dtype>(1));
        return res;
    }

    template <typename T>
    concept is_matrix = is_ndarray_v<T> && T::shape_type::dims == 2;

    template <typename Ndarray>
    requires is_matrix<std::decay_t<Ndarray>>
    auto transpose(Ndarray&& a) {
        static constexpr std::size_t N = std::decay_t<Ndarray>::N;
        static constexpr std::size_t M = std::decay_t<Ndarray>::data_type::N;
    
        ndarray<shape<M, N>> res;
        for (std::size_t i = 0; i < M; i++) {
            for (std::size_t j = 0; j < N; j++) {
                res[i][j] = a[j][i];
            }
        }
        return res;
    }

    template <typename Ndarray1, typename Ndarray2>
    requires is_matrix<std::decay_t<Ndarray1>> && is_matrix<std::decay_t<Ndarray2>>
    auto dot(Ndarray1&& a, Ndarray2&& b) {
        // (N x P) mul (P x M) -> (M x N)
        static constexpr std::size_t N = std::decay_t<Ndarray1>::N;
        static constexpr std::size_t P = std::decay_t<Ndarray2>::N;
        static constexpr std::size_t M = std::decay_t<Ndarray2>::data_type::N;
        ndarray<shape<N, M>> res;
        for (std::size_t i = 0; i < N; i++) {
            for (std::size_t j = 0; j < M; j++) {
                for (std::size_t k = 0; k < P; k++) {
                    res[i][j] = a[i][k] * b[k][j];
                }
            }
        }
        return res;
    }
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
