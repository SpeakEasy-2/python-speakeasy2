#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <igraph.h>
#include <igraphmodule_api.h>
#include <numpy/arrayobject.h>
#include <speak_easy_2.h>

#define PYIGRAPH_CHECK(expr)                                                  \
  do {                                                                        \
    igraph_error_t igraph_i_ret = (expr);                                     \
    if (IGRAPH_UNLIKELY(igraph_i_ret != IGRAPH_SUCCESS)) {                    \
      IGRAPH_ERROR_NO_RETURN("", igraph_i_ret);                               \
      return NULL;                                                            \
    }                                                                         \
  } while (0)

void se2_pywarning(char const* reason, char const* file, int line)
{
  char msg[512];
  snprintf(msg, sizeof(msg), "%s\n\n> In %s (line %d)\n", reason, file, line);
  PyErr_WarnEx(PyExc_RuntimeWarning, msg, 1);
}

void se2_pyerror(
  char const* reason, char const* file, int line, igraph_error_t igraph_errno)
{
  char const* errmsg = igraph_strerror(igraph_errno);
  PyObject* type = PyExc_RuntimeError;
  char msg[1024];
  snprintf(
    msg, sizeof(msg) - 1, "%s: %s\n\n%s -- %d\n", errmsg, reason, file, line);

  if (igraph_errno == IGRAPH_ENOMEM) {
    type = PyExc_MemoryError;
  }

  if (igraph_errno == IGRAPH_INTERRUPTED) {
    type = PyExc_KeyboardInterrupt;
  }

  IGRAPH_FINALLY_FREE();

  if (!PyErr_Occurred()) {
    PyErr_SetString(type, msg);
  }
}

igraph_error_t se2_pystatus(char const* message, void* data)
{
  PyObject* msg = PyUnicode_FromString(message);
  if (msg == NULL) {
    return IGRAPH_FAILURE;
  }

  PyObject* py_stdout = PySys_GetObject("stdout");
  if (py_stdout == NULL) {
    return IGRAPH_FAILURE;
  }

  int ret = PyFile_WriteObject(msg, py_stdout, Py_PRINT_RAW);
  Py_DECREF(msg);

  return (ret < 0) ? IGRAPH_FAILURE : IGRAPH_SUCCESS;
}

igraph_bool_t se2_pyinterrupt() { return PyErr_CheckSignals() < 0; }

static void se2_init(void)
{
  igraph_set_error_handler(se2_pyerror);
  igraph_set_warning_handler(se2_pywarning);
  igraph_set_status_handler(se2_pystatus);
  igraph_set_interruption_handler(se2_pyinterrupt);
}

static igraph_error_t py_sequence_to_igraph_vector_i(
  PyObject* seq, igraph_vector_t* vec)
{
  size_t n_edges = PySequence_Size(seq);
  if ((n_edges == (size_t)-1) && PyErr_Occurred()) {
    return IGRAPH_FAILURE;
  }

  IGRAPH_CHECK(igraph_vector_init(vec, n_edges));
  IGRAPH_FINALLY(igraph_vector_destroy, vec);
  for (size_t i = 0; i < n_edges; i++) {
    PyObject* item = PySequence_GetItem(seq, i);
    if (item == NULL) {
      return IGRAPH_FAILURE;
    }

    double val = PyFloat_AsDouble(item);
    Py_DECREF(item);
    if (val == -1.0 && PyErr_Occurred()) {
      return IGRAPH_FAILURE;
    }

    VECTOR(*vec)[i] = val;
  }
  IGRAPH_FINALLY_CLEAN(1);

  return IGRAPH_SUCCESS;
}

static igraph_error_t py_list_to_igraph_matrix_int_i(
  PyObject* list, igraph_matrix_int_t* mat)
{
  size_t n_row = PyList_Size(list);
  PyObject* first_el = PyList_GetItem(list, 0);
  size_t n_col = 0;
  bool nested = true;
  if (PyList_Check(first_el)) {
    n_col = PyList_Size(first_el);
  } else {
    n_col = n_row;
    n_row = 1;
    nested = false;
  }

  IGRAPH_CHECK(igraph_matrix_int_init(mat, n_row, n_col));
  IGRAPH_FINALLY(igraph_matrix_int_destroy, mat);
  for (size_t i = 0; i < n_row; i++) {
    PyObject* inner = nested ? PyList_GetItem(list, i) : list;
    for (size_t j = 0; j < n_col; j++) {
      double val = PyFloat_AsDouble(PyList_GetItem(inner, j));
      if (val == -1.0 && PyErr_Occurred()) {
        return IGRAPH_FAILURE;
      }
      MATRIX(*mat, i, j) = val;
    }
  }
  IGRAPH_FINALLY_CLEAN(1);

  return IGRAPH_SUCCESS;
}

