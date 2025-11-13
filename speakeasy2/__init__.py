"""The SpeakEasy2 community detection algorithm.

This module provides a community detection algorithm for clustering nodes in a
graph. The method accepts graphs created with the python-igraph package.

Methods:
-------
cluster : The SpeakEasy2 community detection algorithm.

Constants:
---------
__version__ : C library version.

Example:
-------
   >>> import igraph as ig
   >>> import speakeasy2 as se2
   >>> g = ig.Graph.Famous("Zachary")
   >>> memb = se2.cluster(g)

"""

__all__ = ["cluster", "knn_graph", "order_nodes", "__version__"]

import importlib.metadata
from typing import Optional

import igraph as _ig
import numpy as _np

from speakeasy2._speakeasy2 import cluster as _cluster
from speakeasy2._speakeasy2 import knn_graph as _knn_graph
from speakeasy2._speakeasy2 import order_nodes as _order_nodes

__version__ = importlib.metadata.version(__name__)


def _to_graph(arr: _np.ndarray, weights: str) -> _ig.Graph:
    assert arr.ndim == 2, "Array must be 2d to be an adjacency matrix."
    edgelist = list(zip(*arr.nonzero()))
    is_weighted = _np.logical_not(_np.logical_or(arr == 0, arr == 1)).sum() > 0
    is_directed = (arr.shape[0] == arr.shape[1]) and (
        _np.abs(arr - arr.T) < 1e-5
    ).all()

    g = _ig.Graph(edgelist, directed=is_directed)
    if is_weighted:
        g.es[weights] = [arr[i, j] for i, j in edgelist]

    return g


def cluster(
    g: _ig.Graph | _np.ndarray,
    weights: Optional[str | list[int]] = "weight",
    discard_transient: int = 3,
    independent_runs: int = 10,
    max_threads: int = 0,
    seed: int = 0,
    target_clusters: int = 0,
    target_partitions: int = 5,
    subcluster: int = 1,
    min_cluster: int = 5,
    verbose: bool = False,
) -> list[int] | list[list[int]]:
    """Cluster a graph using the SpeakEasy2 community detection algorithm.

    For all integer parameters below, values should be positive, setting a
    value to 0 gives the underlying C implementation responsibility for setting
    the default (for example choose of threads, random seed, and number of
    target clusters is left for the C implementation to decide on).

    Parameters
    ----------
    g : igraph.Graph, numpy.ndarray
        The graph to cluster. If `g` is a `numpy.ndarray` it is treated as an
        adjacency matrix and converted to an `igraph.Graph`.
    weights : str, list[float], None
        Optional name of weight attribute or list of weights. If a string, use
        the graph edge attribute with the given name (default is "weight"). If
        a list, must have length equal to the number of edges in the graph.
    discard_transient : int
        The number of partitions to discard before tracking. Default 3.
    independent_runs : int
        How many runs SpeakEasy2 should perform. Default 10.
    max_threads : int
        The maximum number of threads to use. By default OpenMP will determine
        the number to use based on either the value of the environment variable
        OMP_NUM_THREADS or the number of available virtual cores.
    seed : int
        A seed to use for the random number generator for reproducible results.
    target_clusters : int
        The expected number of clusters to find (default is dependent on the
        size of the graph). This only impacts how many initial labels are used.
        The final number of labels may be different.
    target_partitions : int
        Number of partitions to find per independent run (default 5).
    subcluster : int
        How many levels of community detection to run (default 1). If greater
        than 1, community detection will be run each of the resulting
        communities to perform subclustering. This is repeated subcluster
        times.
    min_cluster : int
        Smallest community to perform subclustering on (default 5). Ignored if
        subcluster is 1 (i.e. no subclustering).
    verbose : bool
        Whether to provide additional information about the clustering or not.

    Returns
    -------
    memb : list[int] | list[list[int]]
        The detected community structure. If subclustering, a nested list will
        be returned, one list for each level of clustering. The top level
        clustering is in index 0.

    """
    if isinstance(g, _np.ndarray):
        if not isinstance(weights, str):
            raise ValueError(
                "Weights must be a string for an adjacency matrix."
            )
        g = _to_graph(g, weights=weights)

    if isinstance(weights, str):
        if weights in g.edge_attributes():
            weights = g.es[weights]
        elif weights == "weight":
            weights = None
        else:
            raise KeyError(f"Graph does not have edge attribute {weights}")

    return _cluster(
        g,
        weights,
        discard_transient=discard_transient,
        independent_runs=independent_runs,
        max_threads=max_threads,
        seed=seed,
        target_clusters=target_clusters,
        target_partitions=target_partitions,
        subcluster=subcluster,
        min_cluster=min_cluster,
        verbose=verbose,
    )


