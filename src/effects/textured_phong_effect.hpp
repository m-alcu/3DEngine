#pragma once
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <cmath>
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "vertex_shaders.hpp"
#include "geometry_shaders.hpp"
#include "lighting.hpp"

// Specular: reflection vector R = 2(N·L)N - L, then dot(R, viewDir)
class TexturedPhongEffect {
public:
  using Vertex = vertex::TexturedLit;
  using VertexShader = vertex::TexturedLitVertexShader;
  using GeometryShader = vertex::TexturedViewGeometryShader<Vertex>;

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      float w = 1.0f / vRaster.texOverW.w;
      float r, g, b;
      poly.material->map_Kd.sample(vRaster.texOverW.x * w, vRaster.texOverW.y * w, r, g, b);
      slib::vec3 texColor{r, g, b};
      slib::vec3 worldPos = vRaster.worldOverW * w;

      const auto &Ks = poly.material->Ks; // vec3
      slib::vec3 normal = smath::normalize(vRaster.normal);
      slib::vec3 color{0.0f, 0.0f, 0.0f};

      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(worldPos);
        float diff = std::max(0.0f, smath::dot(normal, luxDirection));
        slib::vec3 R =
            normal * 2.0f * smath::dot(normal, luxDirection) - luxDirection;
        float specAngle =
            std::max(0.0f, smath::dot(R, scene.camera.forwardNeg()));
        float spec = std::pow(specAngle, poly.material->Ns);
        float attenuation = light.getAttenuation(worldPos);
        float shadow = lighting::sampleShadow(scene, entity_, worldPos, diff, light.position);
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += texColor * lightColor * diff;
        color += Ks * lightColor * spec;
      }

      return color.toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