typedef igraph_real_t element_to_double_t(char* dataptr);

static igraph_real_t arr_bool_to_double_i(char* dataptr)
{
  return *(npy_bool*)dataptr;
}

static igraph_real_t arr_float32_to_double_i(char* dataptr)
{
  return *(npy_float32*)dataptr;
}

static igraph_real_t arr_float64_to_double_i(char* dataptr)
{
  return *(npy_float64*)dataptr;
}

static igraph_real_t arr_integer_to_double_i(char* dataptr)
{
  return *(npy_int*)dataptr;
}

static element_to_double_t* get_element_converter(PyArrayObject* arr)
{
  if (PyArray_ISBOOL(arr)) {
    return arr_bool_to_double_i;
  }

  if (PyArray_TYPE(arr) == NPY_FLOAT32) {
    return arr_float32_to_double_i;
  }

  if (PyArray_TYPE(arr) == NPY_FLOAT64) {
    return arr_float64_to_double_i;
  }

  if (PyArray_ISINTEGER(arr)) {
    return arr_integer_to_double_i;
  }

  PyErr_SetString(PyExc_TypeError,
    "Data type of \"cols\" array is not handled. Please report or cast to "
    "another data type with \"cols.astype\".");
  return NULL;
}

static igraph_error_t ndarray_to_igraph_matrix_i(
  PyArrayObject* arr, igraph_matrix_t* mat)
{
  npy_intp* dims = PyArray_DIMS(arr);
  npy_intp stride_row = PyArray_STRIDE(arr, 0);
  npy_intp stride_col = PyArray_STRIDE(arr, 1);
  element_to_double_t* element_to_double;

  element_to_double = get_element_converter(arr);
  if (!element_to_double) {
    return IGRAPH_FAILURE;
  }

  IGRAPH_CHECK(igraph_matrix_init(mat, dims[0], dims[1]));
  IGRAPH_FINALLY(igraph_matrix_destroy, mat);

  for (npy_intp j = 0; j < dims[1]; j++) {
    char* dataptr = PyArray_BYTES(arr) + (stride_col * j);
    for (npy_intp i = 0; i < dims[0]; i++) {
      MATRIX(*mat, i, j) = element_to_double(dataptr);
      dataptr += stride_row;
    }
  }

  return IGRAPH_SUCCESS;
}

static PyObject* igraph_matrix_int_to_py_list_i(igraph_matrix_int_t* mat)
{
  PyObject* outer = PyList_New(igraph_matrix_int_nrow(mat));
  if (!outer) {
    return NULL;
  }

  for (igraph_integer_t i = 0; i < igraph_matrix_int_nrow(mat); i++) {
    PyObject* inner = PyList_New(igraph_matrix_int_ncol(mat));
    if (!inner) {
      return NULL;
    }
    for (igraph_integer_t j = 0; j < igraph_matrix_int_ncol(mat); j++) {
      PyList_SetItem(inner, j, PyLong_FromLong(MATRIX(*mat, i, j)));
    }
    PyList_SetItem(outer, i, inner);
  }

  if (igraph_matrix_int_nrow(mat) == 1) {
    PyObject* res = PyList_GetItem(outer, 0);
    Py_INCREF(res);
    Py_DECREF(outer);
    return res;
  }

  return outer;
}

static PyObject* igraph_vector_to_py_list_i(igraph_vector_t* vec)
{
  if (!vec) {
    return PyList_New(0);
  }

  PyObject* res = PyList_New(igraph_vector_size(vec));
  for (igraph_integer_t i = 0; i < igraph_vector_size(vec); i++) {
    PyList_SetItem(res, i, PyFloat_FromDouble(VECTOR(*vec)[i]));
  }

  return res;
}

