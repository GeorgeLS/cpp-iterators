#ifndef ITERATOR__ITERATOR_H
#define ITERATOR__ITERATOR_H

#include <cstddef>
#include <functional>
#include <unordered_set>

/**
 * Summary:
 *      Internal namespace providing helper traits for the iterators design
 */
namespace internal {
#define MUST_RETURN_BOOL "The return value of the predicate must be bool"
#define ASSERT_RETURNS_BOOL(p, a) \
  static_assert(internal::returns_bool_v<p, a>, MUST_RETURN_BOOL)

/**
 * Summary:
 *      Trait that determines whether a callable of type `F` taking
 *      arguments of type `Args...` return value of type `R`
 *
 * @tparam R:    The type of the return value
 * @tparam F:    The type of the callable
 * @tparam Args: The type(s) of the arguments that the callable accepts
 */
template<typename R, typename F, typename... Args>
inline constexpr bool returns_type_v = std::is_same_v<std::result_of_t<F(Args...)>, R>;

/**
 * Summary:
 *      Determines whether a callable of type `F` that takes argument(s)
 *      of type(s) `Args...` returns bool
 *
 * @tparam F:    The type of the callable
 * @tparam Args: The type(s) of the arguments that the callable accepts
 */
template<typename F, typename ... Args>
inline constexpr bool returns_bool_v = returns_type_v<bool, F, Args...>;

template<typename T>
struct is_ref_wrapper {
  static constexpr bool value = false;
};

template<typename T>
struct is_ref_wrapper<std::reference_wrapper<T>> {
  static constexpr bool value = true;
};

template<typename T>
inline constexpr bool is_ref_wrapper_v = is_ref_wrapper<T>::value;

/**
 * Summary:
 *      Trait to strip std::reference_wrapper from a type
 *
 * @tparam T: The type that we want to remove std::reference_wrapper
 */
template<typename T>
struct strip_ref_wrapper {
  using type = T;
};

/**
 * Summary:
 *      Trait to strip std::reference_wrapper from a type
 *
 * @tparam T: The type that we want to remove std::reference_wrapper
 */
template<typename T>
struct strip_ref_wrapper<std::reference_wrapper<T>> {
  using type = T;
};

/**
 * Summary:
 *      Extracts the type `T` without std::reference_wrapper
 *      if it is a reference wrapper
 */
template<typename T>
using strip_ref_wrapper_t = typename strip_ref_wrapper<T>::type;

template<typename T>
struct unwrap_ref_wrapper {
  using type = T;
};

template<typename T>
struct unwrap_ref_wrapper<std::reference_wrapper<T>> {
  using type = T &;
};

template<typename T>
using unwrap_ref_wrapper_t = typename unwrap_ref_wrapper<T>::type;

/**
 * Summary:
 *      Extracts the ItemType that an iterator of type
 *      `IteratorType` yields.
 *
 * @tparam IteratorType: The type of the iterator
 */
template<typename IteratorType>
using item_type = typename IteratorType::ItemType;

template<typename IteratorType>
using unwraped_item_type = unwrap_ref_wrapper_t<typename IteratorType::ItemType>;
}

// Forward declare Iterator
template<typename ItemType, typename IteratorType> struct Iterator;

/**
 * Summary:
 *      An iterator type that steps by some amount other iterators.
 *      To get an iterator of this type, invoke `step_by` function on
 *      an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator to step by
 *
 * @example:
 * ```
 * Array<size_t> ints(5);
 * for (size_t i = 0U; i != ints.len(); ++i) {
 *      ints[i] = i + 1;
 * }
 *
 * // ints is: [1, 2, 3, 4, 5]
 *
 * auto res = ints.iter()
 *      .step_by(2)
 *      .collect<Array>();
 *
 * // res is: [1, 3, 5]
 * ```
 */
template<typename IteratorType>
struct StepBy : public Iterator<internal::item_type<IteratorType>, StepBy<IteratorType>> {
  using ItemType = internal::item_type<IteratorType>;

  StepBy(IteratorType it, size_t step) : inner{it}, step{step} {};

  std::optional<ItemType> next() {
    std::optional<ItemType> v = inner.next();

    if (v.has_value()) {
      for (size_t i = 1U; i != step; ++i) {
        if (!inner.next().has_value()) {
          break;
        }
      }
    }

    return v;
  }

  IteratorType inner;
  size_t step;
};

/**
 * Summary:
 *      An iterator type that maps items yielded from the
 *      underlying iterator to some other value using a
 *      callable object of type `MapF`.
 *      To get an iterator of this type, invoke `map` method
 *      on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 * @tparam MapF:         The type of the callable that performs the mapping
 *
 * @example:
 * ```
 * Array<int> ints(2);
 * for (size_t i = 0U; i != ints.len(); ++i) {
 *      ints[i] = (int) i + 1;
 * }
 *
 * // ints is: [1, 2]
 *
 * auto mapped = ints.iter()
 *      .map([](const int &v) { return "Mapped value is " + std::to_string(v); })
 *      .collect<Array>();
 *
 * // mapped is: ["Mapped value is 1", "Mapped value is 2"]
 * ```
 */
template<typename IteratorType, typename MapF>
struct Map : public Iterator<std::result_of_t<MapF(internal::unwraped_item_type<IteratorType>)>,
                             Map<IteratorType, MapF>> {

  using ItemType = std::result_of_t<MapF(internal::unwraped_item_type<IteratorType>)>;

  explicit Map(IteratorType it, MapF mapper) : inner{it}, mapper{mapper} {}

  std::optional<ItemType> next() {
    auto v = inner.next();
    if (v.has_value()) {
      return std::make_optional(mapper(*v));
    } else {
      return std::nullopt;
    }
  }

  IteratorType inner;
  MapF mapper;
};

/**
 * Summary:
 *      An iterator that skips other iterators by some amount.
 *      The skip is performed on the first call to `next` and
 *      then all subsequent items are yielded from the underlying
 *      iterator. To get an iterator of this type, invoke `skip`
 *      method on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 *
 * @example:
 * ```
 * Array<int> ints(10);
 * for (size_t i = 0U; i != ints.len(); ++i) {
 *      ints[i] = (int) i + 1;
 * }
 *
 * // ints is: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
 *
 * // This will print 9 and 10
 * ints.iter()
 *     .skip(8)
 *     .foreach([](const int &v) { printf("%d\n", v); });
 * ```
 */