def knn_graph(
    cols: _np.ndarray, k: int, is_weighted: bool = False
) -> _ig.Graph:
    """Calculate a k-nearest neighbors graphs between columns.

    Uses Euclidean distance to determine the k closest columns to each column.
    In the resulting graph, there is a directed edge between each column and
    it's k-nearest neighbors.

    Parameters
    ----------
    cols: numpy.ndarray
        The columns to compare. Should be in the form of a 2d array.
    k: int
        Number of closest neighbors to connect. Must be less than the number of
        columns.
    is_weighted: bool
        Whether to return a weighted graph where weights are the similarity
        between the two columns (1 / distance). Default False.

    Returns
    -------
    graph: igraph.Graph
        The knn graph as an igraph.Graph object.

    """
    if len(cols.shape) != 2:
        raise ValueError("Cols must be 2-dimensional.")

    g_base, weights = _knn_graph(cols, k, is_weighted)

    # TODO: Ideally ig.Graph returned from C but currently get a GraphBase
    # type.
    g = _ig.Graph(g_base.vcount(), g_base.get_edgelist(), g_base.is_directed())
    if weights:
        g.es["weight"] = weights

    return g


def order_nodes(
    g: _ig.Graph | _np.ndarray,
    membership: list[int] | list[list[int]],
    weights: Optional[str | list[int]] = "weight",
) -> list[int] | list[list[int]]:
    """Order nodes by communities to emphasize network structure.

    This ordering can be used with heatmaps to ensure nodes within a community
    a displayed together.

    Uses the partition to group nodes so that members of the same community are
    together. Communities are ordered by size such that the first nodes are the
    nodes in the largest community and the last nodes are those in the smallest
    community. Within communities, nodes are ordered by degree.

    If membership is a list VertexClustering objects (the case when obtained by
    running SpeakEasy2 with subclustering), ordering is done recursively. In
    this case, one ordering is returned for each level of subclustering. The
    nodes in the largest community of level 1 will still be the first nodes in
    all lower level ordering, but the ordering within the first top level
    community will be changed to emphasize the lower level communities.

    Parameters
    ----------
    g : igraph.Graph
        The graph to cluster.
    membership : list[int] | list[list[int]]
        Community labels obtained from a community detection algorithm.
    weights : str, list[float], None
        Optional name of weight attribute or list of weights. If a string, use
        the graph edge attribute with the given name (default is "weight"). If
        a list, must have length equal to the number of edges in the graph.

    Returns
    -------
    ordering : list[int] | list[list[int]]
        An array of indices with shape equal to the input membership that can
        be applied to reorder nodes such that nodes in the same community are
        grouped together.

    """
    if isinstance(g, _np.ndarray):
        if not isinstance(weights, str):
            raise ValueError(
                "Weights must be a string for an adjacency matrix."
            )
        g = _to_graph(g, weights)

    if isinstance(weights, str):
        if weights in g.edge_attributes():
            weights = g.es[weights]
        elif weights == "weight":
            weights = None
        else:
            raise KeyError(f"Graph does not have edge attribute {weights}")

    return _order_nodes(g, membership, weights=weights)
