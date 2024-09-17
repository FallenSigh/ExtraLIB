#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>
namespace exlib {
    namespace details {
        template <typename T, class U>
        struct rebind {
            using type = typename std::allocator_traits<U>::template rebind_alloc<T>;
        };

        template <class U>
        struct rebind<void, U> {
            using type = std::allocator<void>;
        };

        template<typename T, std::size_t N>
        struct static_array : public std::array<T, N>{
            using base_class_type = std::array<T, N>;

            static_array() noexcept {

            }
        };

        template<typename T, std::size_t N, class Allocator> 
        struct dynamic_array : public std::vector<T, Allocator>{
            using base_class_type = std::vector<T, Allocator>;

            dynamic_array() noexcept 
            : base_class_type(N)  {}
        
            void fill(T value = 0) noexcept {
                std::fill(this->begin(), this->end(), static_cast<T>(value));
            }
        };  
    }
}