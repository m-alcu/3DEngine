#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <algorithm>
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "vertex_shaders.hpp"

class ShadowMap;

// solid color attribute not interpolated
class GouraudEffect {
public:
  using Vertex = vertex::Lit;
  using VertexShader = vertex::LitVertexShader;

  class GeometryShader {
  public:
    void operator()(Polygon<Vertex> &poly, int32_t width, int32_t height) const {

      for (auto &point : poly.points) {
        Projection<Vertex>::view(width, height, point, false);
      }
    }
  };

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      slib::vec3 diffuseColor{0.0f, 0.0f, 0.0f};
      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        float attenuation = light.getAttenuation(vRaster.world);
        const slib::vec3 &luxDirection = light.getDirection(vRaster.world);
        float diff = std::max(0.0f, smath::dot(vRaster.normal, luxDirection));           
        const auto* shadowComp = scene.shadows().get(entity_);
        float shadow = scene.shadowsEnabled && shadowComp && shadowComp->shadowMap
          ? shadowComp->shadowMap->sampleShadow(vRaster.world, diff, light.position)
          : 1.0f;
        diffuseColor += light.color * (diff * light.intensity * attenuation * shadow);
      }
      return Color(poly.material->Ka + poly.material->Kd * diffuseColor).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
