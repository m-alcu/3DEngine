#pragma once
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "vertex_shaders.hpp"
#include "geometry_shaders.hpp"
#include "lighting.hpp"

class FlatEffect {
public:
  using Vertex = vertex::Flat;
  using VertexShader = vertex::FlatVertexShader;
  using GeometryShader = vertex::ViewGeometryShader<Vertex>;

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      if (poly.material->illum == 1) {
        // Emissive material - use emissive color
        slib::vec3 emissiveColor = poly.material->Ke;
        return emissiveColor.toBgra();
      }

      slib::vec3 worldPos = vRaster.worldOverW / vRaster.oneOverW;
      slib::vec3 diffuseColor{0.0f, 0.0f, 0.0f};
      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        float diff =
          std::max(0.0f, smath::dot(poly.rotatedFaceNormal, light.getDirection(poly.points[0].world)));
        float attenuation = light.getAttenuation(poly.points[0].world);
        float shadow = lighting::sampleShadow(scene, entity_, worldPos, diff, light.position);
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        diffuseColor += lightColor * diff;
      }

      return (poly.material->Ka + poly.material->Kd * diffuseColor).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
