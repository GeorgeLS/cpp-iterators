#ifndef ARRAY_H
#define ARRAY_H

#include <cstdio>
#include "../iterator.h"

template<typename T> struct Array {
  Array() : data{nullptr}, num_elements{0U} {}

  explicit Array(size_t size) : data{new T[size]}, num_elements{size} {}

  template<typename IteratorType>
  static Array<T> from_iterator(IteratorType iter) {
    size_t count = iter.clone().count();
    auto arr = Array<T>(count);
    foreach(it, iter.enumerate()) {
      auto[index, value] = *it;
      arr[index] = value;
    }
    return arr;
  }

  Array(const Array<T> &rhs)
      : data{new T[rhs.num_elements]}, num_elements{rhs.num_elements} {
    for (size_t i = 0U; i != rhs.num_elements; ++i) {
      this->data[i] = rhs.data[i];
    }
  }

  Array(Array<T> &&rhs) noexcept
      : data{rhs.data}, num_elements{rhs.num_elements} {
    rhs.data = nullptr;
    rhs.num_elements = 0U;
  }

  ~Array() { delete[] this->data; }

  [[nodiscard]] size_t len() const noexcept { return this->num_elements; }

  T &operator[](size_t index) { return this->data[index]; }

  struct ArrayIterator : public Iterator<std::reference_wrapper<T>, ArrayIterator> {
    using ItemType = std::reference_wrapper<T>;

    explicit ArrayIterator(const Array<T> &cont) : cont{cont}, cursor{0U} {}

    std::optional<ItemType> next() {
      if (cursor < cont.get().num_elements) {
        ++cursor;
        return std::make_optional(std::ref(cont.get().data[cursor - 1]));
      } else {
        return std::nullopt;
      }
    }

    std::reference_wrapper<const Array<T>> cont;
    size_t cursor;
  };

  [[nodiscard]] ArrayIterator iter() const noexcept {
    return ArrayIterator(*static_cast<const Array<T> *>(this));
  }

private:
  T *data;
  size_t num_elements;
};

#endif