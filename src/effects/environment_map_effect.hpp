#pragma once
#include "../cubemap.hpp"
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

class EnvironmentMapEffect {
public:
  using Vertex = vertex::Lit;
  using VertexShader = vertex::LitVertexShader;
  using GeometryShader = vertex::ViewGeometryShader<Vertex>;

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      CubeMap *cubemap = scene.getCubeMap();
      if (!cubemap) {
        return poly.material->Ka.toBgra();
      }

      slib::vec3 worldPos = vRaster.worldOverW / vRaster.oneOverW;
      slib::vec3 N = smath::normalize(vRaster.normal);
      slib::vec3 V = smath::normalize(scene.camera.pos - worldPos);

      // Reflection: R = 2(N·V)N - V
      float NdotV = smath::dot(N, V);
      slib::vec3 R = N * (2.0f * NdotV) - V;

      float r, g, b;
      cubemap->sample(R.x, R.y, R.z, r, g, b);
      slib::vec3 environmentColor{r,g,b};

      const auto &Ks = poly.material->Ks;
      slib::vec3 color{0.0f, 0.0f, 0.0f};

      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(worldPos);
        float diff = std::max(0.0f, smath::dot(N, luxDirection));
        if (diff == 0.0f) continue;
        float shadow = lighting::sampleShadow(scene, entity_, worldPos, diff, light.position);
        if (shadow == 0.0f) continue;
        slib::vec3 halfwayVector = smath::normalize(luxDirection - scene.camera.forward);
        float spec = std::pow(std::max(0.0f, smath::dot(N, halfwayVector)), poly.material->Ns);
        float attenuation = light.getAttenuation(worldPos);
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += environmentColor * lightColor * diff;
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
