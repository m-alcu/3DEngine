#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <cmath>

class ShadowMap;

// solid color attribute not interpolated
class PhongEffect {
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
      vertex.world = modelMatrix * slib::vec4(vData.vertex, 1);
      vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
      vertex.normal = normalMatrix * slib::vec4(vData.normal, 0);
      Projection<Vertex>::view(scene->screen.width, scene->screen.height, vertex, true);
      return vertex;
    }
  };

  class GeometryShader {
  public:
    void operator()(Polygon<Vertex> &poly, int32_t width, int32_t height, const Scene &/*scene*/) const {
      for (auto &point : poly.points) {
        Projection<Vertex>::view(width, height, point, false);
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
      slib::vec3 normal = smath::normalize(vRaster.normal);
      slib::vec3 color = Ka;
      for (const auto &solidPtr : scene.solids) {
        if (!solidPtr->lightSourceEnabled) {
          continue;
        }
        const Light &light = solidPtr->light;
        slib::vec3 luxDirection = light.getDirection(vRaster.world);
        float diff = std::max(0.0f, smath::dot(normal, luxDirection));
        slib::vec3 R =
            normal * 2.0f * smath::dot(normal, luxDirection) - luxDirection;
        float specAngle =
            std::max(0.0f, smath::dot(R, scene.forwardNeg)); // viewer
        float spec = std::pow(specAngle, poly.material->Ns);
        float attenuation = light.getAttenuation(vRaster.world);
        float shadow = scene.shadowsEnabled && solidPtr->shadowMap ? solidPtr->shadowMap->sampleShadow(vRaster.world, diff) : 1.0f;
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += (Kd * diff + Ks * spec) * lightColor;
      }
      return Color(color).toBgra(); // assumes vec3 uses .r/g/b or [0]/[1]/[2]
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
