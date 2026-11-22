# predictive-treap

A learning-augmented treap with online access predictors, benchmarked against splay trees, counting trees, and cached B-trees.

**Status:** early development. No results yet. This README describes the design and the planned experiments.

## What it is

A treap is a binary search tree where each node also carries a priority, and higher-priority nodes sit closer to the root. In a normal treap the priorities are random.

In this project, priorities come from a predictor. The predictor estimates when each key will be accessed next, and keys that are due soon are rotated toward the root before the access happens. Lookups stay read-only. All restructuring happens in a background batch step.

## Why

- Balanced trees (red-black, AVL, B-trees) treat every key the same, no matter how often it is used.
- Splay trees adapt, but only after an access, and they rewrite the tree on every lookup. That dirties cache lines and blocks concurrent readers.
- A hash-map cache in front of an index handles hot exact-key lookups well, but it cannot answer ordered queries such as "nearest key at or after X" or short range scans.

This project targets that gap: skewed workloads where the hot queries are ordered.

## Research question

When access patterns have structure (skew, bursts, periodic cycles), does a cheap online predictor, with its own cost included, let a treap beat reactive trees on ordered queries? And how accurate does the predictor need to be before it pays off?

## Prior work

This project implements and extends ideas from:

- Lin, Luo, Woodruff. *Learning-Augmented Binary Search Trees.* ICML 2022.
- Chen, Cao, Stepin, Chen. *On the Power of Learning-Augmented Search Trees.* ICML 2025.
- Afek, Kaplan, Korenfeld, Morrison, Tarjan. *CBTree: A Practical Concurrent Self-Adjusting Search Tree.* DISC 2012.
- Sleator, Tarjan. *Self-Adjusting Binary Search Trees.* JACM 1985.

The papers above analyze trees that are given predictions. The contribution here is practical: build the predictors, charge their cost to the tree, and measure the full system.

## Design

Five layers. Each depends only on the one below it.

| Layer | Contents |
|---|---|
| Offline tools (Python) | Trace generator, analysis, plots |
| Harness (C++) | Trace loader, runner, metrics |
| Index | Predictive treap and baselines behind one `OrderedIndex` interface |
| Policy | Predictor, adapt policy, stats policy, all chosen at compile time |
| Memory | Node arena, predictor table, access log |

Key decisions:

- **Array-backed nodes.** All nodes live in one contiguous array and are linked by 32-bit index, not pointers.
- **Read-only lookups.** A lookup only appends a sampled entry to a small ring buffer.
- **Batched adaptation.** Every N accesses, a batch step updates the predictor and re-ranks the keys that are due soon, with a cap on rotations.
- **Log-scale ranks.** A key's rank is based on the log of its predicted time until next access, so a key only moves when that estimate roughly doubles or halves.
- **Pluggable predictors.** Oracle (reads the future, used as an upper bound), frequency, recency, and periodic.
- **Safe fallback.** Random tie-breaking keeps expected depth at O(log n) even when predictions are wrong.

## Baselines

- Treap with random priorities
- Splay tree
- Counting-based self-adjusting tree
- B-tree
- B-tree with a hash-map cache in front

## Workloads

| Workload | Purpose |
|---|---|
| Uniform | Check that prediction does no harm when there is nothing to predict |
| Zipf | Static skew |
| Periodic | Keys that recur on cycles, with adjustable jitter |
| Shifting hot set | Speed of adaptation |
| Broken pattern | Cost of wrong predictions |
| Real trace | Transit arrival data, queried as "next arrival at or after time t" |

Operation mix: mostly `floor` and `ceiling`, plus short range scans and some exact lookups.

## Metrics

- Nodes visited per lookup
- Rotations per operation (background cost)
- Prediction error, reported separately from tree performance
- Nanoseconds per operation
- Cache misses, via `perf stat` on Linux

## Tech stack

- C++20, CMake, GoogleTest, Google Benchmark
- AddressSanitizer and UndefinedBehaviorSanitizer in tests
- Python (numpy, pandas, matplotlib) for traces and plots
- GitHub Actions for tests

## Build

```
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

Requires CMake 3.21 or newer and a C++20 compiler.

## Roadmap

- [ ] Phase 0: project setup
- [ ] Phase 1: feasibility test (treap, splay, oracle predictor, nodes visited on `floor` queries)
- [ ] Phase 2: access log, batch adapter, online predictors
- [ ] Phase 3: all baselines, wall-clock and cache measurements
- [ ] Phase 4: results and write-up
- [ ] Phase 5 (optional): concurrent readers

## Testing

Every tree is tested against `std::set` with randomized operations, and invariant checkers confirm key order and heap order. All tests run under the sanitizers.

## License

MIT
