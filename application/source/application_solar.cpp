#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"

#include <glbinding/gl/gl.h>
#include <glm/gtc/random.hpp>

// use gl definitions from glbinding 
using namespace gl;

//dont load gl bindings from glfw
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include <memory>
#include <string>
#include "geometry_node.hpp"
#include "camera_node.hpp"

ApplicationSolar::ApplicationSolar(std::string const &resource_path)
    : Application{resource_path}, planet_object{},
      m_keys_down{},
      m_planetData{},
      m_cam{nullptr},
      m_last_frame{0} {
  initializeGeometry();
  initializeShaderPrograms();
  initializeSceneGraph();
  SceneGraph::get().printGraph(std::cout);
}

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);
}

void ApplicationSolar::render() {
  double time = glfwGetTime();
  //calculate delta time to last render for FPS independent planet speed
  double dTime = time - m_last_frame;

  rotatePlanets(dTime);
  moveView(dTime);

  glm::fmat4 view_transform = m_cam->getViewTransform();
  uploadView(view_transform);
  SceneGraph::get().getRoot()->render(m_shaders, view_transform);
  m_last_frame = time;
}

void ApplicationSolar::rotatePlanets(double dTime) {
  //run this lambda function for each node of the scene graph
  SceneGraph::get().getRoot()->iterate([this, &dTime] (std::shared_ptr<Node> node) -> void {
    std::string nodeName = node->getName();
    //find out which planet this node is
    std::string planetName = nodeName.substr(0, nodeName.find('-'));
    //try to find planet data for the planet (sun does not get rotated)
    auto iter = m_planetData.find(planetName);

    if (iter == m_planetData.end()) {
      return;
    }
    Planet planet = iter->second;
    float angle = 0;

    if (nodeName.find("hold") != std::string::npos) {
      //calculate how much planet orbited since last frame
      angle = (float) dTime / planet.orbitPeriod * 360;
    } else if (nodeName.find("geom") != std::string::npos){
      //calculate how much planet rotated around own axis
      angle = (float) dTime / planet.rotationPeriod * 360;
    } else {
      return;
    }
    //create transformation matrix containing this rotation
    glm::fmat4 rotation = glm::rotate(glm::fmat4(1), glm::radians(angle), glm::fvec3(0, 1, 0));
    //apply rotation to local transform of planet
    node->setLocalTransform(rotation * node->getLocalTransform());
  });
}

void ApplicationSolar::uploadView(glm::fmat4 const& view_transform) {
  // vertices are transformed in camera space, so camera transform must be inverted
  // upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(view_transform)));
}

void ApplicationSolar::uploadProjection() {
  // upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(m_cam->getProjectionMatrix()));
}

// update uniform locations
void ApplicationSolar::uploadUniforms() {
  // bind shader to which to upload unforms
  glUseProgram(m_shaders.at("planet").handle);
  // upload uniform values to new locations
  uploadView(m_cam->getViewTransform());
  uploadProjection();
}

///////////////////////////// intialisation functions /////////////////////////
// load shader sources
void ApplicationSolar::initializeShaderPrograms() {
  // store shader program objects in container
  m_shaders.emplace("planet", shader_program{{{GL_VERTEX_SHADER, m_resource_path + "shaders/simple.vert"},
                                              {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  // request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;
}

// load models
void ApplicationSolar::initializeGeometry() {
  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL);

  // generate vertex array object
  glGenVertexArrays(1, &planet_object.vertex_AO);
  // bind the array for attaching buffers
  glBindVertexArray(planet_object.vertex_AO);

  // generate generic buffer
  glGenBuffers(1, &planet_object.vertex_BO);
  // bind this as a vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, planet_object.vertex_BO);
  // configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * planet_model.data.size(), planet_model.data.data(), GL_STATIC_DRAW);

  // activate first attribute on gpu
  glEnableVertexAttribArray(0);
  // first attribute is 3 floats with no offset & stride
  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::POSITION]);
  // activate second attribute on gpu
  glEnableVertexAttribArray(1);
  // second attribute is 3 floats with no offset & stride
  glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::NORMAL]);

  // generate generic buffer
  glGenBuffers(1, &planet_object.element_BO);
  // bind this as a vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_object.element_BO);
  // configure currently bound array buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size * planet_model.indices.size(), planet_model.indices.data(), GL_STATIC_DRAW);

  // store type of primitive to draw
  planet_object.draw_mode = GL_TRIANGLES;
  // transfer number of indices to model object 
  planet_object.num_elements = GLsizei(planet_model.indices.size());
}


