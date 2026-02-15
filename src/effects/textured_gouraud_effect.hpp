#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <algorithm>
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"

class ShadowMap;

// solid color attribute not interpolated
class TexturedGouraudEffect {
public:
  // the vertex type that will be input into the pipeline
  class Vertex {
  public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::zvec2 _tex,
           slib::vec3 _world, slib::vec3 _normal, bool _broken)
        : p_x(px), p_y(py), p_z(pz), ndc(vp), tex(_tex), world(_world),
          normal(_normal), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
      return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, ndc + v.ndc, tex + v.tex, 
                    world + v.world, normal + v.normal, true);
    }

    Vertex operator-(const Vertex &v) const {
      return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, ndc - v.ndc, tex - v.tex, 
                    world - v.world, normal - v.normal, true);
    }

    Vertex operator*(const float &rhs) const {
      return Vertex(p_x * rhs, p_y, p_z * rhs, ndc * rhs, tex * rhs,
                    world * rhs, normal * rhs, true);
    }

    Vertex &operator+=(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      ndc += v.ndc;
      tex += v.tex;
      world += v.world;
      normal += v.normal;
      return *this;
    }

    Vertex &vraster(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      tex += v.tex;
      world += v.world;
      normal += v.normal;
      return *this;
    }

    Vertex &hraster(const Vertex &v) {
      p_z += v.p_z;
      tex += v.tex;
      world += v.world;
      normal += v.normal;
      return *this;
    }

  public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
    slib::vec4 ndc;
    slib::zvec2 tex; // Texture coordinates
    slib::zvec2 texOverW;
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
      vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
      vertex.normal = transform.normalMatrix * slib::vec4(vData.normal, 0);
      Projection<Vertex>::texturedView(scene->screen.width, scene->screen.height, vertex, true);
      return vertex;
    }
  };

  class GeometryShader {
  public:
    void operator()(Polygon<Vertex> &poly, int32_t width, int32_t height) const {
      for (auto &point : poly.points) {
        Projection<Vertex>::texturedView(width, height, point, false);
      }
    }
  };

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {
      float w = 1.0f / vRaster.tex.w;
      float r, g, b;
      poly.material->map_Kd.sample(vRaster.tex.x * w, vRaster.tex.y * w, r, g, b);
      slib::vec3 texColor{r, g, b};

      slib::vec3 color{0.0f, 0.0f, 0.0f};
      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(vRaster.world);
        float diff = std::max(0.0f, smath::dot(vRaster.normal, luxDirection));
        float attenuation = light.getAttenuation(vRaster.world);
        const auto* shadowComp = scene.shadows().get(entity_);
        float shadow = scene.shadowsEnabled && shadowComp && shadowComp->shadowMap
          ? shadowComp->shadowMap->sampleShadow(vRaster.world, diff, light.position)
          : 1.0f;
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += texColor * lightColor * diff;
      }
      return Color(color).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