static PyObject* cluster(
  PyObject* Py_UNUSED(dummy), PyObject* args, PyObject* kwds)
{
  se2_init();

  PyObject* py_graph_obj = NULL;
  PyObject* py_weights_obj = NULL;
  igraph_t* graph;
  se2_neighs neigh_list;
  igraph_vector_t weights;
  char* kwlist[] = { "graph", "weights", "discard_transient",
    "independent_runs", "max_threads", "seed", "target_clusters",
    "target_partitions", "subcluster", "min_cluster", "verbose", NULL };
  int discard_transient = 0;
  int independent_runs = 0;
  int max_threads = 0;
  int seed = 0;
  int target_clusters = 0;
  int target_partitions = 0;
  int subcluster = 0;
  int min_cluster = 0;
  int verbose = false;
  igraph_matrix_int_t memb;
  PyObject* py_memb_obj;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|Oiiiiiiiip", kwlist,
        &py_graph_obj, &py_weights_obj, &discard_transient, &independent_runs,
        &max_threads, &seed, &target_clusters, &target_partitions, &subcluster,
        &min_cluster, &verbose)) {
    return NULL;
  }

  se2_options opts = {
    .discard_transient = discard_transient,
    .independent_runs = independent_runs,
    .max_threads = max_threads,
    .random_seed = seed,
    .target_clusters = target_clusters,
    .target_partitions = target_partitions,
    .subcluster = subcluster,
    .minclust = min_cluster,
    .verbose = verbose,
  };

  graph = PyIGraph_ToCGraph(py_graph_obj);

  if (target_clusters > igraph_vcount(graph)) {
    PyErr_SetString(PyExc_ValueError,
      "Number of target clusters cannot exceed the number of "
      "nodes in the graph.");
  }

  if (py_weights_obj && PySequence_Check(py_weights_obj)) {
    PYIGRAPH_CHECK(py_sequence_to_igraph_vector_i(py_weights_obj, &weights));
    IGRAPH_FINALLY(igraph_vector_destroy, &weights);
    if (igraph_vector_size(&weights) != igraph_ecount(graph)) {
      IGRAPH_FINALLY_FREE();
      PyErr_SetString(PyExc_ValueError,
        "Number of weights does not match number of edges in graph.");
      return NULL;
    }

    PYIGRAPH_CHECK(se2_igraph_to_neighbor_list(graph, &weights, &neigh_list));
    igraph_vector_destroy(&weights);
    IGRAPH_FINALLY_CLEAN(1);
  } else {
    PYIGRAPH_CHECK(se2_igraph_to_neighbor_list(graph, NULL, &neigh_list));
  }
  IGRAPH_FINALLY(se2_neighs_destroy, &neigh_list);

  PYIGRAPH_CHECK(speak_easy_2(&neigh_list, &opts, &memb));
  se2_neighs_destroy(&neigh_list);
  IGRAPH_FINALLY_CLEAN(1);

  IGRAPH_FINALLY(igraph_matrix_int_destroy, &memb);

  py_memb_obj = igraph_matrix_int_to_py_list_i(&memb);
  if (!py_memb_obj && PyErr_Occurred()) {
    IGRAPH_FINALLY_FREE();
    return NULL;
  }
  igraph_matrix_int_destroy(&memb);
  IGRAPH_FINALLY_CLEAN(1);

  return py_memb_obj;
}

static PyObject* knn_graph(PyObject* Py_UNUSED(dummy), PyObject* args)
{
  se2_init();

  PyObject* py_cols_obj;
  int k;
  int is_weighted;

  igraph_matrix_t cols_i;
  igraph_t graph_i;
  igraph_vector_t weights_i;

  PyObject* py_graph_obj;
  PyObject* py_weights_obj;
  PyObject* ret;

  if (!PyArg_ParseTuple(args, "Oip", &py_cols_obj, &k, &is_weighted)) {
    return NULL;
  }

  if (!PyArray_ISNUMBER((PyArrayObject*)py_cols_obj)) {
    PyErr_SetString(PyExc_ValueError, "Cols must be numeric.");
    return NULL;
  }

  if (PyArray_ISCOMPLEX((PyArrayObject*)py_cols_obj)) {
    PyErr_SetString(PyExc_ValueError, "Cols must be real not complex.");
    return NULL;
  }

  PYIGRAPH_CHECK(
    ndarray_to_igraph_matrix_i((PyArrayObject*)py_cols_obj, &cols_i));
  IGRAPH_FINALLY(igraph_matrix_destroy, &cols_i);

  PYIGRAPH_CHECK(
    se2_knn_graph(&cols_i, k, &graph_i, is_weighted ? &weights_i : NULL));
  IGRAPH_FINALLY(igraph_destroy, &graph_i);

  if (is_weighted) {
    IGRAPH_FINALLY(igraph_vector_destroy, &weights_i);
  }

  igraph_matrix_destroy(&cols_i);
  IGRAPH_FINALLY_CLEAN(1);

  py_graph_obj = PyIGraph_FromCGraph(&graph_i);
  py_weights_obj = igraph_vector_to_py_list_i(is_weighted ? &weights_i : NULL);
  if (!py_weights_obj && PyErr_Occurred()) {
    IGRAPH_FINALLY_FREE();
    return NULL;
  }

  if (is_weighted) {
    igraph_vector_destroy(&weights_i);
    IGRAPH_FINALLY_CLEAN(1);
  }

  ret = PyTuple_New(2);
  if ((PyTuple_SetItem(ret, 0, py_graph_obj) < 0)) {
    return NULL;
  };

  if ((PyTuple_SetItem(ret, 1, py_weights_obj) < 0)) {
    return NULL;
  };

  return ret;
}

