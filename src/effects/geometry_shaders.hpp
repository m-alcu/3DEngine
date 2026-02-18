#pragma once
#include "../polygon.hpp"
#include "../projection.hpp"

namespace vertex {

template<typename V>
class ViewGeometryShader {
public:
  void operator()(Polygon<V> &poly, int32_t width, int32_t height) const {
    for (auto &point : poly.points) {
      Projection<V>::view(width, height, point, false);
    }
  }
};

template<typename V>
class TexturedViewGeometryShader {
public:
  void operator()(Polygon<V> &poly, int32_t width, int32_t height) const {
    for (auto &point : poly.points) {
      Projection<V>::texturedView(width, height, point, false);
    }
  }
};

} // namespace vertex