template<typename IteratorType>
struct Skip : public Iterator<internal::item_type<IteratorType>, Skip<IteratorType>> {
  using ItemType = internal::item_type<IteratorType>;

  Skip(IteratorType it, size_t skip) : inner{it}, skip{skip} {}

  std::optional<ItemType> next() {
    if (skip) {
      for (size_t i = 0U; i != skip; ++i) {
        inner.next();
      }
      skip = 0;
    }

    return inner.next();
  }

  IteratorType inner;
  size_t skip;
};

/**
 * Summary:
 *      An iterator that skips other iterators based on a predicate.
 *      The iterator will start skipping items yielded by the underlying
 *      iterator on the first call to `next`. It will invoke the given predicate
 *      on each item and while the predicate returns true, it will skip them.
 *      On the next `next` calls, it will yield the subsequent items without calling
 *      the predicate. To get an iterator of this type, invoke `skip_while` method
 *      on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 * @tparam Predicate:    The type of callable that accepts an iterator item of type `ItemType`
 *                       and return `true` or `false`
 *
 * @example:
 * ```
 * Array<size_t> arr(10);
 * for (size_t i = 0U; i != arr.len(); ++i) {
 *      arr[i] = i + 1;
 * }
 *
 * // arr is: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
 *
 * auto skipped = arr.iter()
 *      .skip_while([](const size_t &v) { return v < 5; })
 *      .collect<Array>();
 *
 * // skipped is: [5, 6, 7, 8, 9, 10]
 * ```
 */
template<typename IteratorType, typename Predicate>
struct SkipWhile : public Iterator<internal::item_type<IteratorType>, SkipWhile<IteratorType, Predicate>> {
  ASSERT_RETURNS_BOOL(Predicate, internal::unwraped_item_type<IteratorType>);

  using ItemType = internal::item_type<IteratorType>;

  SkipWhile(IteratorType it, Predicate p) : inner{it}, predicate{p}, skipped{false} {}

  std::optional<ItemType> next() {
    if (skipped) {
      return inner.next();
    }

    std::optional<ItemType> v{};
    do {
      v = inner.next();
    } while (v.has_value() && predicate(*v));
    skipped = true;

    return v;
  }

  IteratorType inner;
  Predicate predicate;
  bool skipped;
};

/**
 *  Summary:
 *      Enhances an iterator by yielding the items from it and return subsequent indexes starting from 0.
 *      The return type of this iterator is `std::pair<size_t, IteratorType::ItemType>` which can also be
 *      destructured using the `auto[...]` syntax.
 *      To get an iterator of this type, invoke `enumerate` method on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 *
 * @example:
 * ```
 * Array<const char *> strings(3);
 * strings[0] = "First String";
 * strings[1] = "Second String";
 * strings[2] = "Third String";
 *
 * // This will print the following:
 * // Index: 0, String: First String
 * // Index: 1, String: Second String
 * // Index; 2, String: Third String
 * foreach(it, strings.iter().enumerate()) {
 *      auto[index, str] = *it;
 *      printf("Index: %zu, String: %s\n", index, str);
 * }
 * ```
 */
template<typename IteratorType>
struct Enumerate : public Iterator<std::pair<size_t, internal::item_type<IteratorType>>, Enumerate<IteratorType>> {
  using ItemType = std::pair<size_t, internal::item_type<IteratorType>>;

  explicit Enumerate(IteratorType it) : inner{it}, index{0U} {}

  std::optional<ItemType> next() {
    auto v = inner.next();

    if (v.has_value()) {
      ++index;
      return std::make_optional(std::make_pair(index - 1, *v));
    } else {
      return std::nullopt;
    }
  }

  IteratorType inner;
  size_t index;
};

/**
 * Summary:
 *      An iterator that filters elements yielded by other iterators based on a predicate.
 *      On each call of `next`, the `Filter` iterator yields an item and calls the predicate
 *      of type `Predicate` on the item. If the predicate returns false, then it yields again,
 *      until the predicate returns true or all items have been yielded.
 *      To get an iterator of this type, invoke `filter` method on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 * @tparam Predicate:    The type of predicate that determines whether an item should be filtered or not.
 *
 * @example:
 * ```
 * Array<int> ints(10);
 * for (size_t i = 0U; i != ints.len(); ++i) {
 *      ints[i] = (int) i + 1;
 * }
 *
 * // ints is: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
 *
 * auto evens = ints.iter()
 *      .filter([](const int &v) { return v % 2 == 0; })
 *      .collect<Array>();
 *
 * // evens is: [2, 4, 6, 8, 10]
 * ```
 */
template<typename IteratorType, typename Predicate>
struct Filter : public Iterator<internal::item_type<IteratorType>, Filter<IteratorType, Predicate>> {
  ASSERT_RETURNS_BOOL(Predicate, internal::unwraped_item_type<IteratorType>);

  using ItemType = internal::item_type<IteratorType>;

  Filter(IteratorType it, Predicate p) : inner{it}, predicate{p} {}

  std::optional<ItemType> next() {
    std::optional<ItemType> v{};
    do {
      v = std::move(inner.next());
    } while (v.has_value() && !predicate(*v));
    return v;
  }

  IteratorType inner;
  Predicate predicate;
};

/**
 * Summary:
 *      An iterator that chains multiple iterators together.
 *      Starts by yielding items from the first iterator and when it consumes
 *      it completely, it starts yielding items from the second iterator.
 *      Both iterators must yield items of the same type.
 *      To get an iterator of this type, invoke `chain` method on an iterator.
 *
 * @tparam FirstIterator:  The type of the first iterator
 * @tparam SecondIterator: The type of the second iterator
 *
 * @example:
 * ```
 * Array<size_t> a1(3);
 * for (size_t i = 0U; i != a1.len(); ++i) {
 *      a1[i] = i + 1;
 * }
 *
 * // a1 is: [1, 2, 3]
 *
 * Array<size_t> a2(4);
 * for (size_t i = 0U; i != a2.len(); ++i) {
 *      a2[i] = a2.len() - i;
 * }
 *
 * // a2 is: [4, 3, 2, 1]
 *
 * auto res = a1.iter()
 *      .chain(a2.iter())
 *      .chain(a1.iter())
 *      .collect<Array>();
 *
 * // res is: [1, 2, 3, 4, 3, 2, 1, 1, 2, 3]
 * ```
 */
