
#include "geometry_node.hpp"


model const &GeometryNode::getGeometry() {
	return m_geometry;
}

void GeometryNode::setGeometry(model const& geometry) {
	m_geometry = geometry;
}

GeometryNode::GeometryNode(std::string const &name, std::shared_ptr<Node> parent, model const& geometry) : Node(name, parent), m_geometry{geometry} {}
