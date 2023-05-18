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
  void render(std::map<std::string, shader_program> const& m_shaders, glm::mat4 const& view_transform) override;
private:
  model_object m_geometry;
  glm::fvec3 m_color;
  std::string m_shader;
};
#endif //OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