template<typename FirstIterator, typename SecondIterator>
struct Chain : public Iterator<internal::item_type<FirstIterator>, Chain<FirstIterator, SecondIterator>> {
  static_assert(std::is_same_v<internal::item_type<FirstIterator>, internal::item_type<SecondIterator >>,
                "The item types of the first and second iterator must be the same");

  using ItemType = internal::item_type<FirstIterator>;

  Chain(FirstIterator first, SecondIterator second) : first{first}, second{second} {}

  std::optional<ItemType> next() {
    auto v = first.next();

    if (!v.has_value()) {
      v = second.next();
    }

    return v;
  }

  FirstIterator first;
  SecondIterator second;
};

/**
 * Summary:
 *      An iterator that zips together items yielded from two iterators.
 *      On each call to `next`, this iterator will advance both iterators
 *      and will return an `std::pair` containing both items.
 *      If at least one of the iterator does not yield an item, then this
 *      iterator will stop yielding items as well.
 *      Iterators can return items of different type.
 *      To get an iterator of this type, invoke `zip` method on an iterator.
 *
 * @tparam FirstIterator:  The type of the first iterator
 * @tparam SecondIterator: The type of the second iterator
 *
 * @example:
 * ```
 * Array<size_t> indexes(3);
 * for (size_t i = 0U; i != indexes.len(); ++i) {
 *      indexes[i] = i;
 * }
 *
 * // indexes is: [0, 1, 2]
 *
 * Array<const char *> strings(3);
 * strings[0] = "String_0";
 * strings[1] = "String_1";
 * strings[2] = "String_2";
 *
 * auto zipped = indexes.iter()
 *      .zip(strings.iter())
 *      .collect<Array>();
 *
 * // zipped is: [(0, "String_0"), (1, "String_1"), (2, "String_2")]
 * ```
 */
template<typename FirstIterator, typename SecondIterator>
struct Zip : public Iterator<std::pair<internal::item_type<FirstIterator>, internal::item_type<SecondIterator>>,
                             Zip<FirstIterator, SecondIterator>> {

  using FirstItemType = internal::item_type<FirstIterator>;
  using SecondItemType = internal::item_type<SecondIterator>;
  using ItemType = std::pair<FirstItemType, SecondItemType>;

  Zip(FirstIterator first, SecondIterator second) : first{first}, second{second} {}

  std::optional<std::pair<FirstItemType, SecondItemType>> next() {
    auto first_value = first.next();

    if (first_value.has_value()) {
      auto second_value = second.next();
      if (second_value.has_value()) {
        return std::make_optional(std::make_pair(first_value.value(), second_value.value()));
      } else {
        return std::nullopt;
      }
    }

    return std::nullopt;
  }

  FirstIterator first;
  SecondIterator second;
};

/**
 * Summary:
 *      An iterator that yields at most some amount of items from another iterator.
 *      This iterator type is usually used to "convert" infinite iterators to finite ones.
 *      To get an iterator of this type, invoke `take` method on an iterator.
 *
 * @tparam IteratorType: The underlying iterator type
 *
 * @example:
 * ```
 * Array<int> ints(2);
 * ints[0] = 1;
 * ints[1] = 2;
 *
 * auto res = ints.iter()
 *      .cycle() // cycle is an infinite iterator yielding items in a circular fashion.
 *      .take(4)
 *      .collect<Array>();
 *
 * // res is: [1, 2, 1, 2]
 * ```
 */
template<typename IteratorType>
struct Take : public Iterator<internal::item_type<IteratorType>, Take<IteratorType>> {
  using ItemType = internal::item_type<IteratorType>;

  Take(IteratorType it, size_t num) : inner{it}, num{num} {}

  std::optional<ItemType> next() {
    if (num != 0) {
      --num;
      return inner.next();
    }

    return std::nullopt;
  }

  IteratorType inner;
  size_t num;
};

/**
 * Summary:
 *      An iterator that yields items while a condition is met.
 *      The condition on which we are going to yield is defined by
 *      the predicate passed.
 *      To get an iterator of this type, invoke `take_while` method on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 * @tparam Predicate:    The type of the predicate that defines whether to take or stop
 *
 * @example:
 * ```
 * Array<size_t> arr(10);
 * for (size_t i = 0U; i != arr.len(); ++i) {
 *      arr[i] = i + 1;
 * }
 *
 * // arr is: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
 *
 * // This will print the following:
 * // Value: 1
 * // Value: 2
 * // Value: 3
 * // Value: 4
 * // Value: 5
 * // Value: 6
 * arr.iter()
 *    .take_while([](const size_t &v) { return v < 7; })
 *    .for_each([](const size_t &v) { printf("Value: %zu\n", v); });
 * ```
 */
template<typename IteratorType, typename Predicate>
struct TakeWhile : public Iterator<internal::item_type<IteratorType>, TakeWhile<IteratorType, Predicate>> {
  ASSERT_RETURNS_BOOL(Predicate, internal::unwraped_item_type<IteratorType>);

  using ItemType = internal::item_type<IteratorType>;

  TakeWhile(IteratorType it, Predicate p) : inner{it}, predicate{p}, stopped_taking{false} {}

  std::optional<ItemType> next() {
    if (!stopped_taking) {
      auto v = inner.next();
      if (v.has_value() && predicate(*v)) {
        return v;
      }

      stopped_taking = true;
    }

    return std::nullopt;
  }

  IteratorType inner;
  Predicate predicate;
  bool stopped_taking;
};

/**
 * Summary:
 *      An infinite iterator that yields items from an iterator in a circular fashion.
 *      That means, when the last item is yielded then the it starts from the beginning.
 *      To get an iterator of this type, invoke `cycle` method on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 *
 * @example:
 * ```
 * Array<int> ints(2);
 * ints[0] = 1;
 * ints[1] = 2;
 *
 * // Prints 1 and 2 indefinitely.
 * ints.iter()
 *     .cycle()
 *     .for_each([](const int &v) { printf("%d\n", v); });
 * ```
 */
