# One-command reproduction. See README.md.
#
#   make              build + test
#   make smoke        1-minute pipeline check -> results/<ts>-smoke, figures/
#   make quick        10-20 minute coarse sweep
#   make full         the paper's sweep (hours)
#   make analyze RUN=results/<dir>     re-analyse an existing run
#   make paper        build paper/main.pdf (needs latexmk)
#
# Optional: PIN=<core> pins the benchmark thread, e.g. make full PIN=2

PY      := .venv/bin/python
PIN     ?=
RUN     ?= $(shell ls -d results/*/ 2>/dev/null | sort | tail -1)
# Pool several runs: make analyze RUNS="results/a results/b results/c" OUT=results/pooled
RUNS    ?= $(RUN)
OUT     ?= $(lastword $(RUNS))/analysis

.PHONY: all build test smoke quick refine patch full analyze figures paper clean venv build-native native-quick

all: build test

build:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > /dev/null
	cmake --build build -j

test: build
	cd build && ctest --output-on-failure

venv: .venv/bin/activate
.venv/bin/activate: requirements.txt
	python3 -m venv .venv
	.venv/bin/pip install -q --upgrade pip
	.venv/bin/pip install -q -r requirements.txt
	touch .venv/bin/activate

smoke: build test venv
	scripts/run_all.sh smoke results/$$(date +%Y%m%d-%H%M%S)-smoke $(PIN)
	$(MAKE) analyze

quick: build test venv
	scripts/run_all.sh quick results/$$(date +%Y%m%d-%H%M%S)-quick $(PIN)
	$(MAKE) analyze

refine: build test venv
	scripts/run_all.sh refine results/$$(date +%Y%m%d-%H%M%S)-refine $(PIN)

patch: build test venv
	scripts/run_all.sh patch results/$$(date +%Y%m%d-%H%M%S)-patch $(PIN)

full: build test venv
	scripts/run_all.sh full results/$$(date +%Y%m%d-%H%M%S)-full $(PIN)
	$(MAKE) analyze

# Secondary experiment: same code, -O3 -march=native. Results land in a
# separate run directory so they are never pooled with the main sweep.
build-native:
	cmake -S . -B build-native -DCMAKE_BUILD_TYPE=Release -DATLAS_O3=ON -DATLAS_MARCH_NATIVE=ON > /dev/null
	cmake --build build-native -j

native-quick: build-native venv
	BIN=$(CURDIR)/build-native scripts/run_all.sh quick results/$$(date +%Y%m%d-%H%M%S)-native-quick $(PIN)
	$(PY) analysis/atlas.py $$(ls -d results/*-native-quick | sort | tail -1)

analyze: venv
	@test -n "$(RUNS)" || (echo "no run found; pass RUN=results/<dir> or RUNS=\"results/a results/b\""; exit 1)
	$(PY) analysis/atlas.py $(RUNS) --out $(OUT)
	$(PY) analysis/figures.py $(OUT) --out figures --sysinfo $(lastword $(RUNS))/sysinfo.txt

figures: analyze

paper:
	cd paper && latexmk -pdf -quiet main.tex

clean:
	rm -rf build .venv
