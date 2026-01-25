#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <algorithm>

class ShadowMap;

// solid color attribute not interpolated
class GouraudEffect {
public:
  // the vertex type that will be input into the pipeline
  class Vertex {
  public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp,
           slib::vec3 _world, float _diffuse, bool _broken)
        : p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp), world(_world),
          diffuse(_diffuse), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
      return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal,
                    ndc + v.ndc, world + v.world, diffuse + v.diffuse, true);
    }

    Vertex operator-(const Vertex &v) const {
      return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal,
                    ndc - v.ndc, world - v.world, diffuse - v.diffuse, true);
    }

    Vertex operator*(const float &rhs) const {
      return Vertex(p_x * rhs, p_y, p_z * rhs, normal * rhs, ndc * rhs,
                    world * rhs, diffuse * rhs, true);
    }

    Vertex &operator+=(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      normal += v.normal;
      ndc += v.ndc;
      world += v.world;
      diffuse += v.diffuse;
      return *this;
    }

    Vertex &vraster(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      world += v.world;
      diffuse += v.diffuse;
      return *this;
    }

    Vertex &hraster(const Vertex &v) {
      p_z += v.p_z;
      world += v.world;
      diffuse += v.diffuse;
      return *this;
    }

  public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
    slib::vec4 ndc;
    float diffuse;
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
    void operator()(Polygon<Vertex> &poly, int32_t width, int32_t height, const Scene &scene) const {

      for (auto &point : poly.points) {

        bool hasLightSource = false;
        for (const auto &solidPtr : scene.solids) {
          if (!solidPtr->lightSourceEnabled) {
            continue;
          }
          hasLightSource = true;
          const Light &light = solidPtr->light;
          const slib::vec3 &luxDirection = light.getDirection(point.world);
          point.diffuse = std::max(0.0f, smath::dot(point.normal, luxDirection)); // diffuse scalar

          //error no se puede calcular el diffuse con varios lights, no es la suma
        }

        if (!hasLightSource) {
          const Light &light = scene.light;
          const slib::vec3 &luxDirection = scene.light.getDirection(point.world);
          point.diffuse = std::max(0.0f, smath::dot(point.normal, luxDirection)); // diffuse scalar
        }

        Projection<Vertex>::view(width, height, point, false);
      }
    }
  };

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      slib::vec3 diffuseColor{0.0f, 0.0f, 0.0f};
      bool hasLightSource = false;
      for (const auto &solidPtr : scene.solids) {
        if (!solidPtr->lightSourceEnabled) {
          continue;
        }
        hasLightSource = true;
        const Light &light = solidPtr->light;
        float attenuation = light.getAttenuation(vRaster.world);
        float shadow = 1.0f;
        if (scene.shadowsEnabled && solidPtr->shadowMap) {
          shadow = solidPtr->shadowMap->sampleShadow(vRaster.world, vRaster.diffuse);
        }
        diffuseColor += light.color * (vRaster.diffuse * light.intensity * attenuation * shadow);
      }

      if (!hasLightSource) {
        const Light &light = scene.light;
        float attenuation = light.getAttenuation(vRaster.world);
        float shadow = 1.0f;
        if (scene.shadowsEnabled && scene.shadowMap) {
          shadow = scene.shadowMap->sampleShadow(vRaster.world, vRaster.diffuse);
        }
        diffuseColor += light.color * (vRaster.diffuse * light.intensity * attenuation * shadow);
      }
      return Color(poly.material->Ka + poly.material->Kd * diffuseColor).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
