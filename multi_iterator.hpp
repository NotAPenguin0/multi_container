#ifndef MVG_MULTI_ITERATOR_HPP_
#define MVG_MULTI_ITERATOR_HPP_

#include <tuple>
#include <algorithm>
#include <iterator> //To specialize std::iterator_traits<T>
#include <cstddef> //For std::size_t
#include <type_traits>
#include <cassert>
#include <utility>
#include <exception>


#include "TypeList.hpp"
#include "Traits.hpp"
#include "TupleForEach.hpp"
#include "Unused.hpp"

#ifdef _DEBUG
#define dbg_assert(cond, msg) if(!!cond) {} else {throw std::runtime_error(msg);}
#else
#define dbg_assert(cond, msg)
#endif

namespace mvg
{

template<typename...Ts>
class multi_container;



namespace detail
{
class multi_insert;
class multi_erase;
/*
template <class... Args, class = std::enable_if_t<(std::is_reference_v<Args> && ...)>>
void do_swap(std::tuple<Args...> &lhs, std::tuple<Args...> &rhs)
{
	std::swap(lhs, rhs);
}
}*/

#define MAKE_REF_TUPLE_OP(op) \
template<typename... Ts, typename... Us> \
bool operator op(tuple_wrapper<Ts...> const& lhs, tuple_wrapper<Us...> const& rhs) {return lhs.m_tuple op rhs.m_tuple;}

#define MAKE_REF_TUPLE_OP_WITH_TUPLE(op) \
template<typename... Ts, typename... Us> \
bool operator op(tuple_wrapper<Ts...> const& lhs, std::tuple<Us...> const& rhs) {return lhs.m_tuple op rhs;} \
template<typename... Ts, typename... Us> \
bool operator op(std::tuple<Ts...>, tuple_wrapper<Us...> const& rhs) {return lhs op rhs.m_tuple;}

//Wrapper class around std::tuple for reference types. Used by mvg::multi_iterator to dereference
template <typename... Ts>
struct tuple_wrapper 
{
	std::tuple<Ts...> m_tuple;

	using val_tuple_wrapper_t = tuple_wrapper<std::remove_reference_t<Ts>...>;
	using tuple_wrapper_wrapper_t = tuple_wrapper<std::add_lvalue_reference_t<Ts>...>;
	using val_tuple_t = std::tuple<std::remove_reference_t<Ts>...>;
	using tuple_wrapper_t = std::tuple<std::add_lvalue_reference_t<Ts>...>;

	tuple_wrapper(val_tuple_t &&t)
		: m_tuple { std::move(t) } {}
	tuple_wrapper(tuple_wrapper_t &&t)
		: m_tuple { std::move(t) } {}
	tuple_wrapper(val_tuple_wrapper_t &&rhs) noexcept
		: m_tuple { std::move(rhs.m_tuple) } {}
	tuple_wrapper(const val_tuple_wrapper_t &rhs)
		: m_tuple { rhs.m_tuple } {}
	tuple_wrapper(const tuple_wrapper_wrapper_t &rhs)
		: m_tuple { rhs.m_tuple } {}

	auto &operator=(val_tuple_wrapper_t const &rhs) 
	{
		m_tuple = rhs.m_tuple;
		return *this;
	}
	auto &operator=(tuple_wrapper_wrapper_t const &rhs) 
	{
		m_tuple = rhs.m_tuple;
		return *this;
	}

	friend void swap(tuple_wrapper &&lhs, tuple_wrapper &&rhs) 
	{
		std::swap(lhs.m_tuple, rhs.m_tuple);
	}

	template<typename T>
	T& get_elem()
	{
		return std::get<T&>(m_tuple);
	}

