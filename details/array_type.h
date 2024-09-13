#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>
namespace exlib {
    namespace details {
        template<typename T, std::size_t N>
        struct static_array : public std::array<T, N>{
            using base_class_type = std::array<T, N>;

            static_array() noexcept {
                this->fill(static_cast<T>(0u));
            }

            void reset(T value = 0) noexcept {
                this->fill(value);
            }
        };

        template<typename T, std::size_t N, class Allocator> 
        struct dynamic_array : public std::vector<T, Allocator>{
            using base_class_type = std::vector<T, Allocator>;

            dynamic_array() noexcept 
            : base_class_type(N, static_cast<T>(0u))  {}
        
            void reset(T value = 0) noexcept {
                std::fill(this->begin(), this->end(), static_cast<T>(value));
            }
        };  
    }
}