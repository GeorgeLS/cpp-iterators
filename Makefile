CC := g++
CFLAGS := -Wall
CFLAGS += -std=c++17
CFLAGS += -O3
LFLAGS := 

ODIR := .OBJ

TESTS_ARRAY_TEST_SOURCE_DEPS := tests/array_test.cpp unit_test.h data_structures/array.h iterator.h
TESTS_ITERATOR_TEST_SOURCE_DEPS := tests/iterator_test.cpp unit_test.h data_structures/array.h iterator.h

all: binaries

$(ODIR):
	@mkdir $(ODIR)

binaries: 

tests: tests_iterator_test tests_array_test 

TESTS_ITERATOR_TEST_OBJECT_DEPS := $(ODIR)/tests_iterator_test.o

tests_iterator_test: $(ODIR) $(TESTS_ITERATOR_TEST_OBJECT_DEPS)
	$(CC) $(CFLAGS) $(TESTS_ITERATOR_TEST_OBJECT_DEPS) -o tests/iterator_test

TESTS_ARRAY_TEST_OBJECT_DEPS := $(ODIR)/tests_array_test.o

tests_array_test: $(ODIR) $(TESTS_ARRAY_TEST_OBJECT_DEPS)
	$(CC) $(CFLAGS) $(TESTS_ARRAY_TEST_OBJECT_DEPS) -o tests/array_test

$(ODIR)/tests_array_test.o: $(ODIR) $(TESTS_ARRAY_TEST_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) tests/array_test.cpp -o $(ODIR)/tests_array_test.o

$(ODIR)/tests_iterator_test.o: $(ODIR) $(TESTS_ITERATOR_TEST_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) tests/iterator_test.cpp -o $(ODIR)/tests_iterator_test.o

.PHONY: clean
clean:
	rm -rf .OBJ tests/iterator_test tests/array_test 
