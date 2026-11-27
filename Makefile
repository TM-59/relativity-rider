CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS  += -lm
SRC     := src/relativity.c src/svg.c src/render.c src/gif.c src/schwarzschild.c src/orbit.c src/aberration.c src/skyview.c
EMCC    ?= emcc
PYTHON  ?= python3
PORT    ?= 4217

GEO     := src/schwarzschild.c src/orbit.c
GEO_H   := src/schwarzschild.h src/orbit.h

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

test_schwarzschild: src/relativity.c $(GEO) tests/test_schwarzschild.c src/relativity.h $(GEO_H)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

COURIER   := src/relativity.c src/courier.c
COURIER_H := src/relativity.h src/courier.h tests/courier_policy.h tests/courier_run.h

SKY   := src/aberration.c src/skyview.c
SKY_H := src/aberration.h src/skyview.h

test_aberration: src/relativity.c $(SKY) tests/test_aberration.c src/relativity.h $(SKY_H)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

test_courier: $(COURIER) tests/test_courier.c $(COURIER_H)
	$(CC) $(CFLAGS) -o $@ $(COURIER) tests/test_courier.c $(LDLIBS)

story_bench: $(COURIER) tests/story_bench.c $(COURIER_H)
	$(CC) $(CFLAGS) -o $@ $(COURIER) tests/story_bench.c $(LDLIBS)

# The story rows are compared byte for byte; the CI runs them with the Debian 13 toolchain of the Dockerfile.
test: test_relativity test_gif test_rider test_schwarzschild test_aberration test_courier story_bench
	./test_relativity
	./test_gif
	./test_rider
	./test_schwarzschild
	./test_aberration
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

bench: test_rider test_schwarzschild test_aberration
	./test_rider
	@mkdir -p build
	./test_rider --trace > build/native-trace.csv
	./test_schwarzschild --trace > build/orbit-trace.csv
	./test_aberration --trace > build/sky-trace.csv

# Browser observatory: the same C model compiled to WebAssembly (local only, needs emsdk).
web: dist/physics.mjs
	cp web/index.html web/style.css web/app.mjs web/story.mjs web/orbits.mjs web/sky.mjs web/bgm.mjs dist/
	mkdir -p dist/bgm
	cp web/bgm/* dist/bgm/
	mkdir -p dist/art
	cp assets/art/* dist/art/
	$(PYTHON) scripts/check-size.py

dist/physics.mjs: src/relativity.c src/rider.c src/relativity.h src/rider.h src/courier.c src/courier.h src/courier_web.c $(GEO) $(GEO_H) $(SKY) $(SKY_H)
	mkdir -p dist
	$(EMCC) $(CFLAGS) src/relativity.c src/rider.c src/courier.c src/courier_web.c $(GEO) $(SKY) -lm --no-entry -o $@ \
	  -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sFILESYSTEM=0 \
	  -sEXPORTED_FUNCTIONS=_rr_reset,_rr_thrust,_rr_step,_rr_value,_rr_trail_t,_rr_trail_x,_cw_reset,_cw_tick,_cw_answer,_cw_value,_so_reset,_so_step,_so_value,_so_trail_x,_so_trail_y,_so_ntrail_x,_so_ntrail_y,_so_apsis,_so_curve,_so_ncurve,_sv_reset,_sv_value,_sv_star,_sv_curve

play: web
	$(PYTHON) -m http.server $(PORT) --bind 127.0.0.1 --directory dist

smoke: web bench relativity-rider
	node scripts/smoke.cjs

# Regenerates the CSV and the SVG charts shown in the README.
plots: relativity-rider
	mkdir -p docs
	./relativity-rider --headless docs
	./relativity-rider --gif docs/demo.gif
	./relativity-rider --orbit docs
	./relativity-rider --sky docs
	head -c6 docs/demo.gif | grep -q GIF89a

# Regenerates the plotted figures and the measured-values table of docs/theory (one command).
theory_figs: src/relativity.c src/rider.c src/svg.c $(GEO) $(SKY) tests/theory_figs.c src/relativity.h src/rider.h src/svg.h $(GEO_H) $(SKY_H)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

theory-figs: theory_figs
	mkdir -p docs/theory/figures
	./theory_figs docs/theory/figures

# Fails when a statement of docs/theory has no figure, a link is broken, or a referenced function or test is gone.
theory-check:
	$(PYTHON) scripts/theory-check.py

clean:
	rm -f relativity-rider test_relativity test_gif test_rider test_schwarzschild test_aberration test_courier story_bench theory_figs
	rm -rf build dist

# ---- M61: local-only test standard (~/project-context/rules/test-standard.json) ----
# `make test` above stays the CI gate. `make check` runs every adopted kind through the
# common isolated runner (tm test) and writes git-ignored JUnit + test-results/summary.json.
TM_TEST_PYTHON ?= python3.14
TM_TEST_CMAKE ?= cmake
TM_TEST_CTEST ?= ctest
TM_TEST_NPM_CACHE ?= $(HOME)/.npm
TM_UNIT_BINS := test_relativity test_gif test_rider test_schwarzschild test_aberration test_courier story_bench

.PHONY: test-unit check test-trace test-perf test-e2e tm-ctest tm-deps test-tm-unit test-tm-trace test-tm-perf test-tm-e2e
test-unit:
	$(TM_TEST_PYTHON) scripts/local-test.py unit

check:
	$(TM_TEST_PYTHON) scripts/local-test.py all

test-trace test-perf test-e2e:
	$(TM_TEST_PYTHON) scripts/local-test.py $(@:test-%=%)

tm-ctest:
	mkdir -p test-results
	"$(TM_TEST_CMAKE)" -S tests/ctest -B .tm-ctest >/dev/null

test-tm-unit test-tm-trace: tm-ctest
	$(MAKE) -j1 $(TM_UNIT_BINS)
	"$(TM_TEST_CTEST)" --test-dir .tm-ctest -j 1 -L '^$(@:test-tm-%=%)$$' --output-on-failure --output-junit "$(CURDIR)/test-results/$(@:test-tm-%=%).xml"

test-tm-perf: tm-ctest
	"$(TM_TEST_CTEST)" --test-dir .tm-ctest -j 1 -L '^perf$$' --output-on-failure --output-junit "$(CURDIR)/test-results/perf.xml"

tm-deps:
	@test -n "$(TM_TEST_RUN_ID)" || { echo "Requires isolated tm test" >&2; exit 1; }
	mkdir -p .tm-home && touch .tm-home/user.npmrc .tm-home/global.npmrc
	@test -d node_modules/playwright || npm ci --offline --ignore-scripts --no-audit --no-fund --userconfig="$(CURDIR)/.tm-home/user.npmrc" --globalconfig="$(CURDIR)/.tm-home/global.npmrc" --cache="$(TM_TEST_NPM_CACHE)"

test-tm-e2e: tm-ctest tm-deps
	"$(TM_TEST_CTEST)" --test-dir .tm-ctest -j 1 -L '^e2e$$' --output-on-failure --output-junit "$(CURDIR)/test-results/e2e.xml"
