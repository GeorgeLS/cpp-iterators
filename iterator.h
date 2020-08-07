#ifndef ITERATOR__ITERATOR_H
#define ITERATOR__ITERATOR_H

#include <cstddef>
#include <functional>

#define foreach(it, iterable)                        \
    auto __##it##__it = (iterable).iter();           \
    auto it = __##it##__it.next();                   \
    for (; it.has_value(); it = __##it##__it.next()) \

#define MUST_RETURN_BOOL "The return value of the predicate must be bool"
#define ASSERT_RETURNS_BOOL(p, a) \
  static_assert(returns_bool_v<p, a>, MUST_RETURN_BOOL)

template<typename R, typename F, typename... Args>
inline constexpr bool returns_type_v = std::is_same_v<std::result_of_t<F(Args...)>, R>;

template<typename IteratorType>
using item_type = typename IteratorType::ItemType;

template<typename T>
struct strip_possible_ref_wrapper {
  using type = T;
};

template<typename T>
struct strip_possible_ref_wrapper<std::reference_wrapper<T>> {
  using type = T;
};

template<typename T>
using strip_possible_ref_wrapper_t = typename strip_possible_ref_wrapper<T>::type;

template<typename F, typename ... Args>
inline constexpr bool returns_bool_v = returns_type_v<bool, F, Args...>;

template<typename ItemType, typename IteratorType> struct Iterator;

template<typename IteratorType>
struct StepBy : public Iterator<item_type<IteratorType>, StepBy<IteratorType>> {
  using ItemType = item_type<IteratorType>;

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

template<typename IteratorType, typename MapF>
struct Map : public Iterator<std::result_of_t<MapF(item_type<IteratorType>)>, Map<IteratorType, MapF>> {
  using ItemType = std::result_of_t<MapF(item_type<IteratorType>)>;

  explicit Map(IteratorType it, MapF mapper) : inner{it}, mapper{mapper} {}

  std::optional<ItemType> next() {
    auto v = inner.next();
    if (v.has_value()) {
      return std::make_optional(mapper(v.value()));
    } else {
      return std::nullopt;
    }
  }

  IteratorType inner;
  MapF mapper;
};

template<typename IteratorType>
struct Skip : public Iterator<item_type<IteratorType>, Skip<IteratorType>> {
  using ItemType = item_type<IteratorType>;

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

template<typename IteratorType, typename Predicate>
struct SkipWhile : public Iterator<item_type<IteratorType>, SkipWhile<IteratorType, Predicate>> {
  ASSERT_RETURNS_BOOL(Predicate, item_type<IteratorType>);

  using ItemType = item_type<IteratorType>;

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

template<typename IteratorType>
struct Enumerate : public Iterator<std::pair<size_t, item_type<IteratorType>>, Enumerate<IteratorType>> {
  using ItemType = std::pair<size_t, item_type<IteratorType>>;

  explicit Enumerate(IteratorType it) : inner{it}, index{0U} {}

  std::optional<ItemType> next() {
    auto v = inner.next();

    if (v.has_value()) {
      ++index;
      return std::make_optional(std::make_pair(index - 1, v.value()));
    } else {
      return std::nullopt;
    }
  }

  IteratorType inner;
  size_t index;
};

template<typename IteratorType, typename Predicate>
struct Filter : public Iterator<item_type<IteratorType>, Filter<IteratorType, Predicate>> {
  ASSERT_RETURNS_BOOL(Predicate, item_type<IteratorType>);

  using ItemType = item_type<IteratorType>;

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

template<typename FirstIterator, typename SecondIterator>
struct Chain : public Iterator<item_type<FirstIterator>, Chain<FirstIterator, SecondIterator>> {
  static_assert(std::is_same_v<item_type<FirstIterator>, item_type<SecondIterator>>,
                "The item types of the first and second iterator must be the same");

  using ItemType = item_type<FirstIterator>;

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

template<typename FirstIterator, typename SecondIterator>
struct Zip : public Iterator<std::pair<item_type<FirstIterator>, item_type<SecondIterator>>,
                             Zip<FirstIterator, SecondIterator>> {

  using FirstItemType = item_type<FirstIterator>;
  using SecondItemType = item_type<SecondIterator>;
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

template<typename IteratorType>
struct Take : public Iterator<item_type<IteratorType>, Take<IteratorType>> {
  using ItemType = item_type<IteratorType>;

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

template<typename IteratorType, typename Predicate>
struct TakeWhile : public Iterator<item_type<IteratorType>, TakeWhile<IteratorType, Predicate>> {
  ASSERT_RETURNS_BOOL(Predicate, item_type<IteratorType>);

  using ItemType = item_type<IteratorType>;

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

template<typename IteratorType>
struct Cycle : public Iterator<item_type<IteratorType>, Cycle<IteratorType>> {
  using ItemType = item_type<IteratorType>;

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

template<typename ItemType, typename IteratorType>
struct Iterator {
  [[nodiscard]]
  StepBy<IteratorType> step_by(size_t step) {
    auto *it = static_cast<IteratorType *>(this);
    return StepBy<IteratorType>(*it, step);
  }

  template<typename MapF>
  Map<IteratorType, MapF> map(MapF mapper) {
    auto *it = static_cast<IteratorType *>(this);
    return Map<IteratorType, MapF>(*it, mapper);
  }

  Enumerate<IteratorType> enumerate() {
    auto *it = static_cast<IteratorType *>(this);
    return Enumerate<IteratorType>(*it);
  }

  Skip<IteratorType> skip(size_t num) {
    auto *it = static_cast<IteratorType *>(this);
    return Skip<IteratorType>(*it, num);
  }

  template<typename Predicate>
  SkipWhile<IteratorType, Predicate> skip_while(Predicate p) {
    auto *it = static_cast<IteratorType *>(this);
    return SkipWhile<IteratorType, Predicate>(*it, p);
  }

  template<typename Predicate>
  Filter<IteratorType, Predicate> filter(Predicate p) {
    auto *it = static_cast<IteratorType *>(this);
    return Filter<IteratorType, Predicate>(*it, p);
  }

  template<typename SecondIterator>
  Chain<IteratorType, SecondIterator> chain(SecondIterator other) {
    auto *it = static_cast<IteratorType *>(this);
    return Chain<IteratorType, SecondIterator>(*it, other);
  }

  template<typename SecondIterator>
  Zip<IteratorType, SecondIterator> zip(SecondIterator other) {
    auto *it = static_cast<IteratorType *>(this);
    return Zip<IteratorType, SecondIterator>(*it, other);
  }

  Take<IteratorType> take(size_t num) {
    auto *it = static_cast<IteratorType *>(this);
    return Take<IteratorType>(*it, num);
  }

  template<typename Predicate>
  TakeWhile<IteratorType, Predicate> take_while(Predicate p) {
    auto *it = static_cast<IteratorType *>(this);
    return TakeWhile<IteratorType, Predicate>(*it, p);
  }

  Cycle<IteratorType> cycle() {
    auto *it = static_cast<IteratorType *>(this);
    return Cycle<IteratorType>(*it);
  }

  template<typename Predicate>
  bool all(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, typename IteratorType::ItemType);
    auto *iter = static_cast<IteratorType *>(this);
    foreach(it, *iter) {
      if (!p(*it)) {
        return false;
      }
    }

    return true;
  }

  template<typename Predicate>
  bool any(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, typename IteratorType::ItemType);

    auto *iter = static_cast<IteratorType *>(this);
    foreach(it, *iter) {
      if (p(*it)) {
        return true;
      }
    }

    return false;
  }

  template<typename Predicate>
  bool none(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, typename IteratorType::ItemType);

    auto *iter = static_cast<IteratorType *>(this);
    foreach(it, *iter) {
      if (p(*it)) {
        return false;
      }
    }

    return true;
  }

  template<typename Predicate>
  std::optional<ItemType> find(Predicate p) {
    ASSERT_RETURNS_BOOL(Predicate, ItemType);

    auto *iter = static_cast<IteratorType *>(this);
    foreach(it, *iter) {
      if (p(*it)) {
        return it;
      }
    }

    return std::nullopt;
  }

  template<typename Compare>
  std::optional<ItemType> max_by(Compare cmp) {
    static_assert(returns_type_v<ItemType, Compare, ItemType, ItemType>,
                  "Compare function must accept two ItemTypes and return the maximum one");

    auto *iter = static_cast<IteratorType *>(this);
    auto max = iter->next();
    foreach(it, *iter) {
      max = cmp(*it, *max);
    }
    return max;
  }

  std::optional<ItemType> max() {
    auto *iter = static_cast<IteratorType *>(this);
    auto max = iter->next();
    foreach(it, *iter) {
      if (*it > *max) {
        max = it;
      }
    }
    return max;
  }

  template<typename Compare>
  std::optional<ItemType> min_by(Compare cmp) {
    static_assert(returns_type_v<ItemType, Compare, ItemType, ItemType>,
                  "Compare function must accept two ItemTypes and return the minimum one");
    auto *iter = static_cast<IteratorType *>(this);
    auto min = iter->next();
    foreach(it, *iter) {
      min = cmp(*it, *min);
    }
    return min;
  }

  std::optional<ItemType> min() {
    auto *iter = static_cast<IteratorType *>(this);
    auto min = iter->next();
    foreach(it, *iter) {
      if (*it < *min) {
        min = it;
      }
    }
    return min;
  }

  template<typename F>
  void for_each(F func) {
    auto iter = *static_cast<IteratorType *>(this);
    foreach(it, iter) {
      func(*it);
    }
  }

  using StrippedItemType = strip_possible_ref_wrapper_t<ItemType>;

  StrippedItemType sum() {
    auto *iter = static_cast<IteratorType *>(this);
    StrippedItemType res{};
    foreach(it, *iter) {
      res = res + *it;
    }
    return res;
  }

  template<typename F>
  StrippedItemType fold(StrippedItemType init, F func) {
    static_assert(returns_type_v<StrippedItemType, F, StrippedItemType, ItemType>,
                  "Function must take an ItemType and an accumulator and must return the newly accumulated value");
    auto *iter = static_cast<IteratorType *>(this);
    foreach(it, *iter) {
      init = func(init, *it);
    }
    return init;
  }

  [[nodiscard]]
  IteratorType iter() const {
    return *static_cast<const IteratorType *>(this);
  }
};

#endif //ITERATOR__ITERATOR_H
