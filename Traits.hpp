#ifndef TRAITS_HPP_
#define TRAITS_HPP_

#include <type_traits>
#include <tuple>

template<typename T, typename Tuple>
struct TupleHasType;

template<typename T>
struct TupleHasType<T, std::tuple<>> : public std::false_type {};

template<typename T, typename U, typename... Ts>
struct TupleHasType<T, std::tuple<U, Ts...>> : public TupleHasType<T, std::tuple<Ts...>> {};

template<typename T, typename... Ts>
struct TupleHasType<T, std::tuple<T, Ts...>> : public std::true_type {};

template<typename T, typename Tuple>
using TupleHasTypeT = typename TupleHasType<T, Tuple>::type;

template<typename T, typename Tuple>
static constexpr bool TupleHasTypeV = TupleHasTypeT<T, Tuple>::value;

#endif
