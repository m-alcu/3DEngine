#pragma once
#include "../color.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "../slib.hpp"
#include <algorithm>


// solid color attribute not interpolated
class TexturedGouraudEffect {
public:
  // the vertex type that will be input into the pipeline
  class Vertex {
  public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::zvec2 _tex,
           float _diffuse, slib::vec3 _world, bool _broken)
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
    float diffuse;   // Diffuse color
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
      slib::vec3 normal{};
      Projection<Vertex> projection;
      vertex.world = fullTransformMat * slib::vec4(vData.vertex, 1);
      vertex.ndc = slib::vec4(vertex.world, 1) * scene.viewMatrix *
                   scene.projectionMatrix;
      vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
      normal = normalTransformMat * slib::vec4(vData.normal, 0);
      vertex.diffuse = std::max(
          0.0f, smath::dot(normal, scene.light.getDirection(vertex.world)));
      projection.view(scene, vertex, true);
      return vertex;
    }
  };

  class GeometryShader {
  public:
    void operator()(Polygon<Vertex> &poly, const Scene &scene) const {
      Projection<Vertex> projection;
      for (auto &point : poly.points) {
        projection.view(scene, point, false);
      }
    }
  };

  class PixelShader {
  public:
    uint32_t operator()(const Vertex &vRaster, const Scene &scene,
                        const Polygon<Vertex> &poly) const {
      // Shadow calculation
      float shadow = 1.0f;
      if (scene.shadowMap && scene.shadowsEnabled) {
        shadow = scene.shadowMap->sampleShadow(vRaster.world, vRaster.diffuse);
      }

      float diff = vRaster.diffuse * shadow;
      float w = 1.0f / vRaster.tex.w;
      float r, g, b;
      poly.material.map_Kd.sample(vRaster.tex.x * w, vRaster.tex.y * w, r, g, b);
      return Color(r * diff, g * diff, b * diff).toBgra();
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};