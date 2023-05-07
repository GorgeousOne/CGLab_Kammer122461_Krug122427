#ifndef OPENGL_FRAMEWORK_CAMERA_NODE_HPP
#define OPENGL_FRAMEWORK_CAMERA_NODE_HPP

#include "node.hpp"
#include <glm/gtc/matrix_transform.hpp>

// class for the camera inheriting from Node
class CameraNode : public Node {
public:
  // constructor
  CameraNode(std::string const &name, glm::mat4 const& projectionMatrix);
  //
  bool getPerspective();
  // get and set if enabled
  bool getEnabled();
  void setEnabled(bool enable);
  // get and set projection matrix
  glm::mat4 const& getProjectionMatrix();
  void setProjectionMatrix(glm::mat4 const& mat);

  glm::fvec3 getPos();
  void setPos(glm::fvec3 const& newPos);
  float getYaw();
  void setYaw(float newYaw);
  float getPitch();
  void setPitch(float newPitch);

  glm::fmat4 getViewTransform();
  void translate(glm::vec3 const& delta);
  void rotate(float yaw, float pitch);

private:
  bool m_isPerspective;
  bool m_isEnabled;
  glm::mat4 m_projectionMatrix;

  glm::fvec3 m_pos;
  float m_yaw;
  float m_pitch;
};
#endif //OPENGL_FRAMEWORK_CAMERA_NODE_HPP
