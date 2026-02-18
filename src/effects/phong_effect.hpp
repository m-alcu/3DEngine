#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <cmath>
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "vertex_shaders.hpp"

// solid color attribute not interpolated
class PhongEffect {
public:
  using Vertex = vertex::Lit;
  using VertexShader = vertex::LitVertexShader;
  using GeometryShader = vertex::ViewGeometryShader<Vertex>;

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      const auto &Ka = poly.material->Ka; // vec3
      const auto &Kd = poly.material->Kd; // vec3
      const auto &Ks = poly.material->Ks; // vec3
      slib::vec3 normal = smath::normalize(vRaster.normal);
      slib::vec3 color = Ka;
      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(vRaster.world);
        float diff = std::max(0.0f, smath::dot(normal, luxDirection));
        slib::vec3 R =
            normal * 2.0f * smath::dot(normal, luxDirection) - luxDirection;
        float specAngle =
            std::max(0.0f, smath::dot(R, scene.forwardNeg)); // viewer
        float spec = std::pow(specAngle, poly.material->Ns);
        float attenuation = light.getAttenuation(vRaster.world);
        const auto* shadowComp = scene.shadows().get(entity_);
        float shadow = scene.shadowsEnabled && shadowComp && shadowComp->shadowMap
          ? shadowComp->shadowMap->sampleShadow(vRaster.world, diff, light.position)
          : 1.0f;
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
