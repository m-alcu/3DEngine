#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include <cmath>

// solid color attribute not interpolated
class TexturedPhongEffect {
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
                      const slib::mat4 &fullTransformMat,
                      const slib::mat4 &normalTransformMat,
                      const Scene &scene) const {
      Vertex vertex;
      Projection<Vertex> projection;
      vertex.world = fullTransformMat * slib::vec4(vData.vertex, 1);
      vertex.ndc = slib::vec4(vertex.world, 1) * scene.viewMatrix *
                   scene.projectionMatrix;
      vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
      vertex.normal = normalTransformMat * slib::vec4(vData.normal, 0);
      projection.view(scene.screen.width, scene.screen.height, vertex, true);
      return vertex;
    }
  };

  class GeometryShader {
  public:
    void operator()(Polygon<Vertex> &poly, const Scene &scene) const {
      Projection<Vertex> projection;
      for (auto &point : poly.points) {
        projection.view(scene.screen.width, scene.screen.height, point, false);
      }
    }
  };

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {

      const auto &Ks = poly.material.Ks; // vec3
      const slib::vec3 &luxDirection = scene.light.getDirection(vRaster.world);

      slib::vec3 normal = smath::normalize(vRaster.normal);
      float diff = std::max(0.0f, smath::dot(normal, luxDirection));

      slib::vec3 R =
          normal * 2.0f * smath::dot(normal, luxDirection) - luxDirection;
      float specAngle =
          std::max(0.0f, smath::dot(R, scene.forwardNeg)); // viewer
      float spec = std::pow(specAngle, poly.material.Ns);

      // Shadow calculation
      if (scene.shadowMap && scene.shadowsEnabled) {
        float shadow = scene.shadowMap->sampleShadow(vRaster.world, diff);
        diff *= shadow;
        spec *= shadow;
      }

      float w = 1.0f / vRaster.tex.w;
      float r, g, b;
      poly.material.map_Kd.sample(vRaster.tex.x * w, vRaster.tex.y * w, r, g, b);
      return Color(r * diff + Ks.x * spec, g * diff + Ks.y * spec, b * diff + Ks.z * spec)
          .toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};