	template<std::size_t I>
	std::tuple_element_t<I, std::tuple<Ts...>>& get_elem()
	{
		return std::get<I>(m_tuple);
	}

};

template <class... Ts>
tuple_wrapper(std::tuple<Ts...>)->tuple_wrapper<Ts...>;

template <>
struct tuple_wrapper<> {};

/*
 *std::tuple has these operators defined, so logically this tuple_wrapper needs them too.
 *They're simply implemented by doing
 *
 *return lhs.m_tuple op rhs.m_tuple;
 */

MAKE_REF_TUPLE_OP(<)
MAKE_REF_TUPLE_OP(>)
MAKE_REF_TUPLE_OP(<=)
MAKE_REF_TUPLE_OP(>=)
MAKE_REF_TUPLE_OP(==)
MAKE_REF_TUPLE_OP(!=)

MAKE_REF_TUPLE_OP_WITH_TUPLE(<)
MAKE_REF_TUPLE_OP_WITH_TUPLE(>)
MAKE_REF_TUPLE_OP_WITH_TUPLE(<=)
MAKE_REF_TUPLE_OP_WITH_TUPLE(>=)
MAKE_REF_TUPLE_OP_WITH_TUPLE(==)
MAKE_REF_TUPLE_OP_WITH_TUPLE(!=)

#undef MAKE_REF_TUPLE_OP
#undef MAKE_REF_TUPLE_OP_WITH_TUPLE

} //namespace detail

} //namespace mvg

/*Specializations for mvg::detail::tuple_wrapper for standard library 
 *functions and traits that are frequently used on std::tuple's
*/
namespace std
{

template<std::size_t I, typename... Ts>
auto get(mvg::detail::tuple_wrapper<Ts...> tpl) -> decltype(std::get<I>(tpl.m_tuple))
{
	return std::get<I>(tpl.m_tuple);
}

template<typename T, typename... Ts>
auto get(mvg::detail::tuple_wrapper<Ts...> tpl) -> decltype(std::get<T>(tpl.m_tuple))
{
	return std::get<T>(tpl.m_tuple);
}

template <typename... Ts>
struct remove_reference<mvg::detail::tuple_wrapper<Ts...>> 
{
	using type = mvg::detail::tuple_wrapper<std::remove_reference_t<Ts>...>;
};

template<typename... Ts>
struct tuple_size<mvg::detail::tuple_wrapper<Ts...>>
{
	static constexpr std::size_t value = std::tuple_size_v<std::tuple<Ts...>>;
};

template<std::size_t I, typename... Ts>
struct tuple_element<I, mvg::detail::tuple_wrapper<Ts...>>
{
	using type = std::tuple_element_t<I, std::tuple<Ts...>>;
};

} //namespace std


