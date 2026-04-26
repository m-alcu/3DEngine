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

class GouraudEffect {
public:
  using Vertex = vertex::Lit;
  using VertexShader = vertex::LitVertexShader;
  using GeometryShader = vertex::ViewGeometryShader<Vertex>;

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      slib::vec3 worldPos = vRaster.worldOverW / vRaster.oneOverW;
      slib::vec3 normal = smath::normalize(vRaster.normal);
      slib::vec3 color = poly.material->Ka;
      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(worldPos);
        float diff = std::max(0.0f, smath::dot(normal, luxDirection));
        if (diff == 0.0f) continue;
        float attenuation = light.getAttenuation(worldPos);
        float shadow = lighting::sampleShadow(scene, entity_, worldPos, diff, light.position);
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += poly.material->Kd * diff * lightColor;
      }
      return color.toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
