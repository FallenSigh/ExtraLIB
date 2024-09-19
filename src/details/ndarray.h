#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <sstream>
#include <tuple>
#include <type_traits>

#include "array_type.h"
#include "../log.h"

namespace exlib {
    namespace details {
        template <typename ...Args>
        consteval std::size_t mul(Args... args) {
            return (args * ...);
        }
    }

    template <std::size_t N, std::size_t ...shapes>
    struct shape {
        using type = shape<N, shapes...>;
        inline static constexpr std::size_t first = N;
        inline static constexpr std::tuple<decltype(shapes)...> second = {shapes...};
        inline static constexpr std::tuple<std::size_t, decltype(shapes)...> value = {N, shapes...};
        using next_shape_type = shape<shapes...>;
        inline static constexpr std::size_t dims = 1 + next_shape_type::dims;
        inline static constexpr std::size_t block_size = details::mul(shapes...);
        inline static constexpr std::size_t size = N * block_size;
    };

    template <std::size_t N>
    struct shape<N> {
        using type = shape<N>;
        inline static constexpr std::size_t first = N;
        using second = void;
        inline static constexpr std::tuple<std::size_t> value = {N};
        using next_shape_type = void;
        inline static constexpr std::size_t dims = 1;
        inline static constexpr std::size_t block_size = 1;
        inline static constexpr std::size_t size = N * block_size;
    };

    template <std::size_t N, typename Shape>
    struct shape_helper {
        using type = shape_helper<N, Shape>;
        inline static constexpr auto value = std::tuple_cat(std::make_tuple(N), Shape::value);
        inline static constexpr std::size_t first = N;
        inline static constexpr auto second = Shape::value;
        using next_shape_type = Shape;
        inline static constexpr std::size_t block_size = Shape::size;
        inline static constexpr std::size_t size = N * block_size;
    };

    template <typename T>
    struct is_shape : std::false_type {};

    template <std::size_t N, std::size_t ...shapes>
    struct is_shape<shape<N, shapes...>> : std::true_type {};

    template <std::size_t N, typename Shape>
    struct is_shape<shape_helper<N, Shape>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_shape_v = is_shape<T>::value;

    template <typename Shape, class DType = double, class Allocator = std::allocator<DType>>
    struct ndarray;

    template <typename T>
    struct is_ndarray : std::false_type {};

    template <typename Shape, class DType>
    struct is_ndarray<ndarray<Shape, DType>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_ndarray_v = is_ndarray<T>::value;
}

namespace exlib {
    template <typename Shape, class DType, class Allocator>
    requires std::is_void_v<typename Shape::second> 
    struct ndarray<Shape, DType, Allocator> {
        static_assert(is_shape_v<Shape>);

        inline static constexpr auto shape = Shape::value;
        inline static constexpr std::size_t block_size = Shape::block_size;
        inline static constexpr std::size_t N = Shape::first;

        using shape_type = Shape;
        using dtype = DType;
        using one_dim = std::true_type;
        using data_type = dtype;
        using array_type = std::conditional_t<std::is_void_v<Allocator>, details::static_array<data_type, N>, details::dynamic_array<data_type, N, Allocator>>;
        
        using iterator = typename array_type::iterator;
        using const_iterator = typename array_type::const_iterator;
        using reverse_iterator = typename array_type::reverse_iterator;
        using const_reverse_iterator = typename array_type::const_reverse_iterator;
        
        using self_type = ndarray<Shape, DType>;
        using reference = ndarray<Shape, DType>&;
        using const_reference = const ndarray<Shape, DType>&;

        array_type data;

        ndarray() noexcept {
            data.fill(static_cast<DType>(0));
        }

        ndarray(std::initializer_list<data_type> s) noexcept {
            for (std::size_t i = 0; i < s.size() && i < N; i++) {
                data[i] = *(s.begin() + i);
            }
        }

        template <std::ranges::input_range Range>
        requires (!is_ndarray_v<Range>)
        ndarray(Range in) noexcept {
            this->assign(in);
        }

        ndarray(self_type&& other) noexcept {
            if (&other != this) {
                std::move(other.data.begin(), other.data.end(), data.begin());
            }
        }

        ndarray(const_reference other) noexcept {
            std::copy(other.data.begin(), other.data.end(), data.begin());
        }

