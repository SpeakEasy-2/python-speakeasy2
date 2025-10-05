PYTHON_MODULE := $(PWD)/speakeasy2
NUMPY_INCLUDE := $$(python -c 'import numpy as np; print(np.get_include())')

.PHONY: all
all: build

.PHONY: build
build:
	uv build

.PHONY: sdist
sdist:
	uv build --sdist

.PHONY: wheel
wheel:
	uv build --wheel

.PHONY: dist
dist: clean-dist
	$(MAKE) build

build/wheel:
	python -c "import scikit_build_core.build as build; build.build_wheel(\"$@\")"

.PHONY: clean
clean:
	rm -rf build

.PHONY: clean-dist
clean-dist: clean
	rm -f $(PYTHON_MODULE)/_*.so
	rm -rf dist
	rm -f setup.py
