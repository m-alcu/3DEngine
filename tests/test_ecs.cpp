#include <gtest/gtest.h>
#include "../src/ecs/Entity.hpp"
#include "../src/ecs/ComponentStore.hpp"
#include "../src/ecs/Registry.hpp"
#include "../src/ecs/TransformComponent.hpp"

// ============================================================================
// Entity Tests
// ============================================================================

TEST(EntityTest, NullEntityIsZero) {
    EXPECT_EQ(NULL_ENTITY, 0u);
}

TEST(EntityTest, GeneratorProducesUniqueIds) {
    EntityGenerator gen;
    Entity e1 = gen.create();
    Entity e2 = gen.create();
    Entity e3 = gen.create();
    EXPECT_NE(e1, NULL_ENTITY);
    EXPECT_NE(e2, NULL_ENTITY);
    EXPECT_NE(e3, NULL_ENTITY);
    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_NE(e1, e3);
}

TEST(EntityTest, GeneratorStartsAtOne) {
    EntityGenerator gen;
    EXPECT_EQ(gen.create(), 1u);
    EXPECT_EQ(gen.create(), 2u);
    EXPECT_EQ(gen.create(), 3u);
}

// ============================================================================
// ComponentStore Tests
// ============================================================================

TEST(ComponentStoreTest, AddAndGet) {
    ComponentStore<TransformComponent> store;
    TransformComponent t;
    t.position.x = 42.0f;

    store.add(1, &t);
    TransformComponent* result = store.get(1);
    ASSERT_NE(result, nullptr);
    EXPECT_FLOAT_EQ(result->position.x, 42.0f);
}

TEST(ComponentStoreTest, GetNonExistent) {
    ComponentStore<TransformComponent> store;
    EXPECT_EQ(store.get(999), nullptr);
}

TEST(ComponentStoreTest, HasAndRemove) {
    ComponentStore<TransformComponent> store;
    TransformComponent t;

    store.add(1, &t);
    EXPECT_TRUE(store.has(1));
    EXPECT_FALSE(store.has(2));

    store.remove(1);
    EXPECT_FALSE(store.has(1));
    EXPECT_EQ(store.get(1), nullptr);
}

TEST(ComponentStoreTest, Size) {
    ComponentStore<TransformComponent> store;
    TransformComponent t1, t2, t3;

    EXPECT_EQ(store.size(), 0u);
    store.add(1, &t1);
    EXPECT_EQ(store.size(), 1u);
    store.add(2, &t2);
    store.add(3, &t3);
    EXPECT_EQ(store.size(), 3u);
    store.remove(2);
    EXPECT_EQ(store.size(), 2u);
}

TEST(ComponentStoreTest, Clear) {
    ComponentStore<TransformComponent> store;
    TransformComponent t1, t2;

    store.add(1, &t1);
    store.add(2, &t2);
    EXPECT_EQ(store.size(), 2u);

    store.clear();
    EXPECT_EQ(store.size(), 0u);
    EXPECT_FALSE(store.has(1));
    EXPECT_FALSE(store.has(2));
}

TEST(ComponentStoreTest, Iteration) {
    ComponentStore<TransformComponent> store;
    TransformComponent t1, t2, t3;
    t1.position.x = 1.0f;
    t2.position.x = 2.0f;
    t3.position.x = 3.0f;

    store.add(10, &t1);
    store.add(20, &t2);
    store.add(30, &t3);

    float sum = 0.0f;
    for (auto& [entity, transform] : store) {
        sum += transform->position.x;
    }
    EXPECT_FLOAT_EQ(sum, 6.0f);
}

TEST(ComponentStoreTest, MutationThroughPointer) {
    ComponentStore<TransformComponent> store;
    TransformComponent t;
    t.position.x = 0.0f;

    store.add(1, &t);
    store.get(1)->position.x = 99.0f;

    // Original is modified (non-owning pointer)
    EXPECT_FLOAT_EQ(t.position.x, 99.0f);
}

// ============================================================================
// Registry Tests
// ============================================================================

TEST(RegistryTest, CreateEntityProducesUniqueIds) {
    Registry reg;
    Entity e1 = reg.createEntity();
    Entity e2 = reg.createEntity();
    EXPECT_NE(e1, NULL_ENTITY);
    EXPECT_NE(e2, NULL_ENTITY);
    EXPECT_NE(e1, e2);
}

TEST(RegistryTest, TransformStoreIntegration) {
    Registry reg;
    Entity e = reg.createEntity();

    TransformComponent t;
    t.position.x = 10.0f;
    reg.transforms().add(e, &t);

    EXPECT_TRUE(reg.transforms().has(e));
    EXPECT_FLOAT_EQ(reg.transforms().get(e)->position.x, 10.0f);
}

TEST(RegistryTest, DestroyEntityRemovesFromAllStores) {
    Registry reg;
    Entity e = reg.createEntity();

    TransformComponent t;
    reg.transforms().add(e, &t);
    EXPECT_TRUE(reg.transforms().has(e));

    reg.destroyEntity(e);
    EXPECT_FALSE(reg.transforms().has(e));
}

TEST(RegistryTest, ClearRemovesAllComponents) {
    Registry reg;
    TransformComponent t1, t2;

    Entity e1 = reg.createEntity();
    Entity e2 = reg.createEntity();
    reg.transforms().add(e1, &t1);
    reg.transforms().add(e2, &t2);

    EXPECT_EQ(reg.transforms().size(), 2u);
    reg.clear();
    EXPECT_EQ(reg.transforms().size(), 0u);
}

TEST(RegistryTest, SystemIterationPattern) {
    Registry reg;
    TransformComponent transforms[3];
    transforms[0].position.x = 100.0f;
    transforms[1].position.x = 200.0f;
    transforms[2].position.x = 300.0f;

    for (int i = 0; i < 3; ++i) {
        Entity e = reg.createEntity();
        reg.transforms().add(e, &transforms[i]);
    }

    // Simulate a system iterating all transforms
    float sum = 0.0f;
    for (auto& [entity, transform] : reg.transforms()) {
        sum += transform->position.x;
    }
    EXPECT_FLOAT_EQ(sum, 600.0f);
}
