#include "../unit_test.h"
#include "../data_structures/array.h"

template<typename T>
bool array_cmp_eq(const Array<T> &lhs, const Array<T> &rhs) {
  if (lhs.len() != rhs.len()) {
    return false;
  }

  for (size_t i = 0U; i != lhs.len(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}

UNIT_TEST(step_by_works) {
  Array<int> ints{10};

  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = (int) i + 1;
  }

  Array<int> expected{5};
  expected[0] = 1;
  expected[1] = 3;
  expected[2] = 5;
  expected[3] = 7;
  expected[4] = 9;

  auto iter = ints.iter().step_by(2);
  // We use from_iterator directly because collect is a separate test
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);

  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(map_works) {
  Array<int> ints{4};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i + 1;
  }

  Array<std::string> expected{4};
  expected[0] = "String_1";
  expected[1] = "String_2";
  expected[2] = "String_3";
  expected[3] = "String_4";

  auto iter = ints.iter().map([](const int &v) { return "String_" + std::to_string(v); });
  auto res = Array<std::string>::from_iterator(iter);
  bool is_expecetd = array_cmp_eq(res, expected);

  ASSERT(is_expecetd);
  TEST_PASSED();
}

UNIT_TEST(skip_works) {
  Array<int> ints{10};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i + 1;
  }

  Array<int> expected{3};
  expected[0] = 8;
  expected[1] = 9;
  expected[2] = 10;

  auto iter = ints.iter().skip(7);
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);

  ASSERT(is_expected);
  TEST_PASSED();
}

UNIT_TEST(skip_while_works) {
  Array<int> ints{5};
  ints[0] = 2;
  ints[1] = 4;
  ints[2] = 6;
  ints[3] = 7;
  ints[4] = 8;

  Array<int> expected{2};
  expected[0] = 7;
  expected[1] = 8;

  auto iter = ints.iter().skip_while([](const int &v) { return v % 2 == 0; });
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);

  ASSERT(is_expected);
  TEST_PASSED();
}

UNIT_TEST(enumerate_works) {
  Array<const char *> strings{4};
  strings[0] = "String_0";
  strings[1] = "String_1";
  strings[2] = "String_2";
  strings[3] = "String_3";

  size_t i = 0U;
  for (auto[index, str] : strings.iter().enumerate()) {
    std::string expected_str = "String_" + std::to_string(i);
    ASSERT(index == i);
    ASSERT(expected_str == str.get());
    ++i;
  }

  TEST_PASSED();
}

UNIT_TEST(filter_works) {
  Array<int> ints{10};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i + 1;
  }

  Array<int> expected{5};
  expected[0] = 2;
  expected[1] = 4;
  expected[2] = 6;
  expected[3] = 8;
  expected[4] = 10;

  auto iter = ints.iter().filter([](const int &v) { return v % 2 == 0; });
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);
  TEST_PASSED();
}

