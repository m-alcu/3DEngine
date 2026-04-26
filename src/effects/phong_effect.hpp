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

// Specular: Phong (reflection R) or Blinn-Phong (halfway H), toggled by scene.blinnPhong
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
      slib::vec3 worldPos = vRaster.worldOverW / vRaster.oneOverW;
      slib::vec3 normal = smath::normalize(vRaster.normal);
      slib::vec3 color = Ka;
      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(worldPos);
        float diff = std::max(0.0f, smath::dot(normal, luxDirection));
        float spec;
        if (scene.blinnPhong) {
            slib::vec3 H = smath::normalize(luxDirection - scene.camera.forward);
            spec = std::pow(std::max(0.0f, smath::dot(normal, H)), poly.material->Ns);
        } else {
            slib::vec3 R = normal * 2.0f * smath::dot(normal, luxDirection) - luxDirection;
            spec = std::pow(std::max(0.0f, smath::dot(R, scene.camera.forwardNeg())), poly.material->Ns);
        }
        float attenuation = light.getAttenuation(worldPos);
        float shadow = lighting::sampleShadow(scene, entity_, worldPos, diff, light.position);
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += (Kd * diff + Ks * spec) * lightColor;
      }
      return color.toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
