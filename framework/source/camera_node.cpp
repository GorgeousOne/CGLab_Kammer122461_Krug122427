#include <iostream>
#include "camera_node.hpp"

//Assignment 1.3 - implement camera controls with mouse and keyboard

/*
 * A class that stores a
 * - position in space
 * - yaw: horizontal rotation in radians (looking left right)
 * - pitch: vertical rotation in radians (looking up down)
 * for a camera and provides methods to move and rotate it.
 */
CameraNode::CameraNode(
    std::string const &name,
    glm::fmat4 const& projectionMatrix) :
    Node(name),
    isPerspective{true},
    isEnabled{false},
    projectionMatrix{projectionMatrix},
    yaw{0},
    pitch{0}
{}

// returns the projection matrix
glm::fmat4 const& CameraNode::getProjectionMatrix() {
  return  projectionMatrix;
}

// sets the projection matrix
void CameraNode::setProjectionMatrix(glm::fmat4 const& mat) {
  projectionMatrix = mat;
}

// returns the view transform matrix
glm::fmat4 CameraNode::getViewTransform() {
  // combines transformations for translation, horizontal rotation and vertical rotation
  glm::fmat4 view_transform = glm::fmat4(1);
  view_transform = glm::translate(view_transform, position);
  //perform horizontal rotation around Y-axis
  view_transform = glm::rotate(view_transform, yaw, glm::fvec3(0, 1, 0));
  //perform vertical rotation around X-axis
  view_transform = glm::rotate(view_transform, pitch, glm::fvec3(1, 0, 0));
  return view_transform;
}

//returns the looking forward direction of the camera
glm::fvec3 CameraNode::getForward() {
  return glm::fvec3(getViewTransform() * glm::fvec4(0, 0, -1, 0));
}

//returns the looking right direction of the camera
glm::fvec3 CameraNode::getRight() {
  return glm::fvec3(getViewTransform() * glm::fvec4(1, 0, 0, 0));
}

// returns the camera position
glm::fvec3 CameraNode::getPosition() {
  return position;
}

// sets the camera position
void CameraNode::setPosition(glm::fvec3 const& newPos) {
  position = newPos;
}

// returns the camera yaw angle
float CameraNode::getYaw() const {
  return yaw;
}

// sets the camera yaw angle
void CameraNode::setYaw(float newYaw) {
  yaw = newYaw;
}

// returns the camera pitch angle
float CameraNode::getPitch() const {
  return pitch;
}

// sets the camera pitch angle
void CameraNode::setPitch(float newPitch) {
  pitch = newPitch;
}

// translates the camera position by a given vector
void CameraNode::translate(glm::vec3 const& delta) {
  position += delta;
}

// rotates the camera view by a given yaw and pitch angle
void CameraNode::rotate(float deltaYaw, float deltaPitch) {
  yaw += deltaYaw;
  pitch += deltaPitch;
  //limit pitch rotation to -90 and 90 degree (looking straight down and up)
  pitch = glm::clamp(pitch, -.5f * glm::pi<float>(), .5f * glm::pi<float>());
}