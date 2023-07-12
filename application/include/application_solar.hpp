#ifndef APPLICATION_SOLAR_HPP
#define APPLICATION_SOLAR_HPP

#include "application.hpp"
#include "model.hpp"
#include "structs.hpp"
#include "camera_node.hpp"

//helper struct to store planet information
struct Planet {
  //scale of the planet model
  float diameter;
  //distance from sun (or respective planet) that is being orbited
  float orbitRadius;
  //duration for one orbit in seconds
  float orbitPeriod;
  //duration of rotation around self in seconds
  float rotationPeriod;
};

// gpu representation of model
class ApplicationSolar : public Application {
 public:
  // allocate and initialize objects
  ApplicationSolar(std::string const& resource_path);
  // free allocated objects
  ~ApplicationSolar();

  // react to key input
  void keyCallback(int key, int action, int mods);
  //handle delta mouse movement input
  void mouseCallback(double pos_x, double pos_y);
  //handle resizing
  void resizeCallback(unsigned width, unsigned height);

  // draw all objects
  void render() override;
  void rotatePlanets(double timePassed);

protected:
  void initializeShaderPrograms();
  void initializeGeometry();
  void initializePlanets();
  void initialSceneGraph();

  // update uniform values
  void uploadUniforms();
  // upload projection matrix
  void uploadProjection();
  // upload view matrix
  void uploadView();

  std::shared_ptr<CameraNode> camera;

  std::map<std::string, Planet> planetData;
  // cpu representation of model
  model_object planet_object;
  
  double lastRenderTime;
};

#endif