template<typename IteratorType>
struct Cycle : public Iterator<internal::item_type<IteratorType>, Cycle<IteratorType>> {
  using ItemType = internal::item_type<IteratorType>;

  explicit Cycle(IteratorType it) : original{it}, inner{it} {}

  std::optional<ItemType> next() {
    auto v = inner.next();
    if (v.has_value()) {
      return v;
    }

    inner = original;

    return inner.next();
  }

  IteratorType original;
  IteratorType inner;
};

/**
 * Summary:
 *      An iterator that interleaves the items from two iterators.
 *      On each call to `next` one item is yielded. On first call the first iterator
 *      is advanced, on second call the second iterator, etc.
 *      If the iterators don't have the same number of items, then the items of
 *      the longest iterator will be yielded one after another when the smallest one
 *      has been consumed.
 *      Both iterators must return items of the same type.
 *      To get an iterator of this type, invoke `interleave` method on an iterator.
 *
 * @tparam FirstIterator:  The type of the first iterator
 * @tparam SecondIterator: The type of the second iterator
 *
 * @example:
 * ```
 * Array<int> a1(4);
 * a1[0] = 1;
 * a1[1] = 2;
 * a1[2] = 3;
 * a1[3] = 4;
 *
 * Array<int> a2(2);
 * a2[0] = -1;
 * a2[1] = -2;
 *
 * auto interleaved = a1.iter()
 *      .interleave(a2.iter())
 *      .collect<Array>();
 *
 * // interleaved is: [1, -1, 2, -2, 3, 4]
 * ```
 */
template<typename FirstIterator, typename SecondIterator>
struct Interleave : public Iterator<internal::item_type<FirstIterator>, Interleave<FirstIterator, SecondIterator>> {
  static_assert(std::is_same_v<internal::item_type<FirstIterator>, internal::item_type<SecondIterator >>,
                "FirstIterator and SecondIterator must yield the same ItemTypes");

  using ItemType = internal::item_type<FirstIterator>;

  Interleave(FirstIterator first, SecondIterator second) : first{first}, second{second}, yield_first{true} {}

  std::optional<ItemType> next() {
    auto v = yield_first ? first.next() : second.next();
    if (!v.has_value()) {
      v = yield_first ? second.next() : first.next();
    }
    yield_first = !yield_first;
    return v;
  }

  FirstIterator first;
  SecondIterator second;
  bool yield_first;
};

/**
 * Summary:
 *      An iterator that interleaves the items from two iterators.
 *      On each call to `next` one item is yielded. On first call the first iterator
 *      is advanced, on second call the second iterator, etc.
 *      If the iterators don't have the same number of items, then this iterator
 *      will stop yielding items as soon as the smallest one has been consumed.
 *      Both iterators must return items of the same type.
 *      To get an iterator of this type, invoke `interleave_shortest` method on an iterator.
 *
 * @tparam FirstIterator:  The type of the first iterator
 * @tparam SecondIterator: The type of the second iterator
 *
 * @example:
 * ```
 * Array<int> a1(4);
 * a1[0] = 1;
 * a1[1] = 2;
 * a1[2] = 3;
 * a1[3] = 4;
 *
 * Array<int> a2(2);
 * a2[0] = -1;
 * a2[1] = -2;
 *
 * auto interleaved = a1.iter()
 *      .interleave_shortest(a2.iter())
 *      .collect<Array>();
 *
 * // interleaved is: [1, -1, 2, -2, 3]
 * ```
 */
template<typename FirstIterator, typename SecondIterator>
struct InterleaveShortest : public Iterator<internal::item_type<FirstIterator>,
                                            InterleaveShortest<FirstIterator, SecondIterator>> {
  static_assert(std::is_same_v<internal::item_type<FirstIterator>, internal::item_type<SecondIterator>>,
                "FirstIterator and SecondIterator must yield the same ItemTypes");

  using ItemType = internal::item_type<FirstIterator>;

  InterleaveShortest(FirstIterator first, SecondIterator second) : first{first}, second{second}, yield_first{true} {};

  std::optional<ItemType> next() {
    auto v = yield_first ? first.next() : second.next();
    if (!v.has_value()) {
      return std::nullopt;
    }
    yield_first = !yield_first;
    return v;
  }

  FirstIterator first;
  SecondIterator second;
  bool yield_first;
};

/**
 * Summary:
 *      An iterator that yields the unique items from another iterator.
 *      Uniqueness is determined by hashing an equality comparison.
 *      You will have to implement `std::hash`, `std::equal_to` functors
 *      for the iterator `ItemType` to achieve that.
 *      To get an iterator of this type, invoke `unique` method on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 *
 * @example:
 * ```
 * Array<int> ints(10);
 * ints[0] = 1;
 * ints[1] = 1;
 * ints[2] = 1;
 * ints[3] = 1;
 * ints[4] = 1;
 * ints[5] = 3;
 * ints[6] = 100;
 * ints[7] = -1;
 * ints[8] = 2;
 * ints[9] = 1;
 *
 * auto unique = ints.iter()
 *      .unique()
 *      .collect<Array>();
 *
 * // unique is: [1, 3, 100, -1, 2]
 * ```
 */
template<typename IteratorType>
struct Unique : public Iterator<internal::strip_ref_wrapper_t<internal::item_type<IteratorType>>,
                                Unique<IteratorType>> {
  using ItemType = internal::strip_ref_wrapper_t<internal::item_type<IteratorType>>;

  explicit Unique(IteratorType it) : inner{it}, set{} {}

  std::optional<ItemType> next() {
    auto v = inner.next();
    while (v.has_value() && set.find(*v) != set.end()) {
      v = inner.next();
    }
    if (v.has_value()) {
      set.insert(*v);
    }
    return v;
  }

  IteratorType inner;
  std::unordered_set<ItemType> set;
};

