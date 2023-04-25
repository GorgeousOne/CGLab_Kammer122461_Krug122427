#include "camera_node.hpp"

CameraNode::CameraNode(std::string const &name, std::shared_ptr<Node> parent, bool isPerspective, bool isEnabled, const glm::mat4 &projectionMatrix) : Node(name, parent),
	m_isPerspective{isPerspective},
	m_isEnabled{isEnabled},
	m_projectionMatrix{projectionMatrix}
{}