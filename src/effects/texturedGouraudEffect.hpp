#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <algorithm>

class ShadowMap;

// solid color attribute not interpolated
class TexturedGouraudEffect {
public:
  // the vertex type that will be input into the pipeline
  class Vertex {
  public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::zvec2 _tex,
           slib::vec3 _diffuse, slib::vec3 _world, bool _broken)
        : p_x(px), p_y(py), p_z(pz), ndc(vp), tex(_tex), diffuse(_diffuse), world(_world),
          broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
      return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, ndc + v.ndc, tex + v.tex, 
                    diffuse + v.diffuse, world + v.world, true);
    }

    Vertex operator-(const Vertex &v) const {
      return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, ndc - v.ndc, tex - v.tex, 
                    diffuse - v.diffuse, world - v.world, true);
    }

    Vertex operator*(const float &rhs) const {
      return Vertex(p_x * rhs, p_y, p_z * rhs, ndc * rhs, tex * rhs,
                    diffuse * rhs, world * rhs, true);
    }

    Vertex &operator+=(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      ndc += v.ndc;
      tex += v.tex;
      diffuse += v.diffuse;
      world += v.world;
      return *this;
    }

    Vertex &vraster(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      tex += v.tex;
      diffuse += v.diffuse;
      world += v.world;
      return *this;
    }

    Vertex &hraster(const Vertex &v) {
      p_z += v.p_z;
      tex += v.tex;
      diffuse += v.diffuse;
      world += v.world;
      return *this;
    }

  public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec4 ndc;
    slib::zvec2 tex; // Texture coordinates
    slib::vec3 diffuse;   // Diffuse color
    slib::zvec2 texOverW;
    bool broken = false;
  };

  class VertexShader {
  public:
    Vertex operator()(const VertexData &vData,
                      const slib::mat4 &modelMatrix,
                      const slib::mat4 &normalMatrix,
                      const Scene *scene,
                      const ShadowMap */*shadowMap*/) const {
      Vertex vertex;
      slib::vec3 normal{};
      vertex.world = modelMatrix * slib::vec4(vData.vertex, 1);
      vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
      vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
      normal = normalMatrix * slib::vec4(vData.normal, 0);
      normal = smath::normalize(normal);
      slib::vec3 diffuseColor{0.0f, 0.0f, 0.0f};
      bool hasLightSource = false;
      for (const auto &solidPtr : scene->solids) {
        if (!solidPtr->lightSourceEnabled) {
          continue;
        }
        hasLightSource = true;
        const Light &light = solidPtr->light;
        slib::vec3 luxDirection = light.getDirection(vertex.world);
        float diff = std::max(0.0f, smath::dot(normal, luxDirection));
        float attenuation = light.getAttenuation(vertex.world);
        float shadow = 1.0f;
        if (scene->shadowsEnabled && solidPtr->shadowMap) {
          shadow = solidPtr->shadowMap->sampleShadow(vertex.world, diff);
        }
        diffuseColor += light.color * (diff * light.intensity * attenuation * shadow);
      }

      if (!hasLightSource) {
        const Light &light = scene->light;
        slib::vec3 luxDirection = light.getDirection(vertex.world);
        float diff = std::max(0.0f, smath::dot(normal, luxDirection));
        float attenuation = light.getAttenuation(vertex.world);
        float shadow = 1.0f;
        if (scene->shadowsEnabled && scene->shadowMap) {
          shadow = scene->shadowMap->sampleShadow(vertex.world, diff);
        }
        diffuseColor += light.color * (diff * light.intensity * attenuation * shadow);
      }
      vertex.diffuse = diffuseColor;
      Projection<Vertex>::view(scene->screen.width, scene->screen.height, vertex, true);
      return vertex;
    }
  };

  class GeometryShader {
  public:
    void operator()(Polygon<Vertex> &poly, int32_t width, int32_t height, const Scene &/*scene*/) const {
      for (auto &point : poly.points) {
        Projection<Vertex>::view(width, height, point, false);
      }
    }
  };

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {
      slib::vec3 diff = vRaster.diffuse;
      float w = 1.0f / vRaster.tex.w;
      float r, g, b;
      poly.material->map_Kd.sample(vRaster.tex.x * w, vRaster.tex.y * w, r, g, b);
      slib::vec3 color = slib::vec3{r, g, b} * diff;
      return Color(color).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
