#ifndef ARRAY_H
#define ARRAY_H

#include <cstring>
#include <cstdio>
#include "../iterator.h"

template<typename T> struct Array {
  Array() : data{nullptr}, num_elements{0U} {}

  explicit Array(size_t size) : data{new T[size]}, num_elements{size} {}

  Array(const Array<T> &rhs)
      : data{new T[rhs.num_elements]}, num_elements{rhs.num_elements} {
    copy(rhs);
  }

  Array(Array<T> &&rhs) noexcept {
    move(rhs);
  }

  ~Array() { delete[] this->data; }

  Array &operator=(const Array<T> &rhs) {

    if (this->data && this->num_elements < rhs.num_elements) {
      delete[] this->data;
    }

    this->data = new T[rhs.num_elements];
    this->num_elements = rhs.num_elements;

    copy(rhs);

    return *this;
  }

  Array &operator=(Array<T> &&rhs) noexcept {
    if (this->data) {
      delete[] this->data;
    }

    move(rhs);

    return *this;
  }

  template<typename IteratorType>
  static Array<T> from_iterator(IteratorType &iter) {
    size_t count = iter.clone().count();
    auto arr = Array<T>(count);
    foreach(it, iter.enumerate()) {
      auto[index, value] = *it;
      arr[index] = std::move(value);
    }
    return arr;
  }

  void reserve(size_t size) {
    if (this->data) {
      delete[] this->data;
    }

    this->data = new T[size];
    this->num_elements = size;
  }

  [[nodiscard]] size_t len() const noexcept { return this->num_elements; }

  T &operator[](size_t index) const { return this->data[index]; }

  struct ArrayIterator : public Iterator<std::reference_wrapper<T>, ArrayIterator> {
    using ItemType = std::reference_wrapper<T>;

    explicit ArrayIterator(const Array<T> &cont) : cont{cont}, cursor{0U} {}

    std::optional<ItemType> next() {
      if (this->cursor < this->cont.get().num_elements) {
        ++this->cursor;
        return std::make_optional(std::ref(this->cont.get().data[this->cursor - 1]));
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
  constexpr void copy(const Array<T> &rhs) {
    if constexpr (std::is_trivially_copy_assignable_v<T>) {
      memcpy(this->data, rhs.data, rhs.num_elements * sizeof(T));
    } else {
      for (size_t i = 0U; i != rhs.num_elements; ++i) {
        this->data[i] = rhs.data[i];
      }
    }
  }

  constexpr void move(Array<T> &rhs) noexcept {
    this->data = rhs.data;
    this->num_elements = rhs.num_elements;
    rhs.data = nullptr;
    rhs.num_elements = 0U;
  }

  T *data;
  size_t num_elements;
};

#endif