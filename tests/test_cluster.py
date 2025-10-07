"""Perform tests related to clustering graphs with SE2."""

import igraph as ig
import numpy as np
import pytest

import speakeasy2 as se2


class graph:
    def __init__(self, n_nodes: int, mu: float, seed: int) -> None:
        self._rng: np.random.Generator = np.random.default_rng(seed)
        seed = int(self._rng.integers(1, 1000))

        self._graph, self.ground_truth = self._create(n_nodes, mu, seed)
        self._weights = self._rng.normal(1, 0.25, (self._graph.ecount(),))

    def _create(
        self, n_nodes: int, mu: float, seed: int
    ) -> tuple[ig.Graph, ig.VertexClustering]:
        n_types = n_nodes // 20
        pref = np.zeros((n_types, n_types)) + mu
        diag = np.arange(n_types)
        pref[diag, diag] = 1 - mu

        type_dist = np.ones((n_types,)) / n_types
        type_dist[0] = type_dist[0] + (1 - type_dist.sum())

        g = ig.Graph.Preference(
            n_nodes, type_dist.tolist(), pref, attribute="type"
        )
        ground_truth = ig.VertexClustering(g, g.vs["type"])

        return (g, ground_truth)

    def as_type(self, dtype: str, isweighted: bool = False):
        convert_to = {"ndarray": self.to_array, "igraph": self.to_igraph}

        return convert_to[dtype](isweighted)

    def to_array(self, isweighted: bool) -> np.ndarray:
        n_nodes = self._graph.vcount()
        weights = self._weights if isweighted else 1
        i, j = zip(*self._graph.get_edgelist())
        arr = np.zeros((n_nodes, n_nodes))
        arr[i, j] = weights

        return arr

    def to_igraph(self, isweighted: bool) -> ig.Graph:
        g = self._graph.copy()

        if isweighted:
            g.es["weight"] = self._weights

        return g


class TestTypes:
    seed = 4939
    n_nodes = 100
    subcluster = 3
    mu = 0.2
    graph: ig.Graph

    @classmethod
    def setup_class(cls):
        cls.graph = graph(cls.n_nodes, cls.mu, cls.seed)

    @pytest.mark.parametrize("isweighted", [False, True])
    @pytest.mark.parametrize("dtype", ["ndarray", "igraph"])
    def test_clustering(self, dtype: str, isweighted: bool):
        g = self.graph.as_type(dtype, isweighted)
        expected = self.graph.ground_truth

        actual = se2.cluster(g, seed=self.seed + 1)
        assert ig.compare_communities(actual, expected, method="nmi") > 0.9

    def test_subclustering(self):
        g = self.graph.as_type("igraph")
        membs = se2.cluster(g, seed=self.seed + 1, subcluster=self.subcluster)

        assert len(membs) == self.subcluster
