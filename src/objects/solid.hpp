#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include "../slib.hpp"
#include "../light.hpp"
#include "../material.hpp"
#include "../constants.hpp"
#include "../smath.hpp"
#include "../ShadowMap.hpp"

enum class Shading {
    Wireframe,
    Flat,
    Gouraud,
    BlinnPhong,
    Phong,
    TexturedFlat,
    TexturedGouraud,
    TexturedBlinnPhong,
    TexturedPhong
};

// Labels for the enum (must match order of enum values)
static const char* shadingNames[] = {
    "Wireframe",
    "Flat",
    "Gouraud",
    "Blinn-Phong",
    "Phong",
    "Textured Flat",
    "Textured Gouraud",
    "Textured Blinn-Phong",
    "Textured Phong"
};

struct VertexData {
    slib::vec3 vertex;
    slib::vec3 normal;
    slib::vec2 texCoord;
};

typedef struct Face
{
	std::vector<int> vertexIndices; // For wireframe rendering
    std::string materialKey;
} Face;

struct FaceData {
    Face face;
    slib::vec3 faceNormal;
};

typedef struct Position
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float zoom = 1.0f;
    float xAngle = 0.0f;
    float yAngle = 0.0f;
    float zAngle = 0.0f;
} Position;

enum class MaterialType {
    Rubber,
    Plastic,
    Wood,
    Marble,
    Glass,
    Metal,
    Mirror,
    Light
};

// Struct to hold k_s, k_a, and k_d values
struct MaterialProperties {
    float k_s; // Specular reflection coefficient
    float k_a; // Ambient reflection coefficient
    float k_d; // Diffuse reflection coefficient
    float shininess;
};

struct OrbitState {
    slib::vec3 center{ 0,0,0 };   // orbit center
    float radius = 1.0f;         // orbit radius
    slib::vec3 n{ 0,1,0 };         // plane normal (unit)
    float omega = 1.0f;          // angular speed (radians/sec)
    float phase = 0.0f;          // current angle (radians)
    bool enabled = false;
};

class Solid {
public:
    std::vector<VertexData> vertexData;
    std::vector<FaceData> faceData;
    Shading shading;
    Position position;
    slib::mat4 modelMatrix;
    slib::mat4 normalMatrix;
	bool rotationEnabled = true;
    float incXangle = 0.0f;  // Rotation speed around X axis
    float incYangle = 0.0f;  // Rotation speed around Y axis
    std::string name;
    std::map<std::string, Material> materials;

    int numVertices;
    int numFaces;
	bool lightSourceEnabled = false;
    Light light;  // Light properties when this solid is a light source
    std::shared_ptr<ShadowMap> shadowMap;  // Shadow map for this light source

    // Orthonormal basis of the orbit plane
    slib::vec3 orbitU{ 1,0,0 };
    slib::vec3 orbitV{ 0,0,1 };
    OrbitState orbit_;

    // Minimum and maximum coordinates for bounding box
    slib::vec3 minCoord{};
    slib::vec3 maxCoord{};
 
public:
    // Base constructor that initializes common data members.
    Solid();

    // Virtual destructor for proper cleanup in derived classes.
    virtual ~Solid() = default;

    // A common setup method that calls the helper functions.
    virtual void setup() {
        loadVertices();
        loadFaces();
        calculateFaceNormals();
        calculateVertexNormals();
        calculateMinMaxCoords();
    }

    virtual void calculateFaceNormals();

    virtual void calculateVertexNormals();

    virtual void calculateMinMaxCoords();

    float getBoundingRadius() const;

    slib::vec3 getWorldCenter() const;

    void updateWorldBounds(slib::vec3& minV, slib::vec3& maxV) const;

    void scaleToRadius(float targetRadius);

    void calculateTransformMat();

    virtual MaterialProperties getMaterialProperties(MaterialType type);

    virtual int getColorFromMaterial(const float color);

    Texture LoadTextureFromImg(const char* filename);

    virtual void incAngles(float xAngle, float yAngle, float zAngle);

    virtual void buildOrbitBasis(const slib::vec3& n);

    // Enable a circular orbit
    virtual void enableCircularOrbit(const slib::vec3& center,
        float radius,
        const slib::vec3& planeNormal,
        float angularSpeedRadiansPerSec,
        float initialPhaseRadians = 0.0f,
        bool faceCenter = false);

    virtual void disableCircularOrbit();

    virtual void updateOrbit(float dt);

    void setEmissiveColor(const slib::vec3& color);

protected:
    // Protected virtual methods to be implemented by derived classes.
    virtual void loadVertices() = 0;
    virtual void loadFaces() = 0;
	
};
