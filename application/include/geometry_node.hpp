#ifndef OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP
#define OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

#include "node.hpp"
#include "structs.hpp"

class GeometryNode : public Node {
public:
  GeometryNode(std::string const &name, model_object& geometry);
  model_object const& getGeometry();
  void setGeometry(model_object const& geometry);
private:
  model_object m_geometry;
};
#endif //OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

