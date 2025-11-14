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

Membership is returned as a `list` of node IDs.

```python
print(memb)
```

```python
[8, 8, 8, 8, 5, 5, 5, 8, 0, 2, 5, 1, 8, 8, 9, 9, 5, 8, 9, 8, 9, 8, 9, 6, 7, 7, 4, 6, 3, 4, 0, 7, 9, 9]
```

The results can be converted to an igraph `VertexClustering` object to access more structure metrics such as modularity with:

```python
print(ig.VertexClustering(g, memb))
```

```python
Clustering with 34 elements and 10 clusters
[ 0] 8, 30
[ 1] 11
[ 2] 9
[ 3] 28
[ 4] 26, 29
[ 5] 4, 5, 6, 10, 16
[ 6] 23, 27
[ 7] 24, 25, 31
[ 8] 0, 1, 2, 3, 7, 12, 13, 17, 19, 21
[ 9] 14, 15, 18, 20, 22, 32, 33
```

## Node ordering

For displaying results in a heatmap, a node ordering can be computed from the membership which groups nodes in a community together.
This can be used as an index and works to display the community structure using a heatmap to view the adjacency matrix.

```python
ordering = se2.order_nodes(g, memb)
```

## Weighted graphs

SpeakEasy 2 can work with weighted graphs by using a numpy `ndarray` as an adjacency matrix or passing weights to an igraph `Graph` object, either as a list with length equal to the number of edges or by using the igraph attribute table.

```python
g.es["weight"] = [1 for _ in range(g.ecount())]
memb = se2.cluster(g)
```

By default, SpeakEasy 2 will check if there is an edge attribute associated with the graph named `weight` and use those as weights. If you want to use a different edge attribute, pass the name of the attribute.

```python
memb = se2.cluster(g, weights="tie_strength")
```

Or if a graph has a weight edge attribute but you don't want to use them, explicitly pass `None` to the `weights` keyword argument.

## Subclustering

Subclustering can be used to detect hierarchical community structure.

```python
memb = se2.cluster(g, subcluster=2)
```

The number determines how many levels to perform community detection at.
The default, 1, means only to perform community detection at the top level (i.e. no subclustering).
When subclustering, membership will be a list of lists, the top level membership will be the object at index 0.

## Keyword arguments

A few other useful keywords arguments are `max_threads`, `verbose`, and `seed`.
The `max_thread` keyword determines how many processors SpeakEasy 2 is allowed to use. By default the value returned by OpenMP is used. To prevent parallel processing, explicitly pass `max_threads = 1` to the method.

The `verbose` option will cause the algorithm to print out some information about the process.

For reproducible results, the `seed` option sets the seed of the random number generator.
Note: this is a random number generator managed by the underlying C library and is independent of other random number generators that might have been set in python.

## KNN graph

Speakeasy2 also provides a `knn_graph` function for converting a matrix, where each column is the feature vector of a sample, into a sparse graph.
This is useful to reduce computation required to cluster large graphs.
The function returns a graph with non-zero edges only for the top `k` nearest neighbors (based on Euclidean distance) for each sample.

```python
from numpy.random import randn
samples = randn(20, 100)
g = se2.knn_graph(samples, k=10)
```

In the above example, there are 100 samples of 20 features, the resulting (directed) graph will have 10 edges leaving each of the 100 samples.

By default, `knn_graph` returns a binary graph.
Instead you can get a weighted graph with weights being the inverse of the distance.

## Installation

Speakeasy2 is available from PyPI so it can be installed with `pip` or other package managers.

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
