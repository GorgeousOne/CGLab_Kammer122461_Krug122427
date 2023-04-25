#ifndef APPLICATION_SOLAR_HPP
#define APPLICATION_SOLAR_HPP

#include <memory>
#include <set>

#include "application.hpp"
#include "model.hpp"
#include "structs.hpp"
#include "node.hpp"

// gpu representation of model
class ApplicationSolar : public Application {
public:
  // allocate and initialize objects
  ApplicationSolar(std::string const& resource_path);
  // free allocated objects
  ~ApplicationSolar();

  // react to key input
  void keyCallback(int key, int action, int mods) override;
  //handle delta mouse movement input
  void mouseCallback(double pos_x, double pos_y) override;
  //handle resizing
  void resizeCallback(unsigned width, unsigned height) override;

  // draw all objects
  void render() override;
  void initializeSceneGraph();

protected:
  void initializeShaderPrograms();
  void initializeGeometry();
  // update uniform values
  void uploadUniforms() override;
  // upload projection matrix
  void uploadProjection();
  // upload view matrix
  void uploadView(glm::fmat4 const& view_transform);
  // move view based on key presses
  glm::fmat4 createViewTransform();
  void moveView(double dTime);
  bool isKeyDown(int key);

  // cpu representation of model
  model_object planet_object;
  std::shared_ptr<Node> m_scene_root;

  // camera projection matrix
  glm::fmat4 m_view_projection;
  //
  glm::fvec3 m_cam_pos;
  float m_cam_yaw;
  float m_cam_pitch;

  //pressed keys
  std::set<int> m_keys_down;
  //last time render was called
  double m_last_frame;
};

#endif