namespace mvg
{

namespace detail
{

/*Helper struct for TupleForEach::foreach(), to increment all iterators stored in the std::tuple of iterators
 *held my mvg::multi_iterator
 **/
struct do_increment
{
	template<typename It>
	void operator()(It& it)
	{
		++it;
	}
};

/*Helper struct for TUpleForEach::foreach()*/
struct do_decrement
{
	template<typename It>
	void operator()(It& it)
	{
		--it;
	}
};

/*Type trait for determining the lowest common iterator tag
 *If there is no common type (eg std::output_iterator_tag and std::input_iterator_tag), std::input_iterator_tag is chosen
 */

/*SFINAE fallback. Uses default tag (std::input_iterator_tag)*/
template<typename = std::void_t<>, typename... Its>
struct common_iterator_tag
{
	using type = std::input_iterator_tag; //default tag, lowest possible
};

/*If the SFINAE inside the std::void_t<> fails, the specialization above is chosen, and type is
 *defined to st:d:input_iterator_tag*/
template<typename... Its>
struct common_iterator_tag<
	std::void_t<typename std::common_type<typename Its::iterator_category ...>::type>,
	Its...>
{
	using type = std::common_type_t<typename std::iterator_traits<Its>::iterator_category ...>;
};

//Tag order:
//InputIterator == OutputIterator < ForwardIterator < BidirectionalIterator < RandomAccessIterator
//multi_iterator has a static_assert making sure the iterators are never a mix of InputIterator and OutputIterator

#define EXCLUDE(T, FROM) template<> \
struct is_at_least_tag<T, FROM> : public std::false_type {};

/*Trait to determine if an iterator tag is the tag specified or higher
 *This is implemented using many specializations of the trait for every combination
 *Without the macro from above this would get very lengthy, that's why it's there*/

template<typename T, typename Cat>
struct is_at_least_tag : public std::false_type {};

/*Every category is at least std::input_iterator_tag or std::output_iterator_tag*/

template<typename T>
struct is_at_least_tag<T, std::input_iterator_tag> : public std::true_type {};

template<typename T>
struct is_at_least_tag<T, std::output_iterator_tag> : public std::true_type {};

/*Default for std::forward_iterator_tag to true, then exclude std::input_iterator_tag and std::output_iterator_tag*/

template<typename T>
struct is_at_least_tag<T, std::forward_iterator_tag> : public std::true_type {};

EXCLUDE(std::input_iterator_tag, std::forward_iterator_tag)
EXCLUDE(std::output_iterator_tag, std::forward_iterator_tag)

template<typename T>
struct is_at_least_tag<T, std::bidirectional_iterator_tag> : public std::true_type {};

EXCLUDE(std::input_iterator_tag, std::bidirectional_iterator_tag)
EXCLUDE(std::output_iterator_tag, std::bidirectional_iterator_tag)
EXCLUDE(std::forward_iterator_tag, std::bidirectional_iterator_tag)

template<typename T>
struct is_at_least_tag<T, std::random_access_iterator_tag> : public std::true_type {};

EXCLUDE(std::input_iterator_tag, std::random_access_iterator_tag)
EXCLUDE(std::output_iterator_tag, std::random_access_iterator_tag)
EXCLUDE(std::forward_iterator_tag, std::random_access_iterator_tag)
EXCLUDE(std::bidirectional_iterator_tag, std::random_access_iterator_tag)

template<typename T, typename Cat>
static constexpr bool is_at_least_tag_v = is_at_least_tag<T, Cat>::value;

#undef EXCLUDE


/*helper struct to compare all iterators in the multi_iterator
 *The comparison uses the result of the cmp() function for the first
 *pair of iterators, and then checks if all other iterators yield the same result*/
struct iterator_compare
{
	template<typename Tpl, typename Comp, std::size_t... Is>
	void operator()(Tpl const& a, Tpl const& b, Comp&& cmp, std::index_sequence<Is...>)
	{
		m_val = (cmp(std::get<0>(a), std::get<0>(b)));
		dbg_assert(((cmp(std::get<Is>(a), std::get<Is>(b))) && ...) == m_val, "Ordering isn't equal"); //make sure all comparisons are the same
	}

	bool value()
	{
		return m_val;
	}

private:
	bool m_val;
};

/*Helper to swap two iterators. This will be called inside a TupleForEach::parallel_foreach(), because swapping requires 
 *2 arguments*/
struct iterator_swap
{
	template<typename It>
	void operator()(It& a, It& b)
	{
		It temp = a;
		a = b;
		b = temp;
	}
};

/*struct to pass to iterator_compare. Operator() checks if a == b*/
struct equal
{
	template<typename T>
	bool operator()(T const& a, T const& b)
	{
		return a == b;
	}
};

/*Struct to pass to iterator_compare. Operator() checks if a < b*/
struct less
{
	template<typename T>
	bool operator()(T const& a, T const& b)
	{
		return a < b;
	}
};

/*Struct to pass to iterator_compare. Operator() checks if a <= b*/
struct less_or_equal
{
	template<typename T>
	bool operator()(T const& a, T const& b)
	{
		return a <= b;
	}
};

/*Struct to pass to iterator_compare. Operator() checks if a > b*/
struct greater
{
	template<typename T>
	bool operator()(T const& a, T const& b)
	{
		return a > b;
	}
};


/*Struct to pass to iterator_compare. Operator() checks if a >= b*/
struct greater_or_equal
{
	template<typename T>
	bool operator()(T const& a, T const& b)
	{
		return a >= b;
	}
};


} //namespace detail

/*\class: multi_iterator
 *\usage: This iterator is intended to be used in combination with mvg::multi_container
 *		  But you can construct it directly from an argpack of iterators if you want
*/
template<typename... Its>
class multi_iterator
{
public:
	//Required typedefs for iterators

	using difference_type = std::ptrdiff_t;
	using value_type = detail::tuple_wrapper<typename std::iterator_traits<Its>::value_type...>;
	using pointer = std::add_pointer_t<value_type>;
	using reference = detail::tuple_wrapper<typename std::iterator_traits<Its>::reference...>;
	using iterator_category =
		std::common_type_t<typename std::iterator_traits<Its>::iterator_category...>; /*typename detail::common_iterator_tag<Its...>::type;*/

private:
	
	//Some typedefs to make the declaration a bit easier, and we also need TupleT again later
	using TList = TypeList<Its...>;
	using TupleT = typename TList::TupleT;

	TupleT m_iterators;

public:
	template<typename... Tys>
	friend class multi_container;
	friend class ::mvg::detail::multi_insert;
	friend class ::mvg::detail::multi_erase;

	//This static_assert checks if the iterators are not a mix of std::input_iterator_tag and std::output_iterator_tag
	static_assert(!(
		TupleHasTypeT<
			std::input_iterator_tag,
			typename TypeList<
				typename std::iterator_traits<Its>::iterator_category ...>
					::TupleT>
		::value
		&& TupleHasTypeT<
			std::output_iterator_tag,
			typename TypeList<
				typename std::iterator_traits<Its>::iterator_category ...>
					::TupleT>
		::value),
		"Iterators can't be both input and output iterators!");

	multi_iterator()
	{
	}

	multi_iterator(Its... its) : m_iterators(TupleT { std::forward<Its>(its)... })
	{
	}

	multi_iterator(multi_iterator const& other) : m_iterators(other.m_iterators)
	{
	}

	multi_iterator& operator=(multi_iterator const& other)
	{
		m_iterators = other.m_iterators;
		return *this;
	}

	virtual ~multi_iterator()
	{
	}

	friend void swap(multi_iterator& a, multi_iterator& b)
	{
		TupleForEach::parallel_foreach(a.m_iterators, b.m_iterators, detail::iterator_swap {});
	}

	//increment is allowed on all iterators

	multi_iterator& operator++() //pre increment
	{
		TupleForEach::foreach(m_iterators, detail::do_increment {});
		return *this;
	}

	multi_iterator& operator++(int) //post increment
	{
		multi_iterator copy = *this;
		++(*this);
		return copy;
	}

	reference operator*()
	{
		return detail::tuple_wrapper { std::tie(*std::get<Its>(m_iterators) ...) };
	}

	auto operator*() const
	{
		//Make sure we get a const version
		return detail::tuple_wrapper<
			std::add_const_t< //has to be const
				std::add_lvalue_reference_t< //add lvalue reference, because std::tie() creates a tuple of refs
					typename std::iterator_traits<Its>::value_type //add that stuff to the value_type of this iterator
				>
			>
			... //expand pack
		> 
		{ std::tie(*std::get<Its>(m_iterators) ...) };
	}

	//Conversion to a tuple of references to the iterators. Needed for structured bindings

	operator std::tuple<std::add_lvalue_reference_t<Its>...>()
	{
		return **this;
	}

	//Following operations are only defined if the iterator is at least an InputIterator:
	/*
	 * ==
	 * !=
	 */

	friend bool operator==(multi_iterator const& lhs, multi_iterator const& rhs)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::input_iterator_tag>::value,
			"iterator_category must be at least InputIterator to use operator==");

