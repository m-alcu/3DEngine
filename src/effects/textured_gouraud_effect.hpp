#pragma once
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <algorithm>
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "vertex_shaders.hpp"
#include "geometry_shaders.hpp"
#include "lighting.hpp"

class TexturedGouraudEffect {
public:
  using Vertex = vertex::TexturedLit;
  using VertexShader = vertex::TexturedLitVertexShader;
  using GeometryShader = vertex::TexturedViewGeometryShader<Vertex>;

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {
      float w = 1.0f / vRaster.texOverW.w;
      slib::vec3 texColor;
      poly.material->map_Kd.sample(vRaster.texOverW.x * w, vRaster.texOverW.y * w, texColor.x, texColor.y, texColor.z);
      slib::vec3 worldPos = vRaster.worldOverW * w;

      slib::vec3 color{0.0f, 0.0f, 0.0f};
      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(worldPos);
        float diff = std::max(0.0f, smath::dot(vRaster.normal, luxDirection));
        if (diff == 0.0f) continue;
        float shadow = lighting::sampleShadow(scene, entity_, worldPos, diff, light.position);
        if (shadow == 0.0f) continue;
        float attenuation = light.getAttenuation(worldPos);
        slib::vec3 lightColor = light.color * (light.intensity * attenuation * shadow);
        color += texColor * diff * lightColor;
      }
      return color.toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
