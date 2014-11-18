INNCABS
=======
The Innsbruck C++11 Async Benchmark Suite

Abstract
--------
INNCABS is a cross-platform cross-library benchmark suite consisting of 14 benchmarks with varying task granularities and synchronization requirements. It is designed to test the quality of implementation of C++11 parallelism constructs, in particular of std::async() and the deadlock-free multi-mutex std::lock() call.


Applications
------------

* **Alignment**:
This benchmark aligns all protein sequences from an input file against every other sequence, using the algorithm introduced by Myers et al. The result is a scoring of each pair, which is determined using a dynamic programming algorithm. The parallel structure is loop-like, without global synchronization requirements and relatively large granularity.

* **FFT**:
Calculates the Fast Fourier Transform of an array of N complex numbers using a divide-and-conquer scheme based on the Cooley-Tukey algorithm. The implementation is recursive in nature, and generates a binary task tree, without any synchronization requirements beyond joining these two asynchronous tasks in each node.

* **Fib**:
Represents a focused test of task creation and invocation overheads, by calculating a given number N in the Fibonacci series. This benchmark features the finest granularity of tasks within INNCABS, and has a binary recursive tree structure. While not a realistic test case, it provides a good measure for the capability of an implementation to deal with very fine-grained tasks in terms of overheads.

* **Floorplan**:
This benchmark implements a 2D packing problem. For a number of cells described in an input file, the arrangement of the smallest possible 2D grid for placing them is found using a branch-and-bound algorithm. This results in a recursively task-parallel formulation, with loop-like parallelism in each of its nodes. Additionally, the generated tree structure is highly imbalanced due to aggressive early atomics-based pruning of non-viable paths. Individual task granularity varies depending on the number of viable cell placements, but is generally rather fine-grained.

* **Health**:
Simulates a health care system, based on a number of villages, hospitals, and patients. Details are described by Duran et al. In this benchmark, asynchronous calls are generated in a loop-like fashion, and after each step global synchronization is performed. Task granularity is variable, and moderate on average.

* **Intersim**
Executes a network of interaction combinators, as specified by Lafont. Interaction combinators are a simple, universal model for distributed computation, implemented in this benchmark as a sea of asynchronous co-dependent tasks. The evaluation of each combinator is asynchronous, and requires two to six shared mutexes, which are acquired using the variadic std::lock function call. Each evaluation may also spawn new asynchronous function calls. The task granularity is fine, and the test is a good benchmark for both task management and multiple lock acquisition overhead.

* **NQueens**:
Finds the solution to the N-Queens problem for a given N. This recursive implementation generates a variable arity (up to N-1) asynchronous invocation tree, with small to moderate computational effort in each node and no synchronization beyond what is prescribed by the tree structure.

* **Pyramids**:
A task-based 2D stencil solver using the cache-oblivious algorithm presented by Frigo and Strumpen. We included this benchmark to represent an important category of cache-oblivious numeric divide-and-conquer algorithms, since it has more significant computation bounds than most other tests. It features a regular recursive invocation tree with an arity of 4, and performs computation on the leaves. There is no global synchronization, and the individual tasks are relatively coarse-grained.

* **QAP**:
Solves an instance of the quadratic assignment problem using a recursive branch-and-bound solver. QAP and Floorplan are unique within the INNCABS suite, as pruning and cancellation are performed for entire sub-trees using atomic test-and-set operations. Per-task computational effort is small.

* **Round**:
An implementation of the dining philosophers problem serving as a microbenchmark of the computational and scheduling overhead of the deadlock-free lock acquisition algorithm implemented by a given C++ standard library. It is based on a recent study by Hinnant. A given number N of asynchronous invocations are launched, and compete for the acquisition of two mutually shared locks each for a fixed period of time.

* **Sort**:
A recursive merge-sort algorithm based on the cilksort code included in the original Cilk distribution. It maps to a uniform invocation tree with an arity of 4, and requires no additional synchronization. The task granularity is variable, and can be adjusted by parameters specifying multiple cutoff points (one for limiting the generation of additional parallelism and one for switching to a purely sequential algorithm).

* **SparseLU**:
In this benchmark, adapted from the BOTS benchmark of the same name, the LU factorization of a sparse matrix is computed. It features a loop-like parallel structure, with no nested asynchronous invocations. The generated leaf nodes which perform the actual computation are very coarse-grained compared to all other INNCABS benchmarks.

* **Strassen**:
Implements efficient multiplication of large dense matrices using the Strassen algorithm. It is based on a recursive, uniform tree structure with an arity of 8 and moderate granularity.

* **UTS**:
A C++11 implementation of the Unbalanced Tree Search benchmark. It is designed to challenge the workload schedulers of task-parallel systems, and features a highly imbalanced tree of asynchronous invocations, with variable arity and per-node computational load.

