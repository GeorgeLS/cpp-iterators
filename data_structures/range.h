#ifndef ITERATOR_DATA_STRUCTURES_RANGE_H
#define ITERATOR_DATA_STRUCTURES_RANGE_H

#include <cassert>
#include "../iterator.h"
#include <utility>

template<typename T>
struct Range {

  template<typename _T = T>
  Range(_T &&start, _T &&end) : start(std::forward<T>(start)), end(std::forward<T>(end)) {
    assert(start < end);
  }

  struct RangeIterator : public Iterator<T, RangeIterator> {
    using ItemType = T;

    explicit RangeIterator(const Range &range) : range(range), cursor(range.start) {}

    std::optional<ItemType> next() {
      if (cursor <= range.end) {
        auto res = std::make_optional(cursor);
        ++cursor;
        return res;
      }
      return std::nullopt;
    }

    const Range &range;
    T cursor;
  };

  RangeIterator iter() const {
    return RangeIterator(*this);
  }

  T start;
  T end;
};

// Class template argument deduction guide
template<typename T>
Range(T start, T end) -> Range<T>;

#endif //ITERATOR_DATA_STRUCTURES_RANGE_H