/**
 * Summary:
 *      An iterator that yields the unique items out of another iterator.
 *      The uniqueness is tested against the metric returned by given function
 *      of type `F`.
 *      Uniqueness is determined based on hashing and equality comparison.
 *      You will need to implement `std::hash`, `std::equal_to` functors for the
 *      type of items returned by function of type `F`.
 *      To get an iterator of this type, invoke `unique_by` method on an iterator.
 *
 * @tparam IteratorType: The type of the underlying iterator
 * @tparam F:            The type of the function that determines the uniqueness property
 *
 * @example:
 * ```
 * Array<const char*> strings(5);
 * strings[0] = "a";
 * strings[1] = "aa";
 * strings[2] = "b";
 * strings[3] = "ccc";
 * strings[4] = "c";
 *
 * auto unique_by_length = strings.iter()
 *      .unique_by([](const char *s) { return strlen(s); })
 *      .collect<Array>();
 *
 * // unique_by_length is: ["a", "aa", "ccc"]
 * ```
 */
template<typename IteratorType, typename F>
struct UniqueBy : public Iterator<internal::strip_ref_wrapper_t<internal::item_type<IteratorType>>,
                                  UniqueBy<IteratorType, F>> {
  using ItemType = internal::strip_ref_wrapper_t<internal::item_type<IteratorType>>;
  using KeyType = std::result_of_t<F(internal::unwraped_item_type<IteratorType>)>;

  UniqueBy(IteratorType it, F func) : inner{it}, set{}, func{func} {}

  std::optional<ItemType> next() {
    auto v = inner.next();
    while (v.has_value() && set.find(func(*v)) != set.end()) {
      v = inner.next();
    }
    if (v.has_value()) {
      set.insert(func(*v));
    }
    return v;
  }

  IteratorType inner;
  std::unordered_set<KeyType> set;
  F func;
};

/**
 * Summary:
 *      That's the heart of the iterator.
 *      It's a base class which uses CRTP (Curiously Recurring Template Pattern)
 *      to automatically implement all the iterator methods for a user defined
 *      iterator. It defines all the methods an iterator can have. All methods
 *      make use of the `next` method which is the only method you will need to
 *      implement. Everything else is already implemented. However, if you can
 *      provide a better implementation for some of the methods, then you can shadow
 *      the methods here by reimplementing them in the specific iterator type.
 *      What you need to provide when implementing an iterator is the type of the item
 *      that the iterator will yield (for references use std::reference_wrapper<T>).
 *      This type must be supplied when inheriting from the `Iterator` type as well
 *      as as a member type of your iterator. Also you need to implement the `next`
 *      method.
 *
 *      e.g:
 *      template<typename T>
 *      struct MyCollection {
 *          // In place of T in specialization below, pass std::reference_wrapper<T>
 *          // to yield by reference
 *          struct MyCollectionIterator : public Iterator<T, MyCollectionIterator> {
 *              using ItemType = T; // That's by value yielding. It will cause copy construction/assignment
 *              // using ItemType = std::reference_wrapper<T> if you want to yield by reference
 *
 *              std::optional<ItemType> next() { ... }
 *          };
 *      };
 *
 * @tparam ItemType:     The type of the item that the iterator will yield
 * @tparam IteratorType: The type of the iterator that is implementing the functionality
 */
template<typename ItemType, typename IteratorType>
struct Iterator {
  /**
   * Summary:
   *    Creates a `StepBy` iterator with given `step`
   *
   * @param step: The amount of stepping to perform
   * @return: A `StepBy` iterator
   */
  [[nodiscard]]
  StepBy<IteratorType> step_by(size_t step) {
    auto *it = static_cast<IteratorType *>(this);
    return StepBy<IteratorType>(*it, step);
  }

  /**
   * Summary:
   *    Creates a `Map` iterator with the given map function
   *
   * @tparam MapF:  The type of the mapping function
   * @param mapper: The mapping function
   * @return: A `Map` iterator
   */
  template<typename MapF>
  Map<IteratorType, MapF> map(MapF mapper) {
    auto *it = static_cast<IteratorType *>(this);
    return Map<IteratorType, MapF>(*it, mapper);
  }

  /**
   * Summary:
   *    Creates an `Enumerate` iterator
   *
   * @return: An `Enumerate` iterator
   */
  Enumerate<IteratorType> enumerate() {
    auto *it = static_cast<IteratorType *>(this);
    return Enumerate<IteratorType>(*it);
  }

  /**
   * Summary:
   *    Creates a `Skip` iterator with the given skip amount
   *
   * @param num: The number of items to skip
   * @return:    A `Skip` iterator
   */
  Skip<IteratorType> skip(size_t num) {
    auto *it = static_cast<IteratorType *>(this);
    return Skip<IteratorType>(*it, num);
  }

  /**
   * Summary:
   *    Creates a `SkipWhile` iterator with a given predicate
   *
   * @tparam Predicate: The type of the predicate
   * @param p:          The predicate to apply
   * @return:           A `SkipWhile` iterator
   */
  template<typename Predicate>
  SkipWhile<IteratorType, Predicate> skip_while(Predicate p) {
    auto *it = static_cast<IteratorType *>(this);
    return SkipWhile<IteratorType, Predicate>(*it, p);
  }

  /**
   * Summary:
   *    Creates a `Filter` iterator with a given predicate
   *
   * @tparam Predicate: The type of the predicate
   * @param p:          The predicate to apply
   * @return:           A `Filter` iterator
   */
  template<typename Predicate>
  Filter<IteratorType, Predicate> filter(Predicate p) {
    auto *it = static_cast<IteratorType *>(this);
    return Filter<IteratorType, Predicate>(*it, p);
  }

  /**
   * Summary:
   *    Creates a `Chain` iterator given a second iterator
   *
   * @tparam SecondIterator: The type of the second iterator
   * @param other:           The second iterator
   * @return:                A `Chain` iterator
   */
  template<typename SecondIterator>
  Chain<IteratorType, SecondIterator> chain(SecondIterator other) {
    auto *it = static_cast<IteratorType *>(this);
    return Chain<IteratorType, SecondIterator>(*it, other);
  }

