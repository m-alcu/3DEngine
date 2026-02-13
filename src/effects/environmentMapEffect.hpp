#pragma once
#include "../color.hpp"
#include "../cubemap.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <cmath>
#include "../ecs/MeshComponent.hpp"
#include "../ecs/TransformComponent.hpp"

class EnvironmentMapEffect {
public:
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
                      const TransformComponent &transform,
                      const Scene *scene) const {
      Vertex vertex;
      vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);
      vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
      vertex.normal = transform.normalMatrix * slib::vec4(vData.normal, 0);
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

      CubeMap *cubemap = scene.getCubeMap();
      if (!cubemap) {
        return Color(poly.material->Ka).toBgra();
      }

      slib::vec3 N = smath::normalize(vRaster.normal);
      slib::vec3 V = smath::normalize(scene.camera.pos - vRaster.world);

      // Reflection: R = 2(N·V)N - V
      float NdotV = smath::dot(N, V);
      slib::vec3 R = N * (2.0f * NdotV) - V;

      float r, g, b;
      cubemap->sample(R.x, R.y, R.z, r, g, b);
      slib::vec3 environmentColor{r,g,b};

      const auto &Ks = poly.material->Ks; // vec3
      slib::vec3 color{0.0f, 0.0f, 0.0f};

      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(vRaster.world);
        slib::vec3 L = luxDirection;
        float diff = std::max(0.0f, smath::dot(N, L));
        slib::vec3 halfwayVector =
            smath::normalize(luxDirection - scene.camera.forward);
        float specAngle = std::max(0.0f, smath::dot(N, halfwayVector));
        float spec = std::pow(specAngle, poly.material->Ns);
        float attenuation = light.getAttenuation(vRaster.world);
        const auto* shadowComp = scene.shadows().get(entity_);
        float shadow = scene.shadowsEnabled && shadowComp && shadowComp->shadowMap
          ? shadowComp->shadowMap->sampleShadow(vRaster.world, diff)
          : 1.0f;
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += environmentColor * lightColor * diff;
        color += Ks * lightColor * spec;
      }      

      return Color(color).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