UNIT_TEST(chain_works) {
  Array<int> a1{5};

  for (size_t i = 0U; i != a1.len(); ++i) {
    a1[i] = i;
  }

  Array<int> a2{3};
  for (size_t i = 0U; i != a2.len(); ++i) {
    a2[i] = a2.len() - i;
  }

  Array<int> expected{13};

  for (size_t i = 0U; i != a1.len(); ++i) {
    expected[i] = i;
  }

  for (size_t i = 0U; i != a1.len(); ++i) {
    expected[a1.len() + i] = a2.len() - i;
  }

  for (size_t i = 0U; i != a1.len(); ++i) {
    expected[a1.len() + a2.len() + i] = i;
  }

  auto iter1 = a1.iter().chain(a2.iter()).chain(a1.iter());
  auto res1 = Array<int>::from_iterator(iter1);
  bool is_expected = array_cmp_eq(res1, expected);
  ASSERT(is_expected);

  auto iter2 = a1.iter().chain(a2.iter().chain(a1.iter()));
  auto res2 = Array<int>::from_iterator(iter2);
  is_expected = array_cmp_eq(res2, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(zip_works) {
  Array<size_t> indexes{3};
  indexes[0] = 0;
  indexes[1] = 1;
  indexes[2] = 2;

  Array<const char *> strings{3};
  strings[0] = "String_0";
  strings[1] = "String_1";
  strings[2] = "String_2";

  // Here we also map because Array iterator
  // yields items by reference which is achieved
  // by using std::reference_wrapper
  // If we didn't map, then the result type would be
  // std::pair<std::reference_wrapper<size_t>, std::reference_wrapper<const char *>>
  // which makes our life more difficult in terms of constructing the expected result.
  // Thus, we map to the actual values by dereferencing then in the map function
  auto iter = indexes.iter()
      .zip(strings.iter())
      .map([](const std::pair<size_t, const char *> &v) {
        return std::make_pair(v.first, v.second);
      });

  using ZipType = decltype(iter)::ItemType;

  Array<ZipType> expected{3};
  expected[0] = std::make_pair(0, "String_0");
  expected[1] = std::make_pair(1, "String_1");
  expected[2] = std::make_pair(2, "String_2");

  auto res = Array<ZipType>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(take_works) {
  Array<int> ints{5};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i + 1;
  }

  Array<int> expected{2};
  expected[0] = 1;
  expected[1] = 2;

  auto iter = ints.iter().take(2);
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(take_while_works) {
  Array<int> ints{5};
  ints[0] = 2;
  ints[1] = 4;
  ints[2] = 6;
  ints[3] = 7;
  ints[4] = 8;

  Array<int> expected{3};
  expected[0] = 2;
  expected[1] = 4;
  expected[2] = 6;

  auto iter = ints.iter().take_while([](const int &v) { return v % 2 == 0; });
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(cycle_works) {
  Array<int> ints{3};
  ints[0] = 1;
  ints[1] = 2;
  ints[2] = 3;

  Array<int> expected{5};
  expected[0] = 1;
  expected[1] = 2;
  expected[2] = 3;
  expected[3] = 1;
  expected[4] = 2;

  // We are using take, otherwise this test would
  // run forever
  auto iter = ints.iter().cycle().take(5);
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(interleave_works) {
  Array<int> a1{4};
  a1[0] = 1;
  a1[1] = 2;
  a1[2] = 3;
  a1[3] = 4;

  Array<int> a2{2};
  a2[0] = -1;
  a2[1] = -2;

  Array<int> expected{6};
  expected[0] = 1;
  expected[1] = -1;
  expected[2] = 2;
  expected[3] = -2;
  expected[4] = 3;
  expected[5] = 4;

  auto iter = a1.iter().interleave(a2.iter());
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(interleave_shortest_works) {
  Array<int> a1{4};
  a1[0] = 1;
  a1[1] = 2;
  a1[2] = 3;
  a1[3] = 4;

  Array<int> a2{2};
  a2[0] = -1;
  a2[1] = -2;

  Array<int> expected{5};
  expected[0] = 1;
  expected[1] = -1;
  expected[2] = 2;
  expected[3] = -2;
  expected[4] = 3;

  auto iter = a1.iter().interleave_shortest(a2.iter());
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(unique_wokrs) {
  Array<int> ints{6};
  ints[0] = 1;
  ints[1] = 1;
  ints[2] = 1;
  ints[3] = 2;
  ints[4] = -3;
  ints[5] = 1;

  Array<int> expected{3};
  expected[0] = 1;
  expected[1] = 2;
  expected[2] = -3;

  auto iter = ints.iter().unique();
  auto res = Array<int>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(unique_by_works) {
  Array<std::string> strings{4};
  strings[0] = "aa";
  strings[1] = "bb";
  strings[2] = "a";
  strings[3] = "ccc";

  Array<std::string> expected{3};
  expected[0] = "aa";
  expected[1] = "a";
  expected[2] = "ccc";

  auto iter = strings.iter().unique_by([](const std::string &s) { return s.length(); });
  auto res = Array<std::string>::from_iterator(iter);
  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

UNIT_TEST(all_works) {
  Array<int> ints{5};
  ints[0] = 2;
  ints[1] = 4;
  ints[2] = 6;
  ints[3] = 8;
  ints[4] = 10;

  bool all_are_even = ints.iter().all([](const int &v) { return v % 2 == 0; });
  ASSERT(all_are_even);

  TEST_PASSED();
}

UNIT_TEST(any_works) {
  Array<int> ints{5};
  ints[0] = 1;
  ints[1] = 3;
  ints[2] = 5;
  ints[3] = 2;
  ints[4] = 7;

  bool any_even = ints.iter().any([](const int &v) { return v % 2 == 0; });
  ASSERT(any_even);

  TEST_PASSED();
}

UNIT_TEST(none_works) {
  Array<int> ints{5};
  ints[0] = 1;
  ints[1] = 3;
  ints[2] = 5;
  ints[3] = 7;
  ints[4] = 9;

  bool none_even = ints.iter().none([](const int &v) { return v % 2 == 0; });
  ASSERT(none_even);

  TEST_PASSED();
}

UNIT_TEST(find_works) {
  Array<int> ints{10};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i + 1;
  }

  auto five = ints.iter().find([](const int &v) { return v == 5; });
  ASSERT(five.has_value());
  ASSERT(*five == 5);

  auto eleven = ints.iter().find([](const int &v) { return v == 11; });
  ASSERT(!eleven.has_value());

  TEST_PASSED();
}

UNIT_TEST(max_by_works) {
  Array<std::string> strings{5};
  strings[0] = "aa";
  strings[1] = "b";
  strings[2] = "ccc";
  strings[3] = "d";
  strings[4] = "a";

  auto max_by_len = strings.iter()
      .max_by([](const std::string &lhs, const std::string &rhs) -> const std::string & {
        return lhs.length() > rhs.length() ? lhs : rhs;
      });

  ASSERT(max_by_len.has_value());
  ASSERT((*max_by_len).get() == "ccc");

  TEST_PASSED();
}

UNIT_TEST(max_works) {
  Array<std::string> strings{3};
  strings[0] = "aaa";
  strings[1] = "aa";
  strings[2] = "b";

  auto max = strings.iter().max();
  ASSERT(max.has_value());
  ASSERT((*max).get() == "b");

  TEST_PASSED();
}

UNIT_TEST(min_by_works) {
  Array<std::string> strings{5};
  strings[0] = "aa";
  strings[1] = "b";
  strings[2] = "ccc";
  strings[3] = "da";
  strings[4] = "ab";

  auto min_by_len = strings.iter()
      .min_by([](const std::string &lhs, const std::string &rhs) -> const std::string & {
        return lhs.length() < rhs.length() ? lhs : rhs;
      });

  ASSERT(min_by_len.has_value());
  ASSERT((*min_by_len).get() == "b");

  TEST_PASSED();
}

UNIT_TEST(min_works) {
  Array<std::string> strings{3};
  strings[0] = "aaa";
  strings[1] = "aa";
  strings[2] = "b";

  auto min = strings.iter().min();
  ASSERT(min.has_value());
  ASSERT((*min).get() == "aa");

  TEST_PASSED();
}

UNIT_TEST(for_each_works) {
  Array<int> ints{10};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i;
  }

  size_t i = 0U;
  ints.iter()
      .for_each([&i](const int &v) {
        ASSERT(i == v);
        ++i;
      });

  TEST_PASSED();
}

UNIT_TEST(sum_works) {
  Array<int> ints{3};
  ints[0] = 2;
  ints[1] = 5;
  ints[2] = 10;

  int sum = ints.iter().sum();
  ASSERT(sum == 17);

  TEST_PASSED();
}

UNIT_TEST(fold_works) {
  Array<int> ints{3};
  ints[0] = 1;
  ints[1] = 3;
  ints[2] = 10;

  int product = ints.iter().fold(1, [](const int &acc, const int &v) {
    return acc * v;
  });

  ASSERT(product == 30);
  TEST_PASSED();
}

UNIT_TEST(join_works) {
  Array<int> ints{5};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i + 1;
  }

  std::string joined = ints.iter().join(", ");
  ASSERT(joined == "1, 2, 3, 4, 5");

  TEST_PASSED();
}

UNIT_TEST(count_works) {
  Array<int> ints{5};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i;
  }

  auto iter = ints.iter();
  iter.next();
  iter.next();

  size_t count = iter.count();

  ASSERT(count == 3);

  TEST_PASSED();
}

UNIT_TEST(collect_works) {
  Array<int> ints{10};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i + 1;
  }

  Array<std::string> expected{3};
  expected[0] = "Mapped_Value_3";
  expected[1] = "Mapped_Value_5";
  expected[2] = "Mapped_Value_7";

  auto res = ints.iter()
      .filter([](const int &v) { return v % 2 == 0; })
      .map([](const int &v) { return v + 1; })
      .filter([](const int &v) { return v <= 7; })
      .map([](const int &v) { return "Mapped_Value_" + std::to_string(v); })
      .collect<Array>();

  bool is_expected = array_cmp_eq(res, expected);
  ASSERT(is_expected);

  TEST_PASSED();
}

TestFn tests[] = {
    test_step_by_works,
    test_map_works,
    test_skip_works,
    test_skip_while_works,
    test_enumerate_works,
    test_filter_works,
    test_chain_works,
    test_zip_works,
    test_take_works,
    test_take_while_works,
    test_cycle_works,
    test_interleave_works,
    test_interleave_shortest_works,
    test_unique_wokrs,
    test_unique_by_works,
    test_all_works,
    test_any_works,
    test_none_works,
    test_find_works,
    test_max_by_works,
    test_max_works,
    test_min_by_works,
    test_min_works,
    test_for_each_works,
    test_sum_works,
    test_fold_works,
    test_join_works,
    test_count_works,
    test_collect_works
};

int main() {
  constexpr size_t num_tests = sizeof(tests) / sizeof(tests[0]);
  run_tests(tests, num_tests);
}