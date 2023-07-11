#ifndef OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP
#define OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

#include "node.hpp"
#include "structs.hpp"
#include "model.hpp"

//Assignment 1.1 - implementing a scene graph
class GeometryNode : public Node {
public:
  GeometryNode(std::string const &name, model_object& geometry, std::string const& shader);
  model_object const& getGeometry();
  void setGeometry(model_object const& geometry);
  void render(std::map<std::string, shader_program> const& shaders) override;
private:
  //geometry model to render
  model_object geometry;
  //name of the shader to render object with
  std::string shader;
};
#endif //OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP
