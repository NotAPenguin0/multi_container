#ifndef MVG_MULTI_CONTAINER_HPP_
#define MVG_MULTI_CONTAINER_HPP_

#include <tuple>
#include <iterator>
#include <limits>
#include <initializer_list>

#include "multi_iterator.hpp"

namespace mvg
{

namespace detail
{

template<typename T>
struct underlying_iterator
{
	using type = typename T::iterator;
};

template<typename T>
struct underlying_iterator<T*>
{
	using type = std::add_pointer_t<T>;
};

template<typename T, std::size_t N>
struct underlying_iterator<T[N]>
{
	using type = std::add_pointer_t<T>;
};

template<typename T>
struct underlying_const_iterator
{
	using type = typename T::const_iterator;
};

template<typename T>
struct underlying_const_iterator<T*>
{
	using type = std::add_const_t<std::add_pointer_t<T>>;
};

template<typename T, std::size_t N>
struct underlying_const_iterator<T[N]> 
{
	using type = std::add_const_t<std::add_pointer_t<T>>;
};

struct multi_size
{
	multi_size()
	{
		val = std::numeric_limits<std::size_t>::max();
	}

	template<typename T>
	void operator()(T const& cont)
	{
		if (cont.size() < val)
		{
			val = cont.size();
		}
	}

	std::size_t value()
	{
		return val;
	}

private:
	std::size_t val;
};

struct multi_clear
{
	template<typename T>
	void operator()(T& cont)
	{
		cont.clear();
	}
};

struct multi_push_back
{
	template<typename C, typename T>
	void operator()(C& c, T&& elem)
	{
		c.push_back(std::forward<T>(elem));
	}
};

struct multi_pop_back
{
	template<typename C>
	void operator()(C& c)
	{
		c.pop_back();
	}
};

class multi_insert
{
	//Return type has to be type of iterator at index I, which also is the iterator of the container in Cs... at index I,
	//But auto is a lot easier
	template<std::size_t I, typename It, typename... Cs, typename... Ts>
	static auto
	do_insert_helper(It const& pos, std::tuple<Cs...>& conts, std::tuple<Ts...> const& elems)
	{
		return std::get<I>(conts).insert(std::get<I>(pos.m_iterators), std::get<I>(elems));
	}

	template<typename It, typename... Cs, typename...Ts, std::size_t... Is>
	static It do_insert(It const& pos, std::tuple<Cs...>& conts, std::tuple<Ts...> const& elems, std::index_sequence<Is...>)
	{
		return mvg::multi_iterator(do_insert_helper<Is>(pos, conts, elems) ...);
//		(std::get<Is>(conts).insert((*pos).get_elem<Is>()..., std::get<Is>(elems)) ...);
	}
public:
	template<typename It, typename...Cs, typename...Ts>
	static It insert(It const& pos, std::tuple<Cs...>& conts, std::tuple<Ts...> const& elems)
	{
		return do_insert(
			pos, 
			conts,
			elems,
			std::index_sequence_for<Ts...> {});
		
	}
};


class multi_erase
{
private:
	template<std::size_t I, typename It, typename... Cs>
	static auto do_erase_helper(std::tuple<Cs...>& conts, It pos)
	{
		return std::get<I>(conts).erase(std::get<I>(pos.m_iterators));
	}

	template<typename It, typename... Cs, std::size_t... Is>
	static It do_erase(std::tuple<Cs...>& conts, It pos, std::index_sequence<Is...>)
	{
		return mvg::multi_iterator(do_erase_helper<Is>(conts, pos) ...);
	}

public:
	template<typename It, typename... Cs>
	static It erase(std::tuple<Cs...>& conts, It pos)
	{
		return do_erase(conts, pos, std::index_sequence_for<Cs...> {});
	}
};

} //namespace detail

//Warning: using structured binding gives reference, even when doing for(auto[a, b, c] : m) !!
template<typename... Ts>
class multi_container
{
public:
	using iterator = multi_iterator<typename detail::underlying_iterator<Ts>::type ...>;
	using const_iterator = multi_iterator<typename detail::underlying_const_iterator<Ts>::type ...>;
	using value_type = detail::tuple_wrapper<Ts...>;
	using reference = detail::tuple_wrapper<std::add_lvalue_reference_t<Ts> ...>;
	using pointer = std::add_pointer_t<value_type>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::add_const_t<reverse_iterator>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	multi_container()
	{
		
	}


	multi_container(Ts const&... containers) : m_containers(containers...)
	{

	}

	multi_container(std::add_rvalue_reference_t<Ts>... containers) :
		m_containers(std::forward<std::add_rvalue_reference_t<Ts>>(containers)...)
	{
	}

	multi_container(std::tuple<Ts const&>... containers) :
		m_containers(containers)
	{
	}
	
	multi_container(multi_container const&) = default;
	multi_container(multi_container&&) = default;

	multi_container& operator=(multi_container const& rhs)
	{
		m_containers = rhs.m_containers;
		return *this;
	}

	multi_container& operator=(multi_container&& rhs)
	{
		m_containers = std::move(rhs.m_containers);
	}

	iterator begin()
	{
		return iterator(std::begin(std::get<Ts>(m_containers)) ...);
	}

