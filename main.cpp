#include "multi_container.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <algorithm>
#include <sstream>
#include <memory>
#include <array>
#include <limits>

void f(mvg::multi_container<std::vector<int>, std::vector<float>, std::array<long long, 5>> const& foo)
{
	for (auto const[a, b, c] : foo)
	{
		std::cout << a << " " << b << " " << c << "\n";
	}
}

struct X
{
	void f()
	{
		std::cout << "Hi from X!\n";
	}
};


int main()
{
	std::vector<int> vi { 0, 1, -1, 2, -2 };
	std::vector<float> vf {11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
	std::array<long long, 5> all = { 1, 2, 3, 4, 5 };

	mvg::multi_container multiCont { vi, vf, all };

	for (auto[order, f, ll] : multiCont)
	{
		++order;
		++f;
		++ll;
	}

	for (auto[order, f, ll] : multiCont)
	{
		
		std::cout << order << " " << f << " " << ll << "\n";
	}

	std::vector<X> vx { X{}, X{} };
	std::list<X> lx { X{}, X{} };

	mvg::multi_iterator xit { vx.begin(), lx.begin() };

	(*xit).get_elem<0>().f();
	
	std::sort(multiCont.rbegin(), multiCont.rend());

	multiCont[2].get_elem<int>() = 42;

	mvg::multi_container m { vi, vf };

	std::vector<int> vi2 = { -10, -11, -12 };
	std::vector<float> vf2 = { -2.0f, -3.0f, -4.0f };

	mvg::multi_container n { vi2, vf2 };

	m.push_back(std::make_tuple(1, 2.0f));
	m.insert(m.begin(), n.begin(), n.end());
	n.erase(n.begin(), n.end());

	mvg::multi_container l { vx, lx };
	l.clear();

	std::cin.ignore(std::numeric_limits<std::streamsize>::max());

	return 0;
}
