#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <cmath>

class ShadowMap;

// solid color attribute not interpolated
class BlinnPhongEffect {
public:
  // the vertex type that will be input into the pipeline
  class Vertex {
  public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp,
           slib::vec3 _world, bool _broken)
        : p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp), world(_world),
          broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
      return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal,
                    ndc + v.ndc, world + v.world, true);
    }

    Vertex operator-(const Vertex &v) const {
      return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal,
                    ndc - v.ndc, world - v.world, true);
    }

    Vertex operator*(const float &rhs) const {
      return Vertex(p_x * rhs, p_y, p_z * rhs, normal * rhs, ndc * rhs,
                    world * rhs, true);
    }

    Vertex &operator+=(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      normal += v.normal;
      world += v.world;
      ndc += v.ndc;
      return *this;
    }

    Vertex &vraster(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      normal += v.normal;
      world += v.world;
      return *this;
    }

    Vertex &hraster(const Vertex &v) {
      p_z += v.p_z;
      normal += v.normal;
      world += v.world;
      return *this;
    }

  public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
    slib::vec4 ndc;
    bool broken = false;
  };

  class VertexShader {
  public:
    Vertex operator()(const VertexData &vData,
                      const slib::mat4 &modelMatrix,
                      const slib::mat4 &normalMatrix,
                      const Scene *scene,
                      const ShadowMap */*shadowMap*/) const {
      Vertex vertex;
      Projection<Vertex> projection;
      vertex.world = modelMatrix * slib::vec4(vData.vertex, 1);
      vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
      vertex.normal = normalMatrix * slib::vec4(vData.normal, 0);
      projection.view(scene->screen.width, scene->screen.height, vertex, true);
      return vertex;
    }
  };

  class GeometryShader {
  public:
    void operator()(Polygon<Vertex> &poly, int32_t width, int32_t height, const Scene &scene) const {
      Projection<Vertex> projection;
      for (auto &point : poly.points) {
        projection.view(width, height, point, false);
      }
    }
  };

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      const auto &Ka = poly.material->Ka; // vec3
      const auto &Kd = poly.material->Kd; // vec3
      const auto &Ks = poly.material->Ks; // vec3
      const slib::vec3 &luxDirection = scene.light.getDirection(vRaster.world);
      // Normalize vectors
      slib::vec3 N = smath::normalize(vRaster.normal); // Normal at the fragment
      slib::vec3 L = luxDirection;                     // Light direction

      // Diffuse component
      float diff = std::max(0.0f, smath::dot(N, L));

      // Halfway vector H = normalize(L + V)
      const slib::vec3 &halfwayVector =
          smath::normalize(luxDirection - scene.camera.forward);

      // Specular component: spec = (N · H)^shininess
      float specAngle = std::max(0.0f, smath::dot(N, halfwayVector)); // viewer
      float spec = std::pow(
          specAngle,
          poly.material->Ns); // Blinn Phong shininess needs *4 to be like Phong

      // Shadow calculation
      float shadow = scene.shadowMap && scene.shadowsEnabled ? scene.shadowMap->sampleShadow(vRaster.world, diff) : 1.0f;

      // Shadow affects diffuse and specular, not ambient
      slib::vec3 color = Ka + (Kd * diff * scene.light.intensity + Ks * spec) * shadow;
      return Color(color).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};