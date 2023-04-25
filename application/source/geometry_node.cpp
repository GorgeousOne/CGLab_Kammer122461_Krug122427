
#include "geometry_node.hpp"


model_object const &GeometryNode::getGeometry() {
  return m_geometry;
}

void GeometryNode::setGeometry(model_object const& geometry) {
  m_geometry = geometry;
}

GeometryNode::GeometryNode(std::string const &name, model_object& geometry) : Node(name), m_geometry{geometry} {}
