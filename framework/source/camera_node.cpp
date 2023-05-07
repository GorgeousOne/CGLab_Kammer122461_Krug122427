#include "camera_node.hpp"

CameraNode::CameraNode(std::string const &name, const glm::mat4 &projectionMatrix) : Node(name),
                                                                                     m_isPerspective{true},
                                                                                     m_isEnabled{false},
                                                                                     m_projectionMatrix{projectionMatrix}
{}

// returns if camera is perspective or orthographic
bool CameraNode::getPerspective() {
  return m_isPerspective;
}

// returns if the camera node is the enabled camera
bool CameraNode::getEnabled() {
  return m_isEnabled;
}

// sets flag whether camera is enabled
void CameraNode::setEnabled(bool enable) {
  m_isEnabled = enable;
}

// returns the projection matrix
glm::mat4 const& CameraNode::getProjectionMatrix() {
  return  m_projectionMatrix;
}

// sets the projection matrix
void CameraNode::setProjectionMatrix(glm::mat4 const& mat) {
  m_projectionMatrix = mat;
}

// returns the view transform matrix
glm::fmat4 CameraNode::getViewTransform() {
  glm::fmat4 view_transform = glm::fmat4(1);
  view_transform = glm::translate(view_transform, m_pos);
  view_transform = glm::rotate(view_transform, m_yaw, glm::fvec3(0, 1, 0));
  view_transform = glm::rotate(view_transform, m_pitch, glm::fvec3(1, 0, 0));
  return view_transform;
}

// translates the camera by a given vector
void CameraNode::translate(glm::vec3 const& delta) {
  m_pos += delta;
}

// returns the camera position
glm::fvec3 CameraNode::getPos() {
  return m_pos;
}

// sets the camera position
void CameraNode::setPos(glm::fvec3 const& newPos) {
  m_pos = newPos;
}

// returns the camera yaw angle
float CameraNode::getYaw() {
  return m_yaw;
}

// sets the camera yaw angle
void CameraNode::setYaw(float newYaw) {
  m_yaw = newYaw;
}

// returns the camera pitch angle
float CameraNode::getPitch() {
  return m_pitch;
}

// sets the camera pitch angle
void CameraNode::setPitch(float newPitch) {
  m_pitch = newPitch;
}

// rotates the camera by a given yaw and pitch angle
void CameraNode::rotate(float dYaw, float dPitch) {
  m_yaw += dYaw;
  //also limits pitch rotation to -90 and 90 degree (looking straight down and up)
  m_pitch = glm::clamp(m_pitch + dPitch, -.5f * glm::pi<float>(), .5f * glm::pi<float>());
}
