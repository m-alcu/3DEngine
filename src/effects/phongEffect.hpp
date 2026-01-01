#pragma once
#include "../color.hpp"
#include "../slib.hpp"
#include <cmath>

// solid color attribute not interpolated
class PhongEffect {
public:
  // the vertex type that will be input into the pipeline
  class Vertex {
  public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp,
           bool _broken)
        : p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
      return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal,
                    ndc + v.ndc, true);
    }

    Vertex operator-(const Vertex &v) const {
      return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal,
                    ndc - v.ndc, true);
    }

    Vertex operator*(const float &rhs) const {
      return Vertex(p_x * rhs, p_y, p_z * rhs, normal * rhs, ndc * rhs, true);
    }

    Vertex &operator+=(const Vertex &v) {
      p_x += v.p_x;
      p_z += v.p_z;
      normal += v.normal;
      ndc += v.ndc;
      return *this;
    }

    Vertex &hraster(const Vertex &v) {
      p_z += v.p_z;
      normal += v.normal;
      return *this;
    }

  public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
    slib::vec4 ndc;
    bool broken = false;
  };

  // Fragment data - only what's needed for rasterization
  class Fragment {
  public:
    Fragment() {}

    Fragment(float pz, slib::vec3 w, slib::vec3 n)
        : p_z(pz), world(w), normal(n) {}

    Fragment operator+(const Fragment &f) const {
      return Fragment(p_z + f.p_z, world + f.world, normal + f.normal);
    }

    Fragment operator-(const Fragment &f) const {
      return Fragment(p_z - f.p_z, world - f.world, normal - f.normal);
    }

    Fragment operator*(const float &rhs) const {
      return Fragment(p_z * rhs, world * rhs, normal * rhs);
    }

    Fragment &operator+=(const Fragment &f) {
      p_z += f.p_z;
      world += f.world;
      normal += f.normal;
      return *this;
    }

  public:
    static Fragment fromVertex(const Vertex &v) {
      return Fragment(v.p_z, v.world, v.normal);
    }

  public:
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
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
      vertex.normal = normalTransformMat * slib::vec4(vData.normal, 0);
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
    uint32_t operator()(Fragment &frag, const Scene &scene,
                        Polygon<Vertex> &poly) const {

      const auto &Ka = poly.material.Ka; // vec3
      const auto &Kd = poly.material.Kd; // vec3
      const auto &Ks = poly.material.Ks; // vec3
      const slib::vec3 &luxDirection = scene.light.getDirection(frag.world);

      slib::vec3 normal = smath::normalize(frag.normal);
      float diff = std::max(0.0f, smath::dot(normal, luxDirection));

      slib::vec3 R =
          normal * 2.0f * smath::dot(normal, luxDirection) - luxDirection;
      // NOTE: For performance we approximate the per-fragment view vector V
      // with -camera.forward. This assumes all view rays are parallel (like an
      // orthographic camera). Works well when the camera is far away or objects
      // are small on screen. Not physically correct: highlights will "stick" to
      // the camera instead of sliding across surfaces when moving in
      // perspective, but it’s often a good enough approximation.
      float specAngle =
          std::max(0.0f, smath::dot(R, scene.forwardNeg)); // viewer
      float spec = std::pow(specAngle, poly.material.Ns);

      slib::vec3 color = Ka + Kd * diff + Ks * spec;
      return Color(color).toBgra(); // assumes vec3 uses .r/g/b or [0]/[1]/[2]
    }
  };

public:
  VertexShader vs;
  GeometryShader gs;
  PixelShader ps;
};
