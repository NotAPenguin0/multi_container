#ifndef TYPE_LIST_HPP_
#define TYPE_LIST_HPP_

#include <tuple>
#include <cstddef>

template<typename... Types>
struct TypeList
{
	using TupleT = std::tuple<Types...>;

	template<typename U>
	using push_back_t = TypeList<Types..., U>;

	template<typename U>
	using push_front_t = TypeList<U, Types...>;

	template<template<typename...> typename FuncT>
	using apply_t = FuncT<Types...>;

	template<template<typename> typename FuncT>
	using map_t = TypeList<FuncT<Types>...>;

	template<std::size_t I>
	using get = std::tuple_element_t<I, TupleT>;

	constexpr std::size_t size() const
	{
		return std::tuple_size<TupleT>::value;
	}

};

#endif
