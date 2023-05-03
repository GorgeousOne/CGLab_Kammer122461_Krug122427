#ifndef OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP
#define OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

#include "node.hpp"
#include "structs.hpp"
#include "model.hpp"

class GeometryNode : public Node {
public:
  GeometryNode(std::string const &name, model_object& geometry, std::string const& shaderName);
  model_object const& getGeometry();
  void setGeometry(model_object const& geometry);
  void render(std::map<std::string, shader_program> m_shaders, glm::mat4 const& view_transform, glm::mat4 const& projection) override;
private:
  model_object m_geometry;
  std::string m_shaderName;
};
#endif //OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