	iterator end()
	{
		return iterator(std::end(std::get<Ts>(m_containers)) ...);
	}

	const_iterator begin() const
	{
		return const_iterator(std::begin(std::get<Ts>(m_containers)) ...);
	}

	const_iterator end() const
	{
		return const_iterator(std::end(std::get<Ts>(m_containers)) ... );
	}

	const_iterator cbegin() const
	{
		return const_iterator(std::begin(std::get<Ts>(m_containers)) ... );
	}
	
	const_iterator cend() const
	{
		return const_iterator(std::end(std::get<Ts>(m_containers)) ... );
	}

	reverse_iterator rbegin()
	{
		return reverse_iterator { end() };
	}

	reverse_iterator rend()
	{
		return reverse_iterator { begin() };
	}

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator { end() };
	}

	const_reverse_iterator crend() const
	{
		return const_reverse_iterator { begin() };
	}

	std::tuple<Ts...>& data()
	{
		return m_containers;
	}

	std::tuple<Ts...> const& data() const
	{
		return m_containers;
	}

	std::size_t size() const
	{
		detail::multi_size sz;
		TupleForEach::foreach(m_containers, sz);
		return sz.value();
	}

	auto operator[](std::size_t index)
	{
		dbg_assert((index < size()), "multi_container iterator out of range");
		return *(begin() + index);
	}

	auto operator[](std::size_t index) const
	{
		dbg_assert((index < size()), "multi_container iterator out of range");
		return *(begin() + index);
	}

	auto at(std::size_t index)
	{
		if (index > size())
		{
			throw std::out_of_range("multi_container iterator out of range");
		}
		return *(begin() + index);
	}

	auto at(std::size_t index) const
	{
		if (index > size())
		{
			throw std::out_of_range("multi_container iterator out of range");
		}
		return *(begin() + index);
	}

	auto front()
	{
		return *begin();
	}

	auto front() const
	{
		return *begin();
	}

	auto back()
	{
		return *(end() - 1);
	}

	auto back() const
	{
		return *(end() - 1);
	}

	bool empty() const
	{
		return size() == 0;
	}

	void clear()
	{
		TupleForEach::foreach(m_containers, detail::multi_clear {});
	}

	template<typename... Elems>
	void push_back(std::tuple<Elems...> const& elems)
	{
		static_assert(sizeof...(Elems) == sizeof...(Ts), "Invalid argument count");
		TupleForEach::parallel_foreach(m_containers, elems, detail::multi_push_back {});
	}

	//Insert element after pos
	template<typename... Elems>
	iterator insert(iterator pos, std::tuple<Elems...> const& elems)
	{
		static_assert(sizeof...(Elems) == sizeof...(Ts), "Invalid argument count");
		return detail::multi_insert::insert(pos, m_containers, elems);
	}

	template<typename... Elems>
	iterator insert(const_iterator pos, std::tuple<Elems...> const& elems)
	{
		static_assert(sizeof...(Elems) == sizeof...(Ts), "Invalid argument count");
		return detail::multi_insert::insert(pos, m_containers, elems);
	}

	//Does not check if [first, last[ is a valid range
	template<typename InputIt>
	iterator insert(iterator pos, InputIt first, InputIt last)
	{
		while(first != last)
		{
			pos = detail::multi_insert::insert(pos, m_containers, (*first).m_tuple);
			++pos;
			++first;
		}
		return pos;
	}

	void insert(iterator pos, size_type count, std::tuple<Ts...> const& elem)
	{
		for (int i = 0; i < count; ++i)
		{
			pos = insert(pos, elem);
		}
	}

	iterator erase(iterator pos)
	{
		return detail::multi_erase::erase(m_containers, pos);
	}

	iterator erase(const_iterator pos)
	{
		return detail::multi_erase::erase(m_containers, pos);
	}

	iterator erase(iterator first, iterator last)
	{
		std::size_t count = std::distance(first, last);
		iterator pos = first;
		for(std::size_t i = 0; i < count; ++i)
		{
			pos = detail::multi_erase::erase(m_containers, pos);		
		}
		return pos;
	}

	iterator erase(const_iterator first, const_iterator last)
	{
		std::size_t count = std::distance(first, last);
		iterator pos = first;
		for (std::size_t i = 0; i < count; ++i)
		{
			pos = detail::multi_erase::erase(m_containers, pos);
		}
		return pos;
	}

	void pop_back()
	{
		TupleForEach::foreach(m_containers, detail::multi_pop_back {});
	}

	template<typename T>
	T& get_container()
	{
		return std::get<T>(m_containers);
	}

	template<typename T>
	T const& get_container() const
	{
		return std::get<T>(m_containers);
	}

	template<std::size_t I>
	std::add_lvalue_reference_t<std::tuple_element_t<I, std::tuple<Ts...>>>
	get_container()
	{
		return std::get<I>(m_containers);
	}

	template<std::size_t I>
	std::add_const_t<std::add_lvalue_reference_t<std::tuple_element_t<I, std::tuple<Ts...>>>>
	get_container() const
	{
		return std::get<I>(m_containers);
	}

private:
	std::tuple<Ts...> m_containers;
};

template<typename...Ts>
using tied_container = multi_container<Ts...>;

} //namespace mvg

#endif
