#include "camera_node.hpp"

// CameraNode class constructor with parameters and initialization list
CameraNode::CameraNode(std::string const &name, bool isPerspective, bool isEnabled, const glm::mat4 &projectionMatrix) : Node(name),
  m_isPerspective{isPerspective},
  m_isEnabled{isEnabled},
  m_projectionMatrix{projectionMatrix}
{}

// Method to get the value of m_isPerspective
bool CameraNode::getPerspective() {
  return m_isPerspective;
}

// Method to get the value of m_isEnabled
bool CameraNode::getEnabled() {
  return m_isEnabled;
}

// Method to set the value of m_isEnabled
void CameraNode::setEnabled(bool enable) {
  m_isEnabled = enable;
}

// Method to get the projection matrix
glm::mat4 const& CameraNode::getProjectionMatrix() {
  return  m_projectionMatrix;
}

// Method to set the projection matrix
void CameraNode::setProjectionMatrix(glm::mat4 const& mat) {
  m_projectionMatrix = mat;
}