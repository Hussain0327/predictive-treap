#include "pt/memory/node_arena.h"

#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include "gtest/gtest.h"

namespace pt {
namespace {

struct Node {
  int key;
  std::uint32_t left;
  std::uint32_t right;
};

using Arena = NodeArena<Node>;

TEST(NodeArenaTest, FreshArenaIsEmpty) {
  Arena arena;
  EXPECT_EQ(arena.size(), 0u);
}

TEST(NodeArenaTest, AllocateReturnsDistinctIndicesAndValuesReadBack) {
  Arena arena;
  Arena::Index a = arena.Allocate(Node{1, Arena::kNull, Arena::kNull});
  Arena::Index b = arena.Allocate(Node{2, a, Arena::kNull});
  EXPECT_NE(a, b);
  EXPECT_EQ(arena[a].key, 1);
  EXPECT_EQ(arena[b].key, 2);
  EXPECT_EQ(arena[b].left, a);
  const Arena& carena = arena;
  EXPECT_EQ(carena[a].key, 1);
}

TEST(NodeArenaTest, IndicesAndValuesSurviveGrowth) {
  Arena arena(4);
  constexpr int kCount = 5000;
  std::vector<Arena::Index> ids;
  for (int i = 0; i < kCount; ++i) {
    ids.push_back(arena.Allocate(Node{i, Arena::kNull, Arena::kNull}));
  }
  EXPECT_EQ(std::set<Arena::Index>(ids.begin(), ids.end()).size(),
            static_cast<std::size_t>(kCount));
  for (int i = 0; i < kCount; ++i) {
    EXPECT_EQ(arena[ids[i]].key, i);
  }
}

TEST(NodeArenaTest, FreeThenAllocateReusesSlot) {
  Arena arena;
  Arena::Index a = arena.Allocate(Node{1, Arena::kNull, Arena::kNull});
  arena.Allocate(Node{2, Arena::kNull, Arena::kNull});
  arena.Free(a);
  Arena::Index c = arena.Allocate(Node{3, Arena::kNull, Arena::kNull});
  EXPECT_EQ(c, a);
  EXPECT_EQ(arena[c].key, 3);
}

TEST(NodeArenaTest, SizeCountsLiveNodes) {
  Arena arena;
  Arena::Index a = arena.Allocate(Node{});
  arena.Allocate(Node{});
  EXPECT_EQ(arena.size(), 2u);
  arena.Free(a);
  EXPECT_EQ(arena.size(), 1u);
  arena.Allocate(Node{});
  EXPECT_EQ(arena.size(), 2u);
  arena.Clear();
  EXPECT_EQ(arena.size(), 0u);
}

TEST(NodeArenaTest, FreedSlotsReusedLifoAndFullyOverwritten) {
  Arena arena;
  Arena::Index a = arena.Allocate(Node{1, 7, 8});
  Arena::Index b = arena.Allocate(Node{2, 9, 10});
  arena.Free(a);
  arena.Free(b);
  Arena::Index c = arena.Allocate(Node{3, Arena::kNull, Arena::kNull});
  EXPECT_EQ(c, b);
  EXPECT_EQ(arena[c].key, 3);
  EXPECT_EQ(arena[c].left, Arena::kNull);
  EXPECT_EQ(arena[c].right, Arena::kNull);
  EXPECT_EQ(arena.Allocate(Node{}), a);
}

TEST(NodeArenaTest, ClearDropsPendingFreeSlots) {
  Arena arena;
  arena.Allocate(Node{});
  arena.Allocate(Node{});
  arena.Free(0);
  arena.Clear();
  EXPECT_EQ(arena.size(), 0u);
  EXPECT_EQ(arena.Allocate(Node{}), 0u);
  EXPECT_EQ(arena.size(), 1u);
}

TEST(NodeArenaTest, AllocateFromOwnSlotSurvivesReallocation) {
  Arena arena(1);
  Arena::Index a = arena.Allocate(Node{42, 1, 2});
  for (int i = 0; i < 100; ++i) {
    Arena::Index c = arena.Allocate(arena[a]);
    EXPECT_EQ(arena[c].key, 42);
    EXPECT_EQ(arena[c].left, 1u);
    EXPECT_EQ(arena[c].right, 2u);
  }
  arena.Free(a);
  Arena::Index d = arena.Allocate(arena[5]);
  EXPECT_EQ(d, a);
  EXPECT_EQ(arena[d].key, 42);
}

}  // namespace
}  // namespace pt