  /**
   * Summary:
   *    Creates a `Zip` iterator given a second iterator
   *
   * @tparam SecondIterator: The type of the second iterator
   * @param other:           The second iterator
   * @return:                A `Zip` iterator
   */
  template<typename SecondIterator>
  Zip<IteratorType, SecondIterator> zip(SecondIterator other) {
    auto *it = static_cast<IteratorType *>(this);
    return Zip<IteratorType, SecondIterator>(*it, other);
  }

  /**
   * Summary:
   *    Creates a `Take` iterator given a take amount
   *
   * @param num: The number of elements to take
   * @return:    A `Take` iterator
   */
  Take<IteratorType> take(size_t num) {
    auto *it = static_cast<IteratorType *>(this);
    return Take<IteratorType>(*it, num);
  }

  /**
   * Summary:
   *    Creates a `TakeWhile` iterator given a predicate
   *
   * @tparam Predicate: The type of the predicate
   * @param p:          The predicate to apply
   * @return:           A `TakeWhile` iterator
   */
  template<typename Predicate>
  TakeWhile<IteratorType, Predicate> take_while(Predicate p) {
    auto *it = static_cast<IteratorType *>(this);
    return TakeWhile<IteratorType, Predicate>(*it, p);
  }

  /**
   * Summary:
   *    Creates a `Cycle` iterator
   *
   * @return: A `Cycle` iterator
   */
  Cycle<IteratorType> cycle() {
    auto *it = static_cast<IteratorType *>(this);
    return Cycle<IteratorType>(*it);
  }

  /**
   * Summary:
   *    Creates an `Interleave` iterator given a second iterator
   *
   * @tparam SecondIterator: The type of the second iterator
   * @param other:           The second iterator
   * @return:                An `Interleave` iterator
   */
  template<typename SecondIterator>
  Interleave<IteratorType, SecondIterator> interleave(SecondIterator other) {
    auto *it = static_cast<IteratorType *>(this);
    return Interleave<IteratorType, SecondIterator>(*it, other);
  }

  /**
   * Summary:
   *    Create an `InterleaveShortest` itearator given a second iterator
   *
   * @tparam SecondIterator: The type of the second iterator
   * @param other:           The second iterator
   * @return:                An `InterleaveShortest` iterator
   */
  template<typename SecondIterator>
  InterleaveShortest<IteratorType, SecondIterator> interleave_shortest(SecondIterator other) {
    auto *it = static_cast<IteratorType *>(this);
    return InterleaveShortest<IteratorType, SecondIterator>(*it, other);
  }

  /**
   * Summary:
   *    Creates a `Unique` iterator
   *
   * @return: A `Unique` iterator
   */
  Unique<IteratorType> unique() {
    auto *it = static_cast<IteratorType *>(this);
    return Unique<IteratorType>(*it);
  }

  /**
   * Summary:
   *    Creates a `UniqueBy` iterator given a uniqueness function
   *
   * @tparam F:   The type of the uniqueness function
   * @param func: The uniqueness function
   * @return:     A `UniqueBy` iterator
   */
  template<typename F>
  UniqueBy<IteratorType, F> unique_by(F func) {
    auto *it = static_cast<IteratorType *>(this);
    return UniqueBy<IteratorType, F>(*it, func);
  }

  using UnwrapedItemType = internal::unwrap_ref_wrapper_t<ItemType>;

  /**
   * Summary:
   *    Consumes the iterator an checks all the items against the
   *    given predicate. If for all items the predicate returns true,
   *    then this function returns also true.
   *
   * @tparam Predicate: The type of the predicate
   * @param p:          The predicate to test
   * @return:           true if for all items the predicate returns true, false otherwise
   *
   * @example:
   * ```
   * Array<int> ints(5);
   * ints[0] = 1;
   * ints[1] = 10;
   * ints[2] = 13;
   * ints[3] = 100;
   * ints[4] = -1;
   *
   * bool all_positive = ints.iter().all([](const int &v) { return v > 0; });
   *
   * // all_positive is false
   * ```
   */
  template<typename Predicate>
  bool all(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, UnwrapedItemType);
    auto *iter = static_cast<IteratorType *>(this);
    for (UnwrapedItemType v : *iter) {
      if (!p(v)) {
        return false;
      }
    }

