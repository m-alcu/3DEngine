#include <gtest/gtest.h>
#include "../src/ecs/Entity.hpp"
#include "../src/ecs/ComponentStore.hpp"
#include "../src/ecs/Registry.hpp"
#include "../src/ecs/TransformComponent.hpp"
#include "../src/ecs/TransformSystem.hpp"
#include "../src/ecs/LightComponent.hpp"
#include "../src/ecs/LightSystem.hpp"

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

    store.add(1, t);
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

    store.add(1, t);
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
    store.add(1, t1);
    EXPECT_EQ(store.size(), 1u);
    store.add(2, t2);
    store.add(3, t3);
    EXPECT_EQ(store.size(), 3u);
    store.remove(2);
    EXPECT_EQ(store.size(), 2u);
}

TEST(ComponentStoreTest, Clear) {
    ComponentStore<TransformComponent> store;
    TransformComponent t1, t2;

    store.add(1, t1);
    store.add(2, t2);
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

    store.add(10, t1);
    store.add(20, t2);
    store.add(30, t3);

    float sum = 0.0f;
    for (auto& [entity, transform] : store) {
        sum += transform.position.x;
    }
    EXPECT_FLOAT_EQ(sum, 6.0f);
}

TEST(ComponentStoreTest, MutationThroughPointer) {
    ComponentStore<TransformComponent> store;
    TransformComponent t;
    t.position.x = 0.0f;

    store.add(1, t);
    store.get(1)->position.x = 99.0f;

    // Store owns data — mutation changes the store's copy
    EXPECT_FLOAT_EQ(store.get(1)->position.x, 99.0f);
    // Original is NOT modified (value was copied into store)
    EXPECT_FLOAT_EQ(t.position.x, 0.0f);
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
    reg.transforms().add(e, t);

    EXPECT_TRUE(reg.transforms().has(e));
    EXPECT_FLOAT_EQ(reg.transforms().get(e)->position.x, 10.0f);
}

TEST(RegistryTest, DestroyEntityRemovesFromAllStores) {
    Registry reg;
    Entity e = reg.createEntity();

    TransformComponent t;
    LightComponent lc;
    reg.transforms().add(e, t);
    reg.lights().add(e, lc);
    EXPECT_TRUE(reg.transforms().has(e));
    EXPECT_TRUE(reg.lights().has(e));

    reg.destroyEntity(e);
    EXPECT_FALSE(reg.transforms().has(e));
    EXPECT_FALSE(reg.lights().has(e));
}

TEST(RegistryTest, ClearRemovesAllComponents) {
    Registry reg;
    TransformComponent t1, t2;
    LightComponent lc;

    Entity e1 = reg.createEntity();
    Entity e2 = reg.createEntity();
    reg.transforms().add(e1, t1);
    reg.transforms().add(e2, t2);
    reg.lights().add(e1, lc);

    EXPECT_EQ(reg.transforms().size(), 2u);
    EXPECT_EQ(reg.lights().size(), 1u);
    reg.clear();
    EXPECT_EQ(reg.transforms().size(), 0u);
    EXPECT_EQ(reg.lights().size(), 0u);
}

TEST(RegistryTest, SystemIterationPattern) {
    Registry reg;
    TransformComponent transforms[3];
    transforms[0].position.x = 100.0f;
    transforms[1].position.x = 200.0f;
    transforms[2].position.x = 300.0f;

    for (int i = 0; i < 3; ++i) {
        Entity e = reg.createEntity();
        reg.transforms().add(e, transforms[i]);
    }

    // Simulate a system iterating all transforms
    float sum = 0.0f;
    for (auto& [entity, transform] : reg.transforms()) {
        sum += transform.position.x;
    }
    EXPECT_FLOAT_EQ(sum, 600.0f);
}

// ============================================================================
// LightComponent Store Tests
// ============================================================================

TEST(RegistryTest, LightStoreAddAndGet) {
    Registry reg;
    Entity e = reg.createEntity();

    LightComponent lc;
    lc.light.intensity = 5.0f;
    reg.lights().add(e, lc);

    EXPECT_TRUE(reg.lights().has(e));
    ASSERT_NE(reg.lights().get(e), nullptr);
    EXPECT_FLOAT_EQ(reg.lights().get(e)->light.intensity, 5.0f);
}

TEST(RegistryTest, LightStoreOnlyForLightEntities) {
    Registry reg;
    Entity lightEntity = reg.createEntity();
    Entity nonLightEntity = reg.createEntity();

    TransformComponent t1, t2;
    LightComponent lc;
    reg.transforms().add(lightEntity, t1);
    reg.transforms().add(nonLightEntity, t2);
    reg.lights().add(lightEntity, lc);

    // Both have transforms
    EXPECT_TRUE(reg.transforms().has(lightEntity));
    EXPECT_TRUE(reg.transforms().has(nonLightEntity));

    // Only light entity has LightComponent
    EXPECT_TRUE(reg.lights().has(lightEntity));
    EXPECT_FALSE(reg.lights().has(nonLightEntity));
}

TEST(RegistryTest, LightStoreIteration) {
    Registry reg;
    LightComponent lc1, lc2;
    lc1.light.intensity = 10.0f;
    lc2.light.intensity = 20.0f;

    Entity e1 = reg.createEntity();
    Entity e2 = reg.createEntity();
    Entity e3 = reg.createEntity(); // non-light entity

    reg.lights().add(e1, lc1);
    reg.lights().add(e2, lc2);

    float sum = 0.0f;
    int count = 0;
    for (auto& [entity, lightComp] : reg.lights()) {
        sum += lightComp.light.intensity;
        count++;
    }
    EXPECT_EQ(count, 2);
    EXPECT_FLOAT_EQ(sum, 30.0f);
}

