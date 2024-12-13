PYTHON_MODULE := $(PWD)/speakeasy2
NUMPY_INCLUDE := $$(python -c 'import numpy as np; print(np.get_include())')

.PHONY: all
all: install

.PHONY: install
install: build
	poetry install

.PHONY: build
build:
	poetry build

.PHONY: sdist
sdist:
	poetry build --format sdist

.PHONY: wheel
wheel:
	poetry build --format wheel

.PHONY: dist
dist: clean-dist
	$(MAKE) build

.PHONY: compile_commands
compile_commands: clean-dist
	bear -- $(MAKE) build
	sed -i \
	  "s#/tmp/tmp.*/\.venv/lib/[^\"]*#$(NUMPY_INCLUDE)#" \
	  compile_commands.json

.PHONY: clean
clean:
	rm -rf build

.PHONY: clean-dist
clean-dist: clean
	rm -f $(PYTHON_MODULE)/_*.so
	rm -rf dist
	rm -f setup.py
