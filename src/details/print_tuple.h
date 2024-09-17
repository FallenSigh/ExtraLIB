#include <format>
#include <iostream>
#include <tuple>
#include <sstream>

template <typename Tuple, std::size_t N>
struct TupleHelper {
    static void print(std::stringstream& ss, const Tuple& t) noexcept {
        TupleHelper<Tuple, N - 1>::print(ss, t);
        if constexpr (N > 1) {
            ss << ", ";
        }
        ss << std::get<N - 1>(t);
    }
};

template <typename Tuple>
struct TupleHelper<Tuple, 0> {
    static void print(std::stringstream&, const Tuple&) noexcept {
    }
};

template <typename ...Types>
std::ostream& operator<<(std::ostream& os, const std::tuple<Types...>& t) noexcept {
    std::stringstream ss;
    ss << "(";
    if constexpr (sizeof...(Types) > 0) {
        TupleHelper<std::tuple<Types...>, sizeof...(Types)>::print(ss, t);
    }
    ss << ")";
    os << ss.str();
    return os;
}

template <typename ...Types>
struct std::formatter<std::tuple<Types...>> : std::formatter<std::string> {
    auto format(const std::tuple<Types...>& tuple, auto& ctx) const {
        std::stringstream ss;
        ss << "(";
        if constexpr (sizeof...(Types) > 0) {
            TupleHelper<std::tuple<Types...>, sizeof...(Types)>::print(ss, tuple);
        }
        ss << ")";
        return std::format_to(ctx.out(), "{}", ss.str());
    }
};
