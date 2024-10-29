# Python SpeakEasy2 package

![PyPI - Version](https://img.shields.io/pypi/v/speakeasy2)
![PyPI - Python Version](https://img.shields.io/pypi/pyversions/speakeasy2)


Provides the SpeakEasy2 community detection algorithm to cluster graph's stored as igraph's data type. The algorithm is described in the [Genome Biology article](https://genomebiology.biomedcentral.com/articles/10.1186/s13059-023-03062-0).

This uses a rewrite of the algorithm used in the publication, to see a comparison to the original implementation see [the benchmarks](https://github.com/SpeakEasy-2/libspeakeasy2/tree/master/benchmarks)

Example:

```python
 import igraph as ig
 import speakeasy2 as se2

 g = ig.Graph.Famous("Zachary")
 memb = se2.cluster(g)
```

Membership is returned as an `igraph.clustering.VertexClustering` object.
Use `print` to view the membership:

```python
print(memb)
```

```python
Clustering with 34 elements and 9 clusters
[0] 0, 1, 2, 3, 7, 12, 13, 17, 19, 21
[1] 14, 15, 18, 20, 22, 32, 33
[2] 8, 30
[3] 26, 29
[4] 11
[5] 23, 24, 25, 27, 31
[6] 9
[7] 28
[8] 4, 5, 6, 10, 16
```

Or to convert to a python list for use outside of `igraph` run `memb.membership`.

From the results, a node ordering can be computed to group nodes in a community together. This can be used as an index and works to display the community structure using a heatmap to view the adjacency matrix.

```python
ordering = se2.order_nodes(g, memb)
```

SpeakEasy 2 can work with weighted graphs by either passing weights as a list with length equal to the number of edges or by using the igraph attribute table.

```python
g.es["weight"] = [1 for _ in range(g.ecount())]
memb = se2.cluster(g)
```

By default, SpeakEasy 2 will check if there is an edge attribute associated with the graph named `weight` and use those as weights. If you want to use a different edge attribute, pass the name of the attribute.

```python
memb = se2.cluster(g, weights="tie_strength")
```

Or if a graph has a weight edge attribute but you don't want to use them, explicitly pass `None` to the `weights` keyword argument.

Subclustering can be used to detect hierarchical community structure.

```python
memb = se2.cluster(g, subcluster=2)
```

The number determines how many levels to perform community detection at. The default 1 means only to perform community detection at the top level (i.e. no subclustering). When subclustering, membership will be a list of `igraph.VertexClustering` objects, the top level membership will be the object at index 0.

A few other useful keywords arguments are `max_threads`, `verbose`, and `seed`. The `max_thread` keyword determines how many processors SpeakEasy 2 is allowed to use. By default the value returned by OpenMP is used. To prevent parallel processing, explicitly pass `max_threads = 1` to the method.

The `verbose` option will cause the algorithm to print out some information about the process.

For reproducible results, the `seed` option sets the seed of the random number generator. Note: this is a random number generator managed by the underlying C library and is independent of other random number generators that might have been set in python.

## Installation

speakeasy2 is available from pypi so it can be installed with `pip` or other package managers.

```bash
pip install --user speakeasy2
```

## Building from source

Compilation depends on a C compiler, CMake, and (optionally) ninja.

Since the `igraph` package is supplied by the vendored SE2 C library, after cloning the source directory, submodules most be recursively initialized.

```bash
git clone "https://github.com/SpeakEasy-2/python-speakeasy2"
cd python-speakeasy2
git submodule update --init --recursive
```

The CMake calls are wrapped into the python build logic in the `build_script.py` (this is a `poetry` specific method for building C extensions).
This allows the package to be built using various python build backends.
Since this package uses poetry, the suggested way to build the package is invoking `poetry build` and `poetry install`, which will install in development mode.

For convenience, the provided `Makefile` defines the `install` target to do this and `clean-dist` to clear all generated files (as well as other targets, see the file for more).

It should now be possible to run scripts through `poetry`:

```bash
poetry run ipython path/to/script.py
```

Or enter a python repository with the private environment activate in the same way.

```bash
poetry run ipython
```

If you don't want to use `poetry`, it's possible to build with other method in their standard way.
For example `python -m build` or `pip install --editable .` should both work.
