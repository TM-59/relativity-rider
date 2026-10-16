CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS  += -lm
SRC     := src/relativity.c src/svg.c src/render.c

.PHONY: all test plots clean
all: relativity-rider

relativity-rider: $(SRC) src/main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test_relativity: $(SRC) tests/test_relativity.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: test_relativity
	./test_relativity

# Regenerates the CSV and the SVG charts shown in the README.
plots: relativity-rider
	mkdir -p docs
	./relativity-rider --headless docs

clean:
	rm -f relativity-rider test_relativity
