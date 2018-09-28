# multi_container

Container that is capable of storing and iterating over any number of other containers. Another name would be `tied_container`, that's why I included an alias for that. It also features a `multi_iterator` class that is used for iterating over it, and works much like a `zip_iterator`. One important note is that `multi_container` owns the containers it stores, and copies the containers given to it in the constructor.

# Examples

We'll start by declaring some containers to use in the following examples.

```cpp
std::vector<int> vi { 0, 1, -1, 2, -2 };
std::vector<float> vf {11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
std::array<long long, 5> all = { 1, 2, 3, 4, 5 };
```

Now, we declare our `multi_container`. Luckily we have type deduction from the constructor, so there is no need to specify all container types manually:

```cpp
mvg::multi_container m(vi, vf, all);
```

I will start off immediately by showing off what I think is the best feature: *support for structured bindings.*

```cpp
for(auto[a, b, c] : m)
{
  //use a, b and c
}
```

This does require some explanation, because you can probably see something with this: There is no `auto&`, just `auto`. Still, `a`, `b` and `c` are references to the elements. This is because under the hood, the `multi_iterator` used for iterating over the `multi_container` uses a wrapper around `std::tuple` for references to return when dereferencing. So even when copying the result of the dereference, you keep an iterator. This does have the downside that you can't explicitely add `const`, e.g

```cpp
for(auto const[a, b, c] : m)
{

}
```

Will not give const references to `a`, `b` and `c`.
