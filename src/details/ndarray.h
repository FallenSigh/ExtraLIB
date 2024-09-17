#pragma once
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <sstream>
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
        inline static constexpr std::size_t first = N;
        inline static constexpr std::tuple<decltype(shapes)...> second = {shapes...};
        inline static constexpr std::tuple<std::size_t, decltype(shapes)...> value = {N, shapes...};
        using next_shape = shape<shapes...>;
        inline static constexpr std::size_t block_size = details::mul(shapes...);
        inline static constexpr std::size_t size = N * block_size;
    };

    template <std::size_t N>
    struct shape<N> {
        inline static constexpr std::size_t first = N;
        using second = void;
        inline static constexpr std::tuple<std::size_t> value = {N};
        using next_shape = void;
        inline static constexpr std::size_t block_size = 1;
        inline static constexpr std::size_t size = N * block_size;
    };

    template <typename T>
    struct is_shape : std::false_type {};

    template <std::size_t N, std::size_t ...shapes>
    struct is_shape<shape<N, shapes...>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_shape_v = is_shape<T>::value;

    template <typename Shape, class Backend = double, class Allocator = std::allocator<Backend>>
    struct ndarray;

    template <typename T>
    struct is_ndarray : std::false_type {};

    template <typename Shape, class Backend>
    struct is_ndarray<ndarray<Shape, Backend>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_ndarray_v = is_ndarray<T>::value;
}

namespace exlib {
    template <typename Shape, class Backend, class Allocator>
    requires std::is_void_v<typename Shape::second> 
    struct ndarray<Shape, Backend, Allocator> {
        static_assert(is_shape_v<Shape>);

        inline static constexpr auto shape = Shape::value;
        inline static constexpr std::size_t block_size = Shape::block_size;
        inline static constexpr std::size_t N = Shape::first;
        
        using shape_type = Shape;
        using backend_type = Backend;
        using one_dim = std::true_type;
        using data_type = backend_type;
        using array_type = std::conditional_t<std::is_void_v<Allocator>, details::static_array<data_type, N>, details::dynamic_array<data_type, N, Allocator>>;
        
        using iterator = typename array_type::iterator;
        using const_iterator = typename array_type::const_iterator;
        using reverse_iterator = typename array_type::reverse_iterator;
        using const_reverse_iterator = typename array_type::const_reverse_iterator;
        
        using self_type = ndarray<Shape, Backend>;
        using reference = ndarray<Shape, Backend>&;
        using const_reference = const ndarray<Shape, Backend>&;

        array_type data;

        ndarray() noexcept {
            data.fill(static_cast<Backend>(0));
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

        template <std::ranges::input_range Range>
        reference operator=(Range in) noexcept {
            return this->assign(in);
        }

        template <std::ranges::input_range Range>
        reference assign(Range in) noexcept {
            data.fill(static_cast<Backend>(0));
            for (std::size_t i = 0; i < in.size() && i < N; i++) {
                data[i] = in[i];
            }
            return *this;
        }

        void _flatten(std::vector<backend_type>& v) const noexcept {
            for (std::size_t i = 0; i < N; i++) {
                v.push_back(data[i]);
            }
        }

        ndarray<exlib::shape<shape_type::size>> flatten() const noexcept {
            ndarray<exlib::shape<shape_type::size>> res;
            std::vector<backend_type> t;
            this->_flatten(t);
            res = t;
            return res;
        }

        template <typename OShape>
        requires is_shape_v<OShape>
        ndarray<OShape, Backend, Allocator> reshape() const noexcept {
            static_assert(N * block_size == OShape::size);
            ndarray<OShape, Backend, Allocator> res;
            std::vector<backend_type> t;
            this->_flatten(t);
            res = t;
            return res;
        };

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
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        reference operator+=(const T& other) noexcept {
            using type = std::common_type_t<backend_type, typename T::backend_type>;
            std::ranges::transform(data, other.data, data.begin(), std::plus<type>());
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        reference operator-=(const T& other) noexcept {
            using type = std::common_type_t<backend_type, typename T::backend_type>;
            std::ranges::transform(data, other.data, data.begin(), std::minus<type>());
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        reference operator*=(const T& other) noexcept {
            using type = std::common_type_t<backend_type, typename T::backend_type>;
            std::ranges::transform(data, other.data, data.begin(), std::multiplies<type>());
            return *this;
        }

        template <typename T>
        requires is_ndarray_v<T> && std::is_same_v<shape_type, typename T::shape_type>
        reference operator/=(const T& other) noexcept {
            using type = std::common_type_t<backend_type, typename T::backend_type>;
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

