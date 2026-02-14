#include <gtest/gtest.h>
#include <cmath>
#include "../src/ecs/TransformComponent.hpp"
#include "../src/ecs/TransformSystem.hpp"

constexpr float EPSILON = 1e-4f;

bool nearEqual(float a, float b, float eps = EPSILON) {
    return std::abs(a - b) < eps;
}

// ============================================================================
// TransformComponent Tests
// ============================================================================

TEST(TransformComponentTest, DefaultInitialization) {
    TransformComponent t;
    EXPECT_FLOAT_EQ(t.position.x, 0.0f);
    EXPECT_FLOAT_EQ(t.position.y, 0.0f);
    EXPECT_FLOAT_EQ(t.position.z, 0.0f);
    EXPECT_FLOAT_EQ(t.position.zoom, 1.0f);
    EXPECT_FLOAT_EQ(t.position.xAngle, 0.0f);
    EXPECT_FLOAT_EQ(t.position.yAngle, 0.0f);
    EXPECT_FLOAT_EQ(t.position.zAngle, 0.0f);
    EXPECT_FALSE(t.orbit.enabled);
}

TEST(TransformComponentTest, ModelMatrixDefaultIsIdentity) {
    TransformComponent t;
    // Identity matrix: diagonal is 1, rest is 0
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_FLOAT_EQ(t.modelMatrix.data[i][j], (i == j) ? 1.0f : 0.0f);
}

// ============================================================================
// TransformSystem Tests
// ============================================================================

TEST(TransformSystemTest, UpdateTransformAtOrigin) {
    TransformComponent t;
    TransformSystem::updateTransform(t);
    // Position at origin, zoom 1, no rotation => identity
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_FLOAT_EQ(t.modelMatrix.data[i][j], (i == j) ? 1.0f : 0.0f);
}

TEST(TransformSystemTest, UpdateTransformTranslation) {
    TransformComponent t;
    t.position.x = 10.0f;
    t.position.y = 20.0f;
    t.position.z = 30.0f;
    TransformSystem::updateTransform(t);

    // Transform the origin point
    slib::vec4 origin(slib::vec3{0, 0, 0}, 1.0f);
    slib::vec4 result = t.modelMatrix * origin;
    EXPECT_TRUE(nearEqual(result.x, 10.0f));
    EXPECT_TRUE(nearEqual(result.y, 20.0f));
    EXPECT_TRUE(nearEqual(result.z, 30.0f));
}

TEST(TransformSystemTest, UpdateTransformScale) {
    TransformComponent t;
    t.position.zoom = 2.0f;
    TransformSystem::updateTransform(t);

    slib::vec4 point(slib::vec3{1, 1, 1}, 1.0f);
    slib::vec4 result = t.modelMatrix * point;
    EXPECT_TRUE(nearEqual(result.x, 2.0f));
    EXPECT_TRUE(nearEqual(result.y, 2.0f));
    EXPECT_TRUE(nearEqual(result.z, 2.0f));
}

