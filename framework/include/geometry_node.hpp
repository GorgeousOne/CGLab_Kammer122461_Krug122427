#ifndef OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP
#define OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

#include "node.hpp"
#include "structs.hpp"
#include "model.hpp"

class GeometryNode : public Node {
public:
  GeometryNode(std::string const &name, model_object& geometry, glm::fvec3 color, std::string const& shader);
  model_object const& getGeometry();
  void setGeometry(model_object const& geometry);
  void setTexture(texture_object const& texture);
  void setNormalMap(texture_object const& normalMap);
  void render(std::map<std::string, shader_program> const& m_shaders, glm::mat4 const& view_transform) override;
private:
  model_object m_geometry;
  texture_object m_texture;
  texture_object m_normalMap;
  glm::fvec3 m_color;
  std::string m_shader;
};
#endif //OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

