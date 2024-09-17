#include <format>
#include <iostream>
#include <tuple>
#include <sstream>
#include <utility>

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