TEST(TransformSystemTest, IncAngles) {
    TransformComponent t;
    TransformSystem::incAngles(t, 1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(t.position.xAngle, 1.0f);
    EXPECT_FLOAT_EQ(t.position.yAngle, 2.0f);
    EXPECT_FLOAT_EQ(t.position.zAngle, 3.0f);

    TransformSystem::incAngles(t, 0.5f, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(t.position.xAngle, 1.5f);
    EXPECT_FLOAT_EQ(t.position.yAngle, 2.5f);
    EXPECT_FLOAT_EQ(t.position.zAngle, 3.5f);
}

TEST(TransformSystemTest, GetWorldCenterAtOrigin) {
    TransformComponent t;
    TransformSystem::updateTransform(t);
    slib::vec3 center = TransformSystem::getWorldCenter(t);
    EXPECT_TRUE(nearEqual(center.x, 0.0f));
    EXPECT_TRUE(nearEqual(center.y, 0.0f));
    EXPECT_TRUE(nearEqual(center.z, 0.0f));
}

TEST(TransformSystemTest, GetWorldCenterTranslated) {
    TransformComponent t;
    t.position.x = 100.0f;
    TransformSystem::updateTransform(t);
    slib::vec3 center = TransformSystem::getWorldCenter(t);
    EXPECT_TRUE(nearEqual(center.x, 100.0f));
    EXPECT_TRUE(nearEqual(center.y, 0.0f));
    EXPECT_TRUE(nearEqual(center.z, 0.0f));
}

TEST(TransformSystemTest, ScaleToRadius) {
    TransformComponent t;
    t.position.zoom = 1.0f;
    TransformSystem::scaleToRadius(t, 5.0f, 10.0f);
    EXPECT_FLOAT_EQ(t.position.zoom, 2.0f);
}

TEST(TransformSystemTest, ScaleToRadiusZeroBounding) {
    TransformComponent t;
    t.position.zoom = 1.0f;
    TransformSystem::scaleToRadius(t, 0.0f, 10.0f);
    // Should not change zoom when bounding radius is 0
    EXPECT_FLOAT_EQ(t.position.zoom, 1.0f);
}

TEST(TransformSystemTest, OrbitEnableDisable) {
    TransformComponent t;
    EXPECT_FALSE(t.orbit.enabled);

    TransformSystem::enableCircularOrbit(t, {0, 0, 0}, 10.0f, {0, 1, 0}, 1.0f);
    EXPECT_TRUE(t.orbit.enabled);
    EXPECT_FLOAT_EQ(t.orbit.radius, 10.0f);

    TransformSystem::disableCircularOrbit(t);
    EXPECT_FALSE(t.orbit.enabled);
}

TEST(TransformSystemTest, UpdateOrbitModifiesPosition) {
    TransformComponent t;
    TransformSystem::enableCircularOrbit(t, {0, 0, 0}, 10.0f, {0, 1, 0}, 1.0f, 0.0f);

    // At phase 0, position should be on the orbit circle
    TransformSystem::updateOrbit(t, 0.0f);
    float dist = std::sqrt(t.position.x * t.position.x +
                           t.position.y * t.position.y +
                           t.position.z * t.position.z);
    EXPECT_TRUE(nearEqual(dist, 10.0f, 0.1f));
}

TEST(TransformSystemTest, UpdateOrbitDisabledNoOp) {
    TransformComponent t;
    t.position.x = 5.0f;
    t.position.y = 5.0f;
    t.position.z = 5.0f;
    // Orbit not enabled
    TransformSystem::updateOrbit(t, 1.0f);
    // Position should be unchanged
    EXPECT_FLOAT_EQ(t.position.x, 5.0f);
    EXPECT_FLOAT_EQ(t.position.y, 5.0f);
    EXPECT_FLOAT_EQ(t.position.z, 5.0f);
}

TEST(TransformSystemTest, NormalMatrixIsRotationOnly) {
    TransformComponent t;
    t.position.x = 100.0f;
    t.position.zoom = 5.0f;
    t.position.xAngle = 45.0f;
    TransformSystem::updateTransform(t);

    // normalMatrix should not include translation or scale
    // Transform a direction vector: (0,0,0,0) homogeneous
    slib::vec4 dir(slib::vec3{0, 0, 0}, 0.0f);
    slib::vec4 result = t.normalMatrix * dir;
    EXPECT_TRUE(nearEqual(result.x, 0.0f));
    EXPECT_TRUE(nearEqual(result.y, 0.0f));
    EXPECT_TRUE(nearEqual(result.z, 0.0f));

    // A unit vector should remain unit length after rotation
    slib::vec4 unitY(slib::vec3{0, 1, 0}, 0.0f);
    slib::vec4 rotated = t.normalMatrix * unitY;
    float len = std::sqrt(rotated.x * rotated.x + rotated.y * rotated.y + rotated.z * rotated.z);
    EXPECT_TRUE(nearEqual(len, 1.0f));
}