        ndarray(const data_type& other) noexcept {
            data.fill(other);
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
        reference operator=(Range in) noexcept {
            return this->assign(in);
        }

        template <std::ranges::input_range Range>
        requires (!is_ndarray_v<Range>)
        reference assign(Range in) noexcept {
            data.fill(static_cast<DType>(0));
            for (std::size_t i = 0; i < in.size() && i < N; i++) {
                data[i] = in[i];
            }
            return *this;
        }

        void _flatten(std::vector<dtype>& v) const noexcept {
            for (std::size_t i = 0; i < N; i++) {
                v.push_back(data[i]);
            }
        }

        template <typename T = dtype>
        T sum() const noexcept {
            T res = 0;
            for (std::size_t i = 0; i < N; i++) {
                res += data[i];
            }
            return res;
        }

        template <typename T = dtype>
        T mean() const noexcept {
            return sum<T>() / size;
        }

        void fill(const dtype& value) noexcept {
            data.fill(value);
        }

        reference pow(const dtype& value) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i] = std::pow(data[i], value);
            }
            return *this;
        }

        reference exp() noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i] = std::exp(data[i]);
            }
            return *this;
        }

        reference log() noexcept {
            for (std::size_t i = 0; i < N; i++) {
                data[i] = std::log(data[i]);
            }
            return *this;
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
                if (cond.data[i]) {
                    res.push_back(data[i]);
                }
            }
            return res;
        }

        template <typename OShape>
        requires is_shape_v<OShape>
        ndarray<OShape, DType, Allocator> reshape() const noexcept {
            static_assert(N * block_size == OShape::size);
            ndarray<OShape, DType, Allocator> res;
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
                res.data[i] = (data[i] > val);
            }
            return res;
        }

        ndarray<shape_type, bool> operator<(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] < val);
            }
            return res;
        }

        ndarray<shape_type, bool> operator==(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] == val);
            }
            return res;
        }

        ndarray<shape_type, bool> operator!=(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] != val);
            }
            return res;
        }

        ndarray<shape_type, bool> operator>=(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] >= val);
            }
            return res;
        }

        ndarray<shape_type, bool> operator<=(const dtype& val) noexcept {
            ndarray<shape_type, bool> res;
            for (std::size_t i = 0; i < N; i++) {
                res.data[i] = (data[i] <= val);
            }
            return res;
        }

        self_type put_mask(const ndarray<shape_type, bool>& cond, const dtype& value) {
            for (std::size_t i = 0; i < N; i++) {
                if (cond.data[i] == true) {
                    this->data[i] = value;
                }
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
        self_type operator+(const T& val) noexcept {
            self_type copy = *this;
            copy += val;
            return copy;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        self_type operator-(const T& val) noexcept {
            self_type copy = *this;
            copy -= val;
            return copy;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        self_type operator*(const T& val) noexcept {
            self_type copy = *this;
            copy *= val;
            return copy;
        }

        template <typename T>
        requires (!is_ndarray_v<T>)
        self_type operator/(const T& val) noexcept {
            self_type copy = *this;
            copy /= val;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> && (std::is_same_v<shape_type, typename T::shape_type> || T::N == 1)
        reference operator+=(const T& other) noexcept {
            using type = std::common_type_t<dtype, typename T::dtype>;
            if constexpr (T::N == 1) {
                // try to broadcast
                std::ranges::for_each(data, [&other](auto& elem){ elem += other.data[0]; });
            } else {
                // add directly
                std::ranges::transform(data, other.data, data.begin(), std::plus<type>());
            }
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        reference operator-=(const T& other) noexcept {
            using type = std::common_type_t<dtype, typename T::dtype>;
            std::ranges::transform(data, other.data, data.begin(), std::minus<type>());
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        reference operator*=(const T& other) noexcept {
            using type = std::common_type_t<dtype, typename T::dtype>;
            std::ranges::transform(data, other.data, data.begin(), std::multiplies<type>());
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        reference operator/=(const T& other) noexcept {
            using type = std::common_type_t<dtype, typename T::dtype>;
            std::ranges::transform(data, other.data, data.begin(), std::divides<type>());
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        self_type operator+(const T& other) noexcept {
            self_type copy = *this;
            copy += other;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        self_type operator-(const T& other) noexcept {
            self_type copy = *this;
            copy -= other;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        self_type operator*(const T& other) noexcept {
            self_type copy = *this;
            copy *= other;
            return copy;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        self_type operator/(const T& other) noexcept {
            self_type copy = *this;
            copy /= other;
            return copy;
        }

        void left_ops(auto& lhs, auto&& op) {
            for (std::size_t i = 0; i < N; i++) {
                data[i] = op(lhs, data[i]);
            }
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
                ss << data[i] << ", ";
            }
            if (N > 0) ss << data[N - 1];
            ss << "]";
        }
    };
}

