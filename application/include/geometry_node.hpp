#ifndef OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP
#define OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

#include "node.hpp"
#include "model.hpp"

class GeometryNode : public Node {
public:
	GeometryNode(std::string const &name, std::shared_ptr<Node> parent, model const& geometry);
	model const& getGeometry();
	void setGeometry(model const& geometry);
private:
	model m_geometry;
};
#endif //OPENGL_FRAMEWORK_GEOMETRY_NODE_HPP

