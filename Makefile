PYTHON_MODULE := $(PWD)/speakeasy2

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
compile-commands: clean-dist
	bear -- $(MAKE) build

.PHONY: clean
clean:
	rm -rf build/temp*

.PHONY: clean-dist
clean-dist: clean
	rm -f $(PYTHON_MODULE)/_*.so
	rm -rf dist
	rm -rf build
	rm -f setup.py
