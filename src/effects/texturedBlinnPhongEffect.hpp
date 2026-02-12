#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include "../scene.hpp"
#include <cmath>

class ShadowMap;

// solid color attribute not interpolated
class TexturedBlinnPhongEffect {
public:
  // the vertex type that will be input into the pipeline
  class Vertex {
  public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp,
           slib::vec3 _world, slib::zvec2 _tex, bool _broken)
        : p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp), world(_world),
          tex(_tex), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
      return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal,
                    ndc + v.ndc, world + v.world, tex + v.tex, true);
    }

    Vertex operator-(const Vertex &v) const {
      return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal,
                    ndc - v.ndc, world - v.world, tex - v.tex, true);
    }

    Vertex operator*(const float &rhs) const {
      return Vertex(p_x * rhs, p_y, p_z * rhs, normal * rhs, ndc * rhs,
                    world * rhs, tex * rhs, true);
    }

    Vertex &operator+=(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      normal += v.normal;
      ndc += v.ndc;
      world += v.world;
      tex += v.tex;
      return *this;
    }

    Vertex &vraster(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      normal += v.normal;
      world += v.world;
      tex += v.tex;
      return *this;
    }

    Vertex &hraster(const Vertex &v) {
      p_z += v.p_z;
      normal += v.normal;
      world += v.world;
      tex += v.tex;
      return *this;
    }

  public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
    slib::vec4 ndc;
    slib::zvec2 tex; // Texture coordinates
    slib::zvec2 texOverW;
    bool broken = false;
  };

  class VertexShader {
  public:
    Vertex operator()(const VertexData &vData,
                      const Solid *solid,
                      const Scene *scene) const {
      Vertex vertex;
      vertex.world = solid->transform->modelMatrix * slib::vec4(vData.vertex, 1);
      vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
      vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
      vertex.normal = solid->transform->normalMatrix * slib::vec4(vData.normal, 0);
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

      float w = 1.0f / vRaster.tex.w;
      float r, g, b;
      poly.material->map_Kd.sample(vRaster.tex.x * w, vRaster.tex.y * w, r, g, b);
      slib::vec3 texColor{r, g, b};

      const auto &Ks = poly.material->Ks; // vec3
      slib::vec3 N = smath::normalize(vRaster.normal); // Normal at the fragment
      slib::vec3 color{0.0f, 0.0f, 0.0f};

      for (const auto &[entity_, lightComp] : scene.lights()) {
        const Light &light = lightComp.light;
        slib::vec3 luxDirection = light.getDirection(vRaster.world);
        slib::vec3 L = luxDirection;
        float diff = std::max(0.0f, smath::dot(N, L));
        slib::vec3 halfwayVector =
            smath::normalize(luxDirection - scene.camera.forward);
        float specAngle = std::max(0.0f, smath::dot(N, halfwayVector));
        float spec = std::pow(specAngle, poly.material->Ns);
        float attenuation = light.getAttenuation(vRaster.world);
        float shadow = scene.shadowsEnabled && lightComp.shadowMap ? lightComp.shadowMap->sampleShadow(vRaster.world, diff) : 1.0f;
        float factor = light.intensity * attenuation * shadow;
        slib::vec3 lightColor = light.color * factor;
        color += texColor * lightColor * diff;
        color += Ks * lightColor * spec;
      }

      return Color(color).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
