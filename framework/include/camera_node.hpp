#ifndef OPENGL_FRAMEWORK_CAMERA_NODE_HPP
#define OPENGL_FRAMEWORK_CAMERA_NODE_HPP

#include "node.hpp"
#include <glm/gtc/matrix_transform.hpp>

//Assignment 1.3 - implement camera controls with mouse and keyboard

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
  glm::fmat4 const& getProjectionMatrix();
  void setProjectionMatrix(glm::fmat4 const& mat);

  glm::fvec3 getPosition();
  void setPosition(glm::fvec3 const& newPos);
  float getYaw() const;
  void setYaw(float newYaw);
  float getPitch() const;
  void setPitch(float newPitch);

  glm::fmat4 getViewTransform();
  glm::fvec3 getForward();
  glm::fvec3 getRight();

  void translate(glm::vec3 const& delta);
  void rotate(float deltaYaw, float deltaPitch);

private:
  bool isPerspective;
  bool isEnabled;
  glm::fmat4 projectionMatrix;

  glm::fvec3 position;
  float yaw;
  float pitch;
};
#endif //OPENGL_FRAMEWORK_CAMERA_NODE_HPP