    return true;
  }

  /**
   * Summary:
   *    Consumes the iterator and checks if any of the elements match
   *    the given predicate. `any` is short-circuiting, so as soon as
   *    predicate returns true, iteration stops.
   *
   * @tparam Predicate: The type of the predicate
   * @param p:          The predicate to test
   * @return:           true if any of the items match the predicate, false otherwise
   *
   * @example:
   * ```
   * Array<const char*> strings(4);
   * strings[0] = "a";
   * strings[1] = "aa";
   * strings[2] = "bb";
   * strings[3] = "abcde";
   *
   * bool res = strings.iter().any([](const char *s) { return strlen(s) == 5; });
   *
   * // res is true
   * ```
   */
  template<typename Predicate>
  bool any(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, UnwrapedItemType);

    auto *iter = static_cast<IteratorType *>(this);
    for (UnwrapedItemType v : *iter) {
      if (p(v)) {
        return true;
      }
    }

    return false;
  }

  /**
   * Summary:
   *    Consumes the iterator and checks if none if of the elements
   *    match the given predicate. `none` is short-circuiting, so as soon as
   *    predicate returns true, iteration stops.
   *
   * @tparam Predicate: The type of the predicate
   * @param p:          The predicate to test
   * @return:           true if none of the items match the predicate, false otherwise
   *
   * @example:
   * ```
   * Array<int> ints(5);
   * ints[0] = 4;
   * ints[1] = 6;
   * ints[2] = 8;
   * ints[3] = 10;
   * ints[4] = 9;
   *
   * auto is_prime = [](const int &v) {
   *    if (v < 2) return false;
   *    if (v == 2) return true;
   *    if (!(v & 1)) return false;
   *    if (v % 3 == 0) return v == 3;
   *    int i,j;
   *    for (i = 5; (j = i*i), j <= v && j > i; i += 6) {
   *        if (v % i == 0) return false;
   *        if (v % (i + 1) == 0) return false;
   *    }
   *    return true;
   * };
   *
   * bool none_prime = ints.iter().none(is_prime);
   * // none_prime is true
   * ```
   */
  template<typename Predicate>
  bool none(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, UnwrapedItemType);

    auto *iter = static_cast<IteratorType *>(this);
    for (UnwrapedItemType v : *iter) {
      if (p(v)) {
        return false;
      }
    }

    return true;
  }

  /**
   * Summary:
   *    Consumes the iterator and tests each item against the given
   *    predicate. If the predicate returns true, then the item is
   *    found and returned.
   *
   * @tparam Predicate: The type of the predicate
   * @param p:          The predicate to test
   * @return:           The first item that matches the predicate, std::nullopt otherwise
   *
   * @example:
   * ```
   * Array<int> ints(5);
   * for (size_t i = 0U; i != ints.len(); ++i) {
   *    ints[i] = (int) i + 1;
   * }
   *
   * // ints is: [1, 2, 3, 4, 5]
   *
   * auto v = ints.iter().find([](const int &v) { return v == 4; });
   *
   * assert(v.has_value());
   * // v.value() is 4
   * ```
   */
  template<typename Predicate>
  std::optional<ItemType> find(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, UnwrapedItemType);

    auto *iter = static_cast<IteratorType *>(this);
    for (UnwrapedItemType v : *iter) {
      if (p(v)) {
        return v;
      }
    }

    return std::nullopt;
  }

  /**
   * Summary:
   *    Consumes the iterator and returns the maximum element.
   *    The maximum property is determined by the `Compare` function
   *    provided.
   *
   * @tparam Compare: The type of the compare function
   * @param cmp:      The compare function
   * @return:         The maximum element if the iterator yielded any items, std::nullopt otherwise
   *
   * @example:
   * ```
   * Array<const char*> strings(3);
   * strings[0] = "aaa";
   * strings[1] = "cccc";
   * strings[2] = "bb";
   *
   * auto str = strings.iter()
   *    .max_by([](const char *lhs, const char *rhs) {
   *        return strlen(lhs) > strlen(rhs);
   *    });
   *
   * assert(str.has_value());
   * // str.value is "cccc"
   * ```
   */
  template<typename Compare>
  std::optional<ItemType> max_by(Compare cmp) {
    auto *iter = static_cast<IteratorType *>(this);
    auto next = iter->next();
    if (next.has_value()) {
      UnwrapedItemType max = *next;
      for (UnwrapedItemType v : *iter) {
        max = cmp(v, max);
      }

      if constexpr (internal::is_ref_wrapper_v<ItemType>) {
        return std::make_optional(std::ref(max));
      } else {
        return std::make_optional(max);
      }
    } else {
      return std::nullopt;
    }
  }

  /**
   * Summary:
   *    Consumes the iterator and returns the maximum item.
   *    The maximum property is determined by the greater (>) operator.
   *
   * @return: The maximum item from the iterator if it yielded any items, std::nullopt otherwise
   */
  std::optional<ItemType> max() {
    auto *iter = static_cast<IteratorType *>(this);
    auto max = iter->next();
    for (UnwrapedItemType v : *iter) {
      if constexpr (internal::is_ref_wrapper_v<ItemType>) {
        if (v > (*max).get()) {
          max = v;
        }
      } else {
        if (v > *max) {
          max = v;
        }
      }
    }
    return max;
  }

  /**
   * Summary:
   *    Consumes the iterator and returns the minimum element.
   *    The minimum property is determined by the `Compare` function
   *    provided.
   *
   * @tparam Compare: The type of the compare function
   * @param cmp:      The comparison function
   * @return:         The minimum element if the iterator yielded any items, std::nullopt otherwise
   */
  template<typename Compare>
  std::optional<ItemType> min_by(Compare cmp) {
    auto *iter = static_cast<IteratorType *>(this);
    auto next = iter->next();
    if (next.has_value()) {
      UnwrapedItemType min = *next;
      for (UnwrapedItemType v : *iter) {
        min = cmp(v, min);
      }
      if constexpr (internal::is_ref_wrapper_v<ItemType>) {
        return std::make_optional(std::ref(min));
      } else {
        return std::make_optional(min);
      }
    } else {
      return std::nullopt;
    }
  }

  /**
   * Summary:
   *    Consumes the iterator and returns the minimum item.
   *    The maximum property is determined by the less (<) operator.
   *
   * @return: The minimum item from the iterator if it yielded any items, std::nullopt otherwise
   */
  std::optional<ItemType> min() {
    auto *iter = static_cast<IteratorType *>(this);
    auto min = iter->next();
    for (UnwrapedItemType v : *iter) {
      if constexpr (internal::is_ref_wrapper_v<ItemType>) {
        if (v < (*min).get()) {
          min = v;
        }
      } else {
        if (v < *min) {
          min = v;
        }
      }
    }
    return min;
  }

  /**
   * Summary:
   *    Consumes the iterator and for each item yielded, applies
   *    the given function.
   *
   * @tparam F:   The type of the function to apply
   * @param func: The function to apply
   *
   * @example:
   * ```
   * Array<int> ints(2);
   * ints[0] = 0;
   * ints[1] = 1;
   *
   * // Prints 1 and 2
   * ints.iter().foreach([](const int &v) { printf("%d\n", v); });
   * ```
   */
  template<typename F>
  void for_each(F func) {
    auto iter = *static_cast<IteratorType *>(this);
    for (UnwrapedItemType v : iter) {
      func(v);
    }
  }

  using StrippedItemType = internal::strip_ref_wrapper_t<ItemType>;

  /**
   * Summary:
   *    Consumes the iterator and returns the sum of the items.
   *    The sum is computed using the addition operator (+) of
   *    the item type
   *
   * @return: The sum of all the iterator items
   *
   * @exmaple:
   * ```
   * Array<int> ints(3);
   * ints[0] = 1;
   * ints[1] = 2;
   * ints[3] = 3;
   *
   * int sum = ints.iter().sum();
   *
   * // sum is 6
   * ```
   */
  StrippedItemType sum() {
    auto *iter = static_cast<IteratorType *>(this);
    StrippedItemType res{};
    for (UnwrapedItemType v : *iter) {
      res = res + v;
    }
    return res;
  }

  /**
   * Summary:
   *    Consumes the iterator and folds (accumulates or reduces) the
   *    items yielded using a provided function.
   *    It takes an initial state that it will fold on and the function
   *    takes as input the running folded value and an iterator element.
   *
   * @tparam F:   The type of the function that performs the fold
   * @param init: The initial value to perform the fold on
   * @param func: The function that performs the fold
   * @return:     A single value produced by folding all the values
   *
   * @example:
   * ```
   * Array<int> ints(3);
   * ints[0] = 1;
   * ints[1] = 2;
   * ints[3] = 3;
   *
   * int product = ints.iter()
   *    .fold(1, [](const int &acc, const int &v) {
   *        return acc * v;
   *    });
   *
   * // product is 6 (1 * 2 * 3)
   * ```
   */
  template<typename F>
  StrippedItemType fold(StrippedItemType init, F func) {
    static_assert(internal::returns_type_v<StrippedItemType, F, StrippedItemType &, UnwrapedItemType>,
                  "Function must take an ItemType and an accumulator and must return the newly accumulated value");

    auto *iter = static_cast<IteratorType *>(this);
    StrippedItemType res = init;
    for (UnwrapedItemType v : *iter) {
      res = func(res, v);
    }
    return res;
  }

  /**
   * Summary:
   *    Consumes the iterator and joins each item using the separator
   *    provided. The items are converted to strings using `std::to_string`
   *    method, so if you have a custom type, implement `std::to_string` for
   *    your type
   *
   * @param sep: The separator to use in order to join the items
   * @return: A string which contains the representation of each item joined
   *          using the provided separator
   *
   * @example:
   * ```
   * Array<int> ints(4);
   * ints[0] = 1;
   * ints[1] = 2;
   * ints[2] = 3;
   * ints[3] = 4;
   *
   * std::string joined = ints.iter().join(", ");
   *
   * // joined is: "1, 2, 3, 4"
   * ```
   */
  std::string join(const std::string_view &sep) {
    std::string res{};
    auto *iter = static_cast<IteratorType *>(this);
    for (UnwrapedItemType v : *iter) {
      res += std::to_string(v) + sep.data();
    }
    res = std::move(res.substr(0, res.length() - sep.length()));
    return res;
  }

  /**
   * Summary:
   *    Consumes the iterator and returns the number of
   *    items it has.
   *
   * @return: The number of remaining items an iterator has
   */
  size_t count() {
    auto *iter = static_cast<IteratorType *>(this);
    size_t count = 0U;
    for (UnwrapedItemType _ : *iter) {
      ++count;
    }
    return count;
  }

  /**
   * Summary:
   *    Consumes the iterator and collects it to a custom
   *    type. In order to collect an iterator to an object,
   *    you must implement a static function for your type
   *    named `from_iterator` which accepts a generic iterator
   *    type.
   *
   *    e.g:
   *
   *    struct MyType {
   *        template<typename IteratorType>
   *        static MyType from_iterator(IteratorType it) { ... }
   *    };
   *
   *    When calling collect you only have to provide the type of
   *    your collection and not the type that the collection accepts.
   *    See @example below.
   *
   * @tparam Collection: The type of the collection to collect to
   * @return: A collection created by consuming the iterator
   *
   * @example:
   * ```
   * Array<int> ints(3);
   * ints[0] = 1;
   * ints[1] = 2;
   * ints[2] = 3;
   *
   * auto squared = ints.iter()
   *    .map([](const int &v) { return v * v; })
   *    .collect<Array>(); // Here we don't have to specify the type of the Array that we are collecting to.
   *                       // Just the fact that we want an Array
   *
   * // squared is: [1, 4, 9]
   * ```
   */
  template<template<typename> typename Collection>
  Collection<ItemType> collect() {
    auto *it = static_cast<IteratorType *>(this);
    return Collection<ItemType>::from_iterator(*it);
  }

  /**
   * Summary:
   *    Creates a clone of the current iterator state.
   *
   * @return: Return an iterator clone
   */
  [[nodiscard]]
  IteratorType clone() const {
    IteratorType res = iter();
    return res;
  }

  /**
   * Summary:
   *    Helper function that returns an iterator
   *    out of an iterator. It just returns itself
   *
   * @return: The iterator itself
   */
  [[nodiscard]]
  IteratorType iter() const {
    return *static_cast<const IteratorType *>(this);
  }

  /**
   * Summary:
   *    This function is just for `range for` syntax
   *    interoperability. You shouldn't use this function
   *    directly
   *
   * @return: Return the iterator itself and advance it as well.
   */
  IteratorType begin() {
    auto *it = static_cast<IteratorType *>(this);
    yielded = it->next();
    return *it;
  }

  /**
   * Summary:
   *    This function is just for `range for` syntax
   *    interoperability. Actually, is just for terminating
   *    the loop. You shouldn't use this function directly.
   *
   * @return: The iterator itself with empty state
   */
  IteratorType end() {
    auto *it = static_cast<const IteratorType *>(this);
    yielded = std::nullopt;
    return *it;
  }

  /**
   * Summary:
   *    This function is just for `range for` syntax
   *    interoperability. It  advances the iterator and stores
   *    the result. You shouldn't use this function directly
   *
   * @return: The iterator itself while advancing it first
   */
  IteratorType &operator++() {
    auto *it = static_cast<IteratorType *>(this);
    this->yielded = it->next();
    return *it;
  }

  /**
   * Summary:
   *    This function is just for `range for` syntax
   *    interoperability. It retrieves the current yielded item.
   *    You shouldn't use this function directly.
   *
   * @return: The item that was yielded last
   */
  UnwrapedItemType operator*() {
    return *this->yielded;
  }

  /**
   * Summary:
   *    This function is just for `range for` syntax
   *    interoperability. It compares the current state of the
   *    iterator with the empty state. You shouldn't use this
   *    function directly.
   *
   * @param _rhs: The iterator we comparing against. Unused
   * @return:     true if the iterator has finished, false otherwise
   */
  bool operator!=(const IteratorType &_rhs) {
    return this->yielded != std::nullopt;
  }

  std::optional<ItemType> yielded{};
};

#endif //ITERATOR__ITERATOR_H
