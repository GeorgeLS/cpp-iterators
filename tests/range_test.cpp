#include "../unit_test.h"
#include "../data_structures/range.h"
#include "../data_structures/array.h"

UNIT_TEST(range_iteration_works) {
  Range<size_t> range{1, 10};
  size_t i = 1U;
  foreach(it, range.iter()) {
    ASSERT(i == *it);
    ++i;
  }
  TEST_PASSED();
}

UNIT_TEST(range_map_works) {
  Range<int> range{1, 10};

  size_t i = 1U;
  foreach(it, range.iter().map([](const int &v) { return v * v; })) {
    ASSERT(i * i == *it);
    ++i;
  }

  TEST_PASSED();
}

UNIT_TEST(range_collect_works) {
  auto array = Range{1, 10}
      .iter()
      .collect<Array>();

  int i = 1;
  array.iter().for_each([&i](const int &v) {
    ASSERT(i == v);
    ++i;
  });

  TEST_PASSED();
}

TestFn tests[] = {
    test_range_iteration_works,
    test_range_map_works,
    test_range_collect_works
};

int main() {
  constexpr size_t num_tests = sizeof(tests) / sizeof(tests[0]);
  run_tests(tests, num_tests);
}