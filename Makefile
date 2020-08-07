CC := g++
CFLAGS := -Wall
CFLAGS += -std=c++17
CFLAGS += -O3
LFLAGS := 

ODIR := .OBJ

MAIN_SOURCE_DEPS := main.cpp

all: binaries

$(ODIR):
	@mkdir $(ODIR)

binaries: iterator 

MAIN_OBJECT_DEPS := $(ODIR)/main.o

iterator: $(ODIR) $(MAIN_OBJECT_DEPS)
	$(CC) $(CFLAGS) $(MAIN_OBJECT_DEPS) -o iterator $(LFLAGS)

$(ODIR)/main.o: $(ODIR) $(MAIN_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) main.cpp -o $(ODIR)/main.o

.PHONY: clean
clean:
	rm -rf .OBJ iterator 
