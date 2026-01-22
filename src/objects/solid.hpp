#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include "../slib.hpp"
#include "../material.hpp"
#include "../constants.hpp"

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
    float x;
    float y;
    float z;
    float zoom;
    float xAngle;
    float yAngle;
    float zAngle;    
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
	bool rotationEnabled = true;
    std::string name;
    std::map<std::string, Material> materials;

    int numVertices;
    int numFaces;
	bool lightSourceEnabled = false;

    // Orthonormal basis of the orbit plane
    slib::vec3 orbitU{ 1,0,0 };
    slib::vec3 orbitV{ 0,0,1 };
    OrbitState orbit_;

    // Minimum and maximum coordinates for bounding box
    slib::vec3 minCoord{};
    slib::vec3 maxCoord{};
 
public:
    // Base constructor that initializes common data members.
    Solid() 
    {
    }

    // Virtual destructor for proper cleanup in derived classes.
    virtual ~Solid() = default;

    // A common setup method that calls the helper functions.
    virtual void setup() {
        loadVertices();
        loadFaces();
        calculateNormals();
        calculateVertexNormals();
        calculateMinMaxCoords();
    }

    virtual void calculateNormals();

    virtual void calculateVertexNormals();

    virtual void calculateMinMaxCoords();

    virtual MaterialProperties getMaterialProperties(MaterialType type);

    virtual int getColorFromMaterial(const float color);

    Texture DecodePng(const char* filename);

    virtual void rotate(float xAngle, float yAngle, float zAngle);

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

protected:
    // Protected virtual methods to be implemented by derived classes.
    virtual void loadVertices() = 0;
    virtual void loadFaces() = 0;
	
};