static PyObject* order_nodes(
  PyObject* Py_UNUSED(dummy), PyObject* args, PyObject* kwds)
{
  se2_init();

  PyObject* py_graph_obj = NULL;
  PyObject* py_weights_obj = NULL;
  PyObject* py_memb_obj = NULL;
  char* kwlist[] = { "graph", "membership", "weights", NULL };
  igraph_t* graph;
  igraph_vector_t weights;
  se2_neighs neigh_list;
  igraph_matrix_int_t memb, order;
  PyObject* py_order_obj;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kwlist, &py_graph_obj,
        &py_memb_obj, &py_weights_obj)) {
    return NULL;
  }

  graph = PyIGraph_ToCGraph(py_graph_obj);
  PYIGRAPH_CHECK(py_list_to_igraph_matrix_int_i(py_memb_obj, &memb));
  IGRAPH_FINALLY(igraph_matrix_int_destroy, &memb);

  if (py_weights_obj && PySequence_Check(py_weights_obj)) {
    PYIGRAPH_CHECK(py_sequence_to_igraph_vector_i(py_weights_obj, &weights));
    IGRAPH_FINALLY(igraph_vector_destroy, &weights);
    if (igraph_vector_size(&weights) != igraph_ecount(graph)) {
      IGRAPH_FINALLY_FREE();
      PyErr_SetString(PyExc_ValueError,
        "Number of weights does not match number of edges in graph.");
      return NULL;
    }
    PYIGRAPH_CHECK(se2_igraph_to_neighbor_list(graph, &weights, &neigh_list));
    igraph_vector_destroy(&weights);
    IGRAPH_FINALLY_CLEAN(1);
  } else {
    PYIGRAPH_CHECK(se2_igraph_to_neighbor_list(graph, NULL, &neigh_list));
  }
  IGRAPH_FINALLY(se2_neighs_destroy, &neigh_list);

  PYIGRAPH_CHECK(se2_order_nodes(&neigh_list, &memb, &order));
  IGRAPH_FINALLY(igraph_matrix_int_destroy, &order);
  py_order_obj = igraph_matrix_int_to_py_list_i(&order);
  if (!py_order_obj && PyErr_Occurred()) {
    IGRAPH_FINALLY_FREE();
    return NULL;
  }

  igraph_matrix_int_destroy(&memb);
  igraph_matrix_int_destroy(&order);
  IGRAPH_FINALLY_CLEAN(2);

  return py_order_obj;
}

static PyMethodDef SpeakEasy2Methods[] = {
  { "cluster", (PyCFunction)(void (*)(void))cluster,
    METH_VARARGS | METH_KEYWORDS, NULL },
  { "knn_graph", knn_graph, METH_VARARGS, NULL },
  { "order_nodes", (PyCFunction)(void (*)(void))order_nodes,
    METH_VARARGS | METH_KEYWORDS, NULL },
  { NULL, NULL, 0, NULL }
};

static struct PyModuleDef speakeasy2_module = {
  .m_base = PyModuleDef_HEAD_INIT,
  .m_name = "_speakeasy2",
  .m_size = -1,
  .m_methods = SpeakEasy2Methods
};

PyMODINIT_FUNC PyInit__speakeasy2(void)
{
  PyObject* m = PyModule_Create(&speakeasy2_module);
  if (m == NULL) {
    return NULL;
  }

  if (import_igraph() < 0) {
    Py_DECREF(m);
    return NULL;
  }

  if (PyArray_ImportNumPyAPI() < 0) {
    Py_DECREF(m);
    return NULL;
  }

  return m;
}
