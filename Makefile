CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS  += -lm
SRC     := src/relativity.c src/svg.c src/render.c src/gif.c
EMCC    ?= emcc
PYTHON  ?= python3
PORT    ?= 4217

.PHONY: all test bench web play smoke plots clean
all: relativity-rider

relativity-rider: $(SRC) src/main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test_relativity: $(SRC) tests/test_relativity.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test_gif: src/gif.c tests/test_gif.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test_rider: src/relativity.c src/rider.c tests/test_rider.c src/relativity.h src/rider.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

test: test_relativity test_gif test_rider
	./test_relativity
	./test_gif
	./test_rider

bench: test_rider
	./test_rider
	@mkdir -p build
	./test_rider --trace > build/native-trace.csv

# Browser observatory: the same C model compiled to WebAssembly (local only, needs emsdk).
web: dist/physics.mjs
	cp web/index.html web/style.css web/app.mjs dist/
	mkdir -p dist/art
	cp assets/art/* dist/art/
	$(PYTHON) scripts/check-size.py

dist/physics.mjs: src/relativity.c src/rider.c src/relativity.h src/rider.h
	mkdir -p dist
	$(EMCC) $(CFLAGS) src/relativity.c src/rider.c -lm --no-entry -o $@ \
	  -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sFILESYSTEM=0 \
	  -sEXPORTED_FUNCTIONS=_rr_reset,_rr_thrust,_rr_step,_rr_value,_rr_trail_t,_rr_trail_x

play: web
	$(PYTHON) -m http.server $(PORT) --bind 127.0.0.1 --directory dist

smoke: web bench relativity-rider
	node scripts/smoke.cjs

# Regenerates the CSV and the SVG charts shown in the README.
plots: relativity-rider
	mkdir -p docs
	./relativity-rider --headless docs
	./relativity-rider --gif docs/demo.gif
	head -c6 docs/demo.gif | grep -q GIF89a

clean:
	rm -f relativity-rider test_relativity test_gif test_rider
	rm -rf build dist
