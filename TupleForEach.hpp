#ifndef MVG_TUPLE_FOR_EACH_HPP_
#define MVG_TUPLE_FOR_EACH_HPP_

#include <tuple>
#include "TypeList.hpp"

namespace TupleForEach
{
	namespace detail
	{
		//mod parameter represents const and ref modifiers
#define GEN_FOREACH(mod) template<std::size_t I, typename F, typename...TupleTs, typename... Args> \
	static constexpr \
		std::enable_if_t<I == sizeof...(TupleTs), void> \
		doForEach( \
			[[maybe_unused]] std::tuple<TupleTs...> mod t, \
			[[maybe_unused]] F&& f, \
			[[maybe_unused]] Args&&... args) \
	{\
\
	}\
	template<std::size_t I, typename F, typename... TupleTs, typename... Args> \
	static constexpr \
		std::enable_if_t < I < sizeof...(TupleTs), void> \
		doForEach(std::tuple<TupleTs...> mod t, F&& f, Args&&... args) \
	{ \
		constexpr std::size_t size = std::tuple_size<std::tuple<TupleTs...>>::value; \
\
		f(std::get<I>(t), args...); \
		if constexpr (I < size) \
			doForEach<I + 1, F, TupleTs..., Args...>(t, std::forward<F>(f), std::forward<Args>(args)...); \
	}

		GEN_FOREACH(&)
		GEN_FOREACH(const&)

	}

#define GEN_FOREACH_PUBL(mod) \
	template<typename F, typename... TupleTs, typename... Args > \
	static constexpr void foreach(std::tuple<TupleTs...> mod t, F&& f, Args&&... args) \
	{ \
		detail::doForEach<0, F, TupleTs..., Args...>(t, std::forward<F>(f), std::forward<Args>(args)...); \
	} \

	GEN_FOREACH_PUBL(&)
	GEN_FOREACH_PUBL(const&)


	namespace detail
	{

#define GEN_PAR_FOREACH_IMPL(moda, modb) \
	template<std::size_t I, typename F, typename TList, typename UList, typename... Args> \
	static constexpr \
		std::enable_if_t< I == std::tuple_size_v<typename TList::TupleT>, void> \
		doParallelForEach( \
			[[maybe_unused]] typename TList::TupleT moda a, \
			[[maybe_unused]] typename UList::TupleT modb b, \
			[[maybe_unused]] F&& f, \
			[[maybe_unused]] Args&&... args) \
	{ \
\
	} \
\
	template<std::size_t I, typename F, typename TList, typename UList, typename... Args> \
	static constexpr \
		std::enable_if_t < I < std::tuple_size_v<typename TList::TupleT>, void > \
		doParallelForEach(typename TList::TupleT moda a, typename UList::TupleT modb b, F&& f, Args&&... args) \
	{ \
		constexpr std::size_t size = std::tuple_size<typename TList::TupleT>::value; \
			\
		f(std::get<I>(a), std::get<I>(b), args...); \
		if constexpr (I < size) \
			doParallelForEach< I + 1, F, TList, UList, Args... > (a, b, std::forward<F>(f), std::forward<Args>(args)...); \
	}

	GEN_PAR_FOREACH_IMPL(&, &)
	GEN_PAR_FOREACH_IMPL(const&, &)
	GEN_PAR_FOREACH_IMPL(&, const&)
	GEN_PAR_FOREACH_IMPL(const&, const&)

	} //namespace detail

#define GEN_PAR_FOREACH(moda, modb) \
	template<typename F, typename... TupleTs, typename... TupleUs, typename... Args> \
	static constexpr void parallel_foreach(std::tuple<TupleTs...> moda a, std::tuple<TupleUs...> modb b, F&& f, Args&&... args) \
	{ \
		detail::doParallelForEach<0, F, TypeList<TupleTs...>, TypeList<TupleUs...>, Args...>(a, b, std::forward<F>(f), std::forward<Args>(args)...); \
	} 

	GEN_PAR_FOREACH(&, &)
	GEN_PAR_FOREACH(&, const&)
	GEN_PAR_FOREACH(const&, &)
	GEN_PAR_FOREACH(const&, const&)

}

#undef GEN_FOREACH
#undef GEN_FOREACH_PUBL
#undef GEN_PAR_FOREACH_IMPL
#undef GEN_PAR_FOREACH

#endif
