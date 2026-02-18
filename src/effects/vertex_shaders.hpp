#pragma once
#include "../projection.hpp"
#include "../scene.hpp"
#include "../slib.hpp"
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "vertex_types.hpp"

namespace vertex {

class FlatVertexShader {
public:
  Flat operator()(const VertexData &vData,
                  const TransformComponent &transform,
                  const Scene *scene) const {
    Flat vertex;
    vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);
    vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
    Projection<Flat>::view(scene->screen.width, scene->screen.height, vertex, true);
    return vertex;
  }
};

class LitVertexShader {
public:
  Lit operator()(const VertexData &vData,
                 const TransformComponent &transform,
                 const Scene *scene) const {
    Lit vertex;
    vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);
    vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
    vertex.normal = transform.normalMatrix * slib::vec4(vData.normal, 0);
    Projection<Lit>::view(scene->screen.width, scene->screen.height, vertex, true);
    return vertex;
  }
};

class TexturedFlatVertexShader {
public:
  TexturedFlat operator()(const VertexData &vData,
                          const TransformComponent &transform,
                          const Scene *scene) const {
    TexturedFlat vertex;
    vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);
    vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
    vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
    Projection<TexturedFlat>::texturedView(scene->screen.width, scene->screen.height, vertex, true);
    return vertex;
  }
};

class TexturedLitVertexShader {
public:
  TexturedLit operator()(const VertexData &vData,
                         const TransformComponent &transform,
                         const Scene *scene) const {
    TexturedLit vertex;
    vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);
    vertex.ndc = slib::vec4(vertex.world, 1) * scene->spaceMatrix;
    vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
    vertex.normal = transform.normalMatrix * slib::vec4(vData.normal, 0);
    Projection<TexturedLit>::texturedView(scene->screen.width, scene->screen.height, vertex, true);
    return vertex;
  }
};

} // namespace vertex