		detail::iterator_compare comp;
		comp(lhs.m_iterators, rhs.m_iterators, mvg::detail::equal {}, std::make_index_sequence<std::tuple_size<TupleT>::value>{});
		return comp.value();
	}

	friend bool operator!=(multi_iterator const& lhs, multi_iterator const& rhs)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::input_iterator_tag>::value,
			"iterator_category must be at least InputIterator to use operator!=");

		return !(lhs == rhs);
	}

	//Since we can't really implement this operator on multi_iterator, we throw a compiler error when
	//it's instantiated. The reason it is here is to make the multi_iterator conform to the standard, which says
	//operator-> should be present on all iterators
	pointer operator->()
	{
		static_assert(false, "operator-> is not supported on multi_iterator");
//		return **this;
	}

	multi_iterator& operator--() //pre decrement
	{
		static_assert(detail::is_at_least_tag<iterator_category, std::bidirectional_iterator_tag>::value,
			"iterator_category must be at least BidirectionalIterator to use operator--");

		TupleForEach::foreach(m_iterators, detail::do_decrement {});
		return *this;
	}

	multi_iterator& operator--(int) //post decrement
	{
		static_assert(detail::is_at_least_tag<iterator_category, std::bidirectional_iterator_tag>::value,
			"iterator_category must be at least BidirectionalIterator to use operator--");

		multi_iterator copy { *this };
		--(*this);
		return copy;
	}

	multi_iterator& operator+=(difference_type n)
	{
		static_assert(detail::is_at_least_tag<iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator+=");
		
		difference_type m = n;
		if (m >= 0) while (m--) ++(*this);
		else while (m++) --(*this);
		return *this;
	}

	friend multi_iterator operator+(multi_iterator const& it, difference_type n)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator+");
	
		multi_iterator temp = it;
		return temp += n;
	}

	friend multi_iterator operator+(difference_type n, multi_iterator const& it)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator+");
		
		multi_iterator temp = it;
		return temp += n;
	}

	multi_iterator& operator-=(difference_type n)
	{
		static_assert(detail::is_at_least_tag<iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator-=");

		return *this += -n;
	}

	friend multi_iterator operator-(multi_iterator const& it, difference_type n)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator-");
		
		multi_iterator temp = it;
		return temp -= n;
	}

	friend multi_iterator operator-(difference_type n, multi_iterator const& it)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator-");
		
		multi_iterator temp = it;
		return temp -= n;
	}

	//Warning, this might not work correctly when containers are of different sizes/iterators are at different positions
	friend difference_type operator-(multi_iterator const& a, multi_iterator const& b)
	{
		static_assert(detail::is_at_least_tag<iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator-");

		return std::get<0>(a.m_iterators) - std::get<0>(b.m_iterators);
	}

	auto operator[](difference_type n)
	{
		static_assert(detail::is_at_least_tag<iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator[]");

		*this += n;
		return **this;
	}

	auto operator[](difference_type n) const
	{
		static_assert(detail::is_at_least_tag<iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator[]");

		*this += n;
		return **this;
	}

	friend bool operator<(multi_iterator const& a, multi_iterator const& b)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator<");
		detail::iterator_compare comp;
		comp(a.m_iterators, b.m_iterators, detail::less {}, std::make_index_sequence<std::tuple_size<TupleT>::value> {});
		return comp.value();
	}

	friend bool operator>(multi_iterator const& a, multi_iterator const& b)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator>");
		detail::iterator_compare comp;
		comp(a.m_iterators, b.m_iterators, detail::greater {}, std::make_index_sequence<std::tuple_size<TupleT>::value> {});
		return comp.value();
	}

	friend bool operator<=(multi_iterator const& a, multi_iterator const& b)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator<=");
		detail::iterator_compare comp;
		comp(a.m_iterators, b.m_iterators, detail::less_or_equal {}, std::make_index_sequence<std::tuple_size<TupleT>::value> {});
		return comp.value();
	}

	friend bool operator>=(multi_iterator const& a, multi_iterator const& b)
	{
		static_assert(detail::is_at_least_tag<typename multi_iterator::iterator_category, std::random_access_iterator_tag>::value,
			"iterator_category must be at least RandomAccessIterator to use operator>=");
		detail::iterator_compare comp;
		comp(a.m_iterators, b.m_iterators, detail::greater_or_equal {}, std::make_index_sequence<std::tuple_size<TupleT>::value> {});
		return comp.value();
	}
}; //class multi_iterator

} //namespace mvg


namespace std
{
//specialization for iterator_traits
template<typename... Ts>
struct iterator_traits<mvg::multi_iterator<Ts...>>
{
private:
	using It = mvg::multi_iterator<Ts...>;
public:
	using value_type = typename It::value_type;
	using difference_type = typename It::difference_type;
	using pointer = typename It::pointer;
	using reference = typename It::reference;
	using iterator_category = typename It::iterator_category;
};

} //namespace std

#endif
