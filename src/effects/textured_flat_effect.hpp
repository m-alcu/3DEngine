#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../scene.hpp"
#include "../slib.hpp"
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "vertex_shaders.hpp"
#include "geometry_shaders.hpp"

// solid color attribute not interpolated
class TexturedFlatEffect {
public:
  using Vertex = vertex::TexturedFlat;
  using VertexShader = vertex::TexturedFlatVertexShader;
  using GeometryShader = vertex::TexturedViewGeometryShader<Vertex>;

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
        float attenuation = light.getAttenuation(vRaster.world);
        float diff = std::max(0.0f, smath::dot(poly.rotatedFaceNormal, luxDirection));
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