TEST(RegistryTest, LightStoreMutationThroughPointer) {
    Registry reg;
    Entity e = reg.createEntity();

    LightComponent lc;
    lc.light.intensity = 1.0f;
    reg.lights().add(e, lc);

    // Modify through registry pointer
    reg.lights().get(e)->light.intensity = 42.0f;

    // Store owns data — mutation changes the store's copy
    EXPECT_FLOAT_EQ(reg.lights().get(e)->light.intensity, 42.0f);
    // Original is NOT modified (value was copied into store)
    EXPECT_FLOAT_EQ(lc.light.intensity, 1.0f);
}

// ============================================================================
// System Tests — TransformSystem batch functions
// ============================================================================

TEST(TransformSystemTest, UpdateAllTransforms) {
    ComponentStore<TransformComponent> store;
    TransformComponent t1, t2;
    t1.position.x = 10.0f;
    t1.position.zoom = 2.0f;
    t2.position.y = 20.0f;

    store.add(1, t1);
    store.add(2, t2);

    TransformSystem::updateAllTransforms(store);

    // Both transforms should have non-identity modelMatrices
    auto* r1 = store.get(1);
    auto* r2 = store.get(2);
    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);

    // t1: translated to x=10, scaled by 2 — translation lives in row 0, col 3
    EXPECT_FLOAT_EQ(r1->modelMatrix.data[0][3], 10.0f);
    // t2: translated to y=20 — translation lives in row 1, col 3
    EXPECT_FLOAT_EQ(r2->modelMatrix.data[1][3], 20.0f);
}

TEST(TransformSystemTest, UpdateAllOrbits) {
    ComponentStore<TransformComponent> store;

    // Orbiting transform
    TransformComponent orbiting;
    orbiting.position.x = 0.0f;
    TransformSystem::enableCircularOrbit(orbiting, {0, 0, 0}, 100.0f, {0, 1, 0}, 1.0f, 0.0f);
    store.add(1, orbiting);

    // Non-orbiting transform
    TransformComponent stationary;
    stationary.position.x = 42.0f;
    store.add(2, stationary);

    TransformSystem::updateAllOrbits(store, 0.1f);

    // Orbiting one should have moved
    auto* o = store.get(1);
    ASSERT_NE(o, nullptr);
    EXPECT_TRUE(o->orbit.enabled);
    // Phase should have advanced from 0
    EXPECT_GT(o->orbit.phase, 0.0f);

    // Stationary one should be unchanged
    auto* s = store.get(2);
    ASSERT_NE(s, nullptr);
    EXPECT_FALSE(s->orbit.enabled);
    EXPECT_FLOAT_EQ(s->position.x, 42.0f);
}

// ============================================================================
// System Tests — LightSystem
// ============================================================================

TEST(LightSystemTest, SyncPositions) {
    Registry reg;
    Entity e = reg.createEntity();

    TransformComponent t;
    t.position.x = 100.0f;
    t.position.y = 200.0f;
    t.position.z = 300.0f;
    reg.transforms().add(e, t);

    LightComponent lc;
    lc.light.position = {0.0f, 0.0f, 0.0f};
    reg.lights().add(e, lc);

    LightSystem::syncPositions(reg);

    auto* light = reg.lights().get(e);
    ASSERT_NE(light, nullptr);
    EXPECT_FLOAT_EQ(light->light.position.x, 100.0f);
    EXPECT_FLOAT_EQ(light->light.position.y, 200.0f);
    EXPECT_FLOAT_EQ(light->light.position.z, 300.0f);
}

TEST(LightSystemTest, SyncPositionsMultipleLights) {
    Registry reg;
    Entity e1 = reg.createEntity();
    Entity e2 = reg.createEntity();

    TransformComponent t1;
    t1.position.x = 10.0f;
    reg.transforms().add(e1, t1);

    TransformComponent t2;
    t2.position.x = 20.0f;
    reg.transforms().add(e2, t2);

    LightComponent lc1, lc2;
    reg.lights().add(e1, lc1);
    reg.lights().add(e2, lc2);

    LightSystem::syncPositions(reg);

    EXPECT_FLOAT_EQ(reg.lights().get(e1)->light.position.x, 10.0f);
    EXPECT_FLOAT_EQ(reg.lights().get(e2)->light.position.x, 20.0f);
}

TEST(LightSystemTest, EnsureShadowMaps) {
    ComponentStore<LightComponent> store;
    LightComponent lc1, lc2;
    store.add(1, lc1);
    store.add(2, lc2);

    // Neither should have a shadow map yet
    EXPECT_EQ(store.get(1)->shadowMap, nullptr);
    EXPECT_EQ(store.get(2)->shadowMap, nullptr);

    LightSystem::ensureShadowMaps(store, 0);

    // Both should now have shadow maps
    EXPECT_NE(store.get(1)->shadowMap, nullptr);
    EXPECT_NE(store.get(2)->shadowMap, nullptr);
}

TEST(LightSystemTest, EnsureShadowMapsIdempotent) {
    ComponentStore<LightComponent> store;
    LightComponent lc;
    store.add(1, lc);

    LightSystem::ensureShadowMaps(store, 0);

    auto* firstMap = store.get(1)->shadowMap.get();
    ASSERT_NE(firstMap, nullptr);

    // Calling again should not create a new map
    LightSystem::ensureShadowMaps(store, 0);
    EXPECT_EQ(store.get(1)->shadowMap.get(), firstMap);
}
