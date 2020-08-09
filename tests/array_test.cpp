#include "../unit_test.h"
#include "../data_structures/array.h"

UNIT_TEST(array_default_ctor_works) {
  Array<int> ints{};
  ASSERT(ints.len() == 0);
  TEST_PASSED();
}

UNIT_TEST(array_size_ctor_works) {
  Array<int> ints{10};
  ASSERT(ints.len() == 10);
  TEST_PASSED();
}

UNIT_TEST(array_copy_ctor_works) {
  Array<int> ints{10};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i;
  }

  Array<int> ints_copy = ints;
  ASSERT(ints.len() == 10);
  ASSERT(ints_copy.len() == 10);

  for (size_t i = 0U; i != ints.len(); ++i) {
    ASSERT(ints[i] == ints_copy[i]);
  }

  TEST_PASSED();
}

UNIT_TEST(array_move_ctor_works) {
  Array<int> ints{10};
  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i;
  }

  Array<int> ints_moved = std::move(ints);
  ASSERT(ints.len() == 0);
  ASSERT(ints_moved.len() == 10);

  for (size_t i = 0U; i != ints_moved.len(); ++i) {
    ASSERT(ints_moved[i] == i);
  }

  TEST_PASSED();
}

UNIT_TEST(array_copy_assignment_works) {
  Array<int> ints{10};

  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i;
  }

  Array<int> ints_copy{};

  ints_copy = ints;

  ASSERT(ints.len() == 10);
  ASSERT(ints_copy.len() == 10);

  for (size_t i = 0U; i != ints.len(); ++i) {
    ASSERT(ints[i] == ints_copy[i]);
  }

  TEST_PASSED();
}

UNIT_TEST(array_move_assignment_works) {
  Array<int> ints{10};

  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i;
  }

  Array<int> ints_moved{};

  ints_moved = std::move(ints);

  ASSERT(ints.len() == 0);
  ASSERT(ints_moved.len() == 10);

  for (size_t i = 0U; i != ints_moved.len(); ++i) {
    ASSERT(ints_moved[i] == i);
  }

  TEST_PASSED();
}

UNIT_TEST(array_reserve_works) {
  Array<int> ints{};

  ASSERT(ints.len() == 0);

  ints.reserve(10);
  ASSERT(ints.len() == 10);

  for (size_t i = 0U; i != ints.len(); ++i) {
    ints[i] = i;
    ASSERT(ints[i] == i);
  }

  TEST_PASSED();
}

TestFn tests[] = {
    test_array_default_ctor_works,
    test_array_size_ctor_works,
    test_array_copy_ctor_works,
    test_array_move_ctor_works,
    test_array_copy_assignment_works,
    test_array_move_assignment_works,
    test_array_reserve_works
};

int main() {
  constexpr size_t num_tests = sizeof(tests) / sizeof(tests[0]);
  run_tests(tests, num_tests);
}