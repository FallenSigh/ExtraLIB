#include <format>
#include <iostream>
#include <tuple>
#include <sstream>
#include <utility>
#include <vector>

template<typename Tuple, std::size_t... I>
void print_tuple(std::stringstream& ss, const Tuple& t, std::index_sequence<I...>) {
    ss << "(";
    bool first = true; 
    ((ss << (first ? (first = false, "") : ", ") << std::get<I>(t)), ...);
    ss << ")";
}

template <typename ...Types>
std::ostream& operator<<(std::ostream& os, const std::tuple<Types...>& t) noexcept {
    std::stringstream ss;
    print_tuple(ss, t, std::make_index_sequence<sizeof...(Types)>());
    os << ss.str();
    return os;
}

template <typename ...Types>
struct std::formatter<std::tuple<Types...>> : std::formatter<std::string> {
    auto format(const std::tuple<Types...>& t, auto& ctx) const {
        std::stringstream ss;
        print_tuple(ss, t, std::make_index_sequence<sizeof...(Types)>());
        return std::format_to(ctx.out(), "{}", ss.str());
    }
};

template <typename T>
struct std::formatter<std::vector<T>> : std::formatter<T> {
    auto format(const auto& vec, auto& ctx) const {
        std::stringstream ss;
        ss << "[";
        for (std::size_t i = 0; i < vec.size() - 1; i++) {
            ss << vec[i] << ", ";
        }
        ss << vec[vec.size() - 1];
        ss << "]";
        return std::format_to(ctx.out(), "{}", ss.str());
    }
};