void ApplicationSolar::initializeSceneGraph() {
  // create camera
  m_cam = std::make_shared<CameraNode>("camera", utils::calculate_projection_matrix(initial_aspect_ratio));
  m_cam->setPos(glm::fvec3(0, 30, 0));
  m_cam->setPitch(-.5f * glm::pi<float>());

  // Create the sun GeometryNode
  std::shared_ptr<Node> root = SceneGraph::get().getRoot();
  std::shared_ptr<Node> sunLight = std::make_shared<Node>("sun-light");
  std::shared_ptr<Node> sunGeometry = std::make_shared<GeometryNode>("sun-geom", planet_object);
  sunGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(5)));

  root->addChild(sunLight);
  sunLight->addChild(sunGeometry);

  // Create the planet data with name, diameter, orbit radius and orbital period in seconds
  m_planetData.emplace("mercury", Planet{.2f, 6, 2, 1});
  m_planetData.emplace("venus", Planet{.3f, 7, 3, 1});
  m_planetData.emplace("earth", Planet{.5, 9, 8, 1});
  m_planetData.emplace("mars", Planet{.4f, 11, 12, 1});
  m_planetData.emplace("jupiter", Planet{2, 14.5f, 20, .1f});
  m_planetData.emplace("saturn", Planet{1.8f, 19, 30, 1});
  m_planetData.emplace("uranus", Planet{1, 22, 45, 1});
  m_planetData.emplace("neptune", Planet{.9f, 24, 60, 1});

  // Add the child GeometryNodes to the sun GeometryNode
  for (auto const& pair : m_planetData) {
    std::string name = pair.first;
    Planet planet = pair.second;
    std::shared_ptr<Node> planetHolder = std::make_shared<Node>(name + "-hold");
    std::shared_ptr<Node> planetGeometry = std::make_shared<GeometryNode>(name + "-geom", planet_object);

    glm::fmat4 transform = glm::fmat4(1);
    //give each planet a random rotation for the start
    transform = glm::rotate(transform, glm::linearRand(0.f, 2 * glm::pi<float>()), glm::fvec3(0, 1, 0));
    //translate each planet from the sun away
    transform = glm::translate(transform, glm::vec3(planet.orbitRadius, 0, 0));
    //update the local transform of the planet holder
    planetHolder->setLocalTransform(transform);
    //scale the geometry node to the defined size of the planet
    planetGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(planet.diameter)));

    //add planet to scene graph
    root->addChild(planetHolder);
    planetHolder->addChild(planetGeometry);
  }
  //create moon separately
  std::shared_ptr<Node> moonHolder = std::make_shared<Node>("moon-hold");
  std::shared_ptr<Node> moonGeometry = std::make_shared<GeometryNode>("moon-geom", planet_object);

  moonHolder->setLocalTransform(glm::translate(glm::mat4(1), glm::vec3(0, 0, 1)));
  moonGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(.1f)));

  //add moon to scene graph rotating around earth
  root->getChild("earth-hold")->addChild(moonHolder);
  moonHolder->addChild(moonGeometry);
  //add moon to planet data, so it gets rotated in render()
  m_planetData.emplace("moon", Planet{.1f, 1 , .5f, .5f});
  m_planetData.emplace("sun", Planet{5, 0 , 0, 1});
}

void ApplicationSolar::moveView(double dTime) {
  glm::fvec4 movement = glm::fvec4(0);
  float speed = 10;
  //update movement vector if movement keys are pressed
  if (isKeyDown(GLFW_KEY_W)) {
    movement[2] -= speed;
  }
  if (isKeyDown(GLFW_KEY_S)) {
    movement[2] += speed;
  }
  if (isKeyDown(GLFW_KEY_A)) {
    movement[0] -= speed;
  }
  if (isKeyDown(GLFW_KEY_D)) {
    movement[0] += speed ;
  }
  if (isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
    movement[1] -= speed;
  }
  if (isKeyDown(GLFW_KEY_SPACE)) {
    movement[1] += speed;
  }
  //create rotation matrix to adapt movement to viewing direction
  glm::fmat4 rotation = glm::fmat4(1);
  rotation = glm::rotate(rotation, m_cam->getYaw(), glm::vec3(0, 1, 0));
  //make movement FPS independent
  movement *= dTime;
  //move camera by rotated movement vector
  m_cam->translate(glm::vec3(rotation * movement));
}

///////////////////////////// callback functions for window events ////////////
// handle key input
void ApplicationSolar::keyCallback(int key, int action, int mods) {
  //store pressed keys in a set
  if (action == GLFW_PRESS) {
    m_keys_down.emplace(key);
  } else if (action == GLFW_RELEASE) {
    m_keys_down.erase(key);
  }
}

//Returns true if a keyboard key is depressed right now
bool ApplicationSolar::isKeyDown(int key) {
  return m_keys_down.find(key) != m_keys_down.end();
}

//handle delta mouse movement input
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  //rotate camera by mouse movement
  float sensitivity = .25f;
  m_cam->rotate(glm::radians(-pos_x) * sensitivity, glm::radians(-pos_y) * sensitivity);
}

//handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
  std::cout << "resize\n";
  //recalculate projection matrix for new aspect ratio
  m_cam->setProjectionMatrix(utils::calculate_projection_matrix(float(width) / float(height)));
  //upload new projection matrix
  uploadProjection();
}

// exe entry point
int main(int argc, char *argv[]) {
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}