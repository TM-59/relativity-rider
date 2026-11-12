CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS  += -lm
SRC     := src/relativity.c src/svg.c src/render.c src/gif.c
EMCC    ?= emcc
PYTHON  ?= python3
PORT    ?= 4217

.PHONY: all test bench story story-check web play smoke plots clean theory-figs theory-check
all: relativity-rider

relativity-rider: $(SRC) src/main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test_relativity: $(SRC) tests/test_relativity.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test_gif: src/gif.c tests/test_gif.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test_rider: src/relativity.c src/rider.c tests/test_rider.c src/relativity.h src/rider.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

COURIER   := src/relativity.c src/courier.c
COURIER_H := src/relativity.h src/courier.h tests/courier_policy.h tests/courier_run.h

test_courier: $(COURIER) tests/test_courier.c $(COURIER_H)
	$(CC) $(CFLAGS) -o $@ $(COURIER) tests/test_courier.c $(LDLIBS)

story_bench: $(COURIER) tests/story_bench.c $(COURIER_H)
	$(CC) $(CFLAGS) -o $@ $(COURIER) tests/story_bench.c $(LDLIBS)

# The story rows are compared byte for byte; the CI runs them with the Debian 13 toolchain of the Dockerfile.
test: test_relativity test_gif test_rider test_courier story_bench
	./test_relativity
	./test_gif
	./test_rider
	./test_courier
	./story_bench --check docs/narrative

# Rewrites docs/narrative/{play-order,ablation,tension}.json.
story: story_bench
	mkdir -p docs/narrative
	./story_bench docs/narrative

# Regenerates every row into a scratch directory and compares the three files byte for byte.
story-check: story_bench
	rm -rf .story-check && mkdir .story-check
	./story_bench .story-check
	cmp .story-check/play-order.json docs/narrative/play-order.json
	cmp .story-check/ablation.json docs/narrative/ablation.json
	cmp .story-check/tension.json docs/narrative/tension.json
	rm -rf .story-check

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

dist/physics.mjs: src/relativity.c src/rider.c src/relativity.h src/rider.h src/courier.c src/courier.h src/courier_web.c
	mkdir -p dist
	$(EMCC) $(CFLAGS) src/relativity.c src/rider.c src/courier.c src/courier_web.c -lm --no-entry -o $@ \
	  -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sFILESYSTEM=0 \
	  -sEXPORTED_FUNCTIONS=_rr_reset,_rr_thrust,_rr_step,_rr_value,_rr_trail_t,_rr_trail_x,_cw_reset,_cw_tick,_cw_answer,_cw_value

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

# Regenerates the plotted figures and the measured-values table of docs/theory (one command).
theory_figs: src/relativity.c src/rider.c src/svg.c tests/theory_figs.c src/relativity.h src/rider.h src/svg.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

theory-figs: theory_figs
	mkdir -p docs/theory/figures
	./theory_figs docs/theory/figures

# Fails when a statement of docs/theory has no figure, a link is broken, or a referenced function or test is gone.
theory-check:
	$(PYTHON) scripts/theory-check.py

clean:
	rm -f relativity-rider test_relativity test_gif test_rider test_courier story_bench theory_figs
	rm -rf build dist
