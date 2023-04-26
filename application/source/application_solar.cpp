#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"

#include <glbinding/gl/gl.h>
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
#include "planet.hpp"

ApplicationSolar::ApplicationSolar(std::string const &resource_path)
    : Application{resource_path}, planet_object{},
      m_view_projection{utils::calculate_projection_matrix(initial_aspect_ratio)},
      m_cam_pos(glm::fvec3(10, 0, 25)),
      m_cam_yaw(0),
      m_cam_pitch(0),

      m_keys_down{},
      m_last_frame{0} {
  initializeGeometry();
  initializeShaderPrograms();
  initializeSceneGraph();
}

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);
}

void ApplicationSolar::render() {
  double time = glfwGetTime();

  glm::fmat4 view_transform = createViewTransform();
  moveView(time - m_last_frame);
  uploadView(view_transform);

  SceneGraph::get().getRoot()->render(m_shaders, view_transform);
  m_last_frame = time;
}

glm::fmat4 ApplicationSolar::createViewTransform() {
  glm::fmat4 view_transform = glm::fmat4(1);
  view_transform = glm::translate(view_transform, m_cam_pos);
  view_transform = glm::rotate(view_transform, m_cam_yaw, glm::fvec3(0, 1, 0));
  view_transform = glm::rotate(view_transform, m_cam_pitch, glm::fvec3(1, 0, 0));
  return view_transform;
}

void ApplicationSolar::uploadView(glm::fmat4 const& view_transform) {
  // vertices are transformed in camera space, so camera transform must be inverted
  // upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(view_transform)));
}

void ApplicationSolar::uploadProjection() {
  // upload matrix to gpu
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(m_view_projection));
}

// update uniform locations
void ApplicationSolar::uploadUniforms() {
  // bind shader to which to upload unforms
  glUseProgram(m_shaders.at("planet").handle);
  // upload uniform values to new locations
  uploadView(createViewTransform());
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
  // Create the sun GeometryNode
  std::shared_ptr<Node> root = SceneGraph::get().getRoot();

  std::shared_ptr<Node> sunLight = std::make_shared<Node>("sun");
  std::shared_ptr<Node> sunGeometry = std::make_shared<GeometryNode>("sun", planet_object);
  sunGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(5)));

  root->addChild(sunLight);
  sunLight->addChild(sunGeometry);

  // create camera
  std::shared_ptr<Node> camera = std::make_shared<CameraNode>("camera", true, true, glm::mat4());
  // Create child GeometryNodes for each plan, planet_object

  std::vector<Planet> planetData {
      {"mercury", .2f, 6, 1},
      {"venus", .3f, 7, 2},
      {"earth", .5, 9, 3},
      {"mars", .4f, 11, 4},
      {"jupiter", 2, 14.5f, 5},
      {"saturn", 1.8f, 19, 6},
      {"uranus", 1, 22, 7},
      {"neptune", .9f, 24, 8}
  };

  // Add the child GeometryNodes to the sun GeometryNode
  for (std::size_t i = 0; i != planetData.size(); ++i) {
    Planet planet = planetData[i];
    std::shared_ptr<Node> planetHolder = std::make_shared<Node>(planet.name + "-hold");
    std::shared_ptr<Node> planetGeometry = std::make_shared<GeometryNode>(planet.name + "-geom", planet_object);
    planetHolder->setLocalTransform(glm::translate(glm::mat4(1), glm::vec3(planet.solarRadius, 0, 0)));
    planetGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(planet.diameter)));

    root->addChild(planetHolder);
    planetHolder->addChild(planetGeometry);
  }
  std::shared_ptr<Node> moonHolder = std::make_shared<Node>("moon-hold");
  std::shared_ptr<Node> moonGeometry = std::make_shared<GeometryNode>("moon-geom", planet_object);

  moonHolder->setLocalTransform(glm::translate(glm::mat4(1), glm::vec3(0, 0, 1)));
  moonGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(.1f)));

  root->getChild("earth-hold")->addChild(moonHolder);
  moonHolder->addChild(moonGeometry);
  SceneGraph::get().printGraph();
}

///////////////////////////// callback functions for window events ////////////
// handle key input
void ApplicationSolar::keyCallback(int key, int action, int mods) {
  if (action == GLFW_PRESS) {
    m_keys_down.emplace(key);
  } else if (action == GLFW_RELEASE) {
    m_keys_down.erase(key);
  }
}

void ApplicationSolar::moveView(double dTime) {
  glm::fvec4 movement = glm::fvec4(0);
  float speed = 5;
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
    m_cam_pos[1] -= speed * dTime;
  }
  if (isKeyDown(GLFW_KEY_SPACE)) {
    m_cam_pos[1] += speed * dTime;
  }
  glm::fmat4 rotation = glm::fmat4(1);
  rotation = glm::rotate(rotation, m_cam_yaw, glm::vec3(0, 1, 0));
  movement *= dTime;
  m_cam_pos += glm::vec3(rotation * movement);
}

bool ApplicationSolar::isKeyDown(int key) {
  return m_keys_down.find(key) != m_keys_down.end();
}

//handle delta mouse movement input
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  // mouse handling
  float sensitivity = .25f;
  m_cam_yaw -= glm::radians(pos_x) * sensitivity;
  m_cam_pitch -= glm::radians(pos_y) * sensitivity;
  m_cam_pitch = glm::clamp(m_cam_pitch, -.5f * glm::pi<float>(), .5f * glm::pi<float>());
}

//handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
  // recalculate projection matrix for new aspect ration
  m_view_projection = utils::calculate_projection_matrix(float(width) / float(height));
  // upload new projection matrix
  uploadProjection();
}


// exe entry point
int main(int argc, char *argv[]) {
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}