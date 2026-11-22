#ifndef PT_MEMORY_NODE_ARENA_H_
#define PT_MEMORY_NODE_ARENA_H_

#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace pt {

// Contiguous node storage linked by 32-bit index. No iterators, allocator
// parameter, or thread safety. Index bounds and the 2^32-2 node limit are
// checked by assert only; in NDEBUG builds violations are undefined behavior.
// Double-free and access to a freed slot are not detected.
template <typename T>
class NodeArena {
  static_assert(std::is_trivially_destructible_v<T>,
                "tree nodes must be plain data");
  static_assert(std::is_move_assignable_v<T>,
                "freed slots are reused by assignment");

 public:
  using Index = std::uint32_t;

  // The "no node" link value; never returned by Allocate.
  static constexpr Index kNull = std::numeric_limits<Index>::max();

  explicit NodeArena(Index reserve = 0) { nodes_.reserve(reserve); }

  // Reuses the most recently freed slot, otherwise appends.
  template <typename... Args>
  Index Allocate(Args&&... args) {
    if (!free_.empty()) {
      Index i = free_.back();
      free_.pop_back();
      nodes_[i] = T(std::forward<Args>(args)...);
      return i;
    }
    assert(nodes_.size() < kNull && "NodeArena index space exhausted");
    nodes_.emplace_back(std::forward<Args>(args)...);
    return static_cast<Index>(nodes_.size() - 1);
  }

  void Free(Index i) {
    assert(i < nodes_.size());
    free_.push_back(i);
  }

  T& operator[](Index i) {
    assert(i < nodes_.size());
    return nodes_[i];
  }

  const T& operator[](Index i) const {
    assert(i < nodes_.size());
    return nodes_[i];
  }

  // Number of live nodes.
  Index size() const {
    return static_cast<Index>(nodes_.size() - free_.size());
  }

  void Clear() {
    nodes_.clear();
    free_.clear();
  }

 private:
  std::vector<T> nodes_;
  std::vector<Index> free_;
};

}  // namespace pt

#endif  // PT_MEMORY_NODE_ARENA_H_
