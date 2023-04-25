#ifndef OPENGL_FRAMEWORK_CAMERA_NODE_HPP
#define OPENGL_FRAMEWORK_CAMERA_NODE_HPP

#include "node.hpp"

// class for the camera inheriting from Node
class CameraNode : public Node {
public:
  CameraNode(std::string const &name, bool isPerspective, bool isEnabled, glm::mat4 const& projectionMatrix);
  bool getPerspective();
  bool getEnabled();
  void setEnabled(bool enable);
  glm::mat4 const& getProjectionMatrix();
  void setProjectionMatrix(glm::mat4 const& mat);

private:
  bool m_isPerspective;
  bool m_isEnabled;
  glm::mat4 m_projectionMatrix;
};
#endif //OPENGL_FRAMEWORK_CAMERA_NODE_HPP
