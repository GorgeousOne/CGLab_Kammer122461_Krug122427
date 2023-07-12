#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"
#include "node.hpp"
#include "scene_graph.hpp"
#include "geometry_node.hpp"

#include <glbinding/gl/gl.h>
// import for glm::linearRand function
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

ApplicationSolar::ApplicationSolar(std::string const& resource_path)
 : Application{resource_path}
 , camera{nullptr}
 , planet_object{}
 , lastRenderTime{0} {

  //creates a camera with a projection matrix to convert 3D coordinates to 2D perspective coordinates
  camera = std::make_shared<CameraNode>("camera", utils::calculate_projection_matrix(initial_aspect_ratio));
  camera->setPosition(glm::fvec3(0, 10, 0));
  camera->setPitch(-.5f * glm::pi<float>());

  initializeGeometry();
  initializeShaderPrograms();
  initializePlanets();
  initialSceneGraph();
}

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);
}

void ApplicationSolar::render() {
  //calculate the time since the last frame has been rendered
  double time = glfwGetTime();
  double timePassed = time - lastRenderTime;

  //rotate the planets depending on how much time passed
  rotatePlanets(timePassed);
  //render scene graph recursively
  SceneGraph::get().getRoot()->render(m_shaders);

  lastRenderTime = time;
}

//Assignment 1.2 - display planets revolving around sun

void ApplicationSolar::rotatePlanets(double timePassed) {
  //run this lambda function to traverse the scene graph and rotate all planets
  SceneGraph::get().getRoot()->traverse([this, &timePassed](std::shared_ptr<Node> node) -> void {
    std::string nodeName = node->getName();
    //find out which planet this node is
    std::string planetName = nodeName.substr(0, nodeName.find('-'));
    //try to find planet data for the planet (sun does not get rotated)
    auto iter = planetData.find(planetName);

    //continue with next node if this is not a planet
    if (iter == planetData.end()) {
      return;
    }
    Planet planet = iter->second;
    float angle;

    if (nodeName.find("hold") != std::string::npos) {
      //calculate how much the planet holder orbited around the sun since last render
      angle = (float) timePassed / planet.orbitPeriod * 360;
    } else if (nodeName.find("geom") != std::string::npos) {
      //calculate how much the planet geometry rotated around own axis
      angle = (float) timePassed / planet.rotationPeriod * 360;
    } else {
      return;
    }
    //create transformation matrix performing the rotation
    glm::fmat4 rotation = glm::rotate(glm::fmat4(1), glm::radians(angle), glm::fvec3(0, 1, 0));
    //apply rotation to local transform of the planet node
    node->setLocalTransform(rotation * node->getLocalTransform());
  });
}

void ApplicationSolar::uploadView() {
  // transformations are relative, so to render everything from the camera's perspective
  // vertices are transformed into camera space, so camera transform needs to be inverted
  glm::fmat4 view_transform = glm::inverse(camera->getViewTransform());

  // upload matrix to shader on gpu
  for (auto& shader : m_shaders) {
    glUseProgram(shader.second.handle);
    glUniformMatrix4fv(shader.second.u_locs.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(view_transform));
  }
}

void ApplicationSolar::uploadProjection() {
  // upload matrix to gpu
  glm::fmat4 projection = camera->getProjectionMatrix();

  for (auto& shader : m_shaders) {
    // bind shader to which to upload unforms
    glUseProgram(shader.second.handle);
    // upload uniform values to new locations
    glUniformMatrix4fv(shader.second.u_locs.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
  }
}

// update uniform locations
void ApplicationSolar::uploadUniforms() {
  uploadView();
  uploadProjection();
}

///////////////////////////// intialisation functions /////////////////////////
// load shader sources
void ApplicationSolar::initializeShaderPrograms() {
  // create a map where shaders loaded from files are stored in
  m_shaders.emplace("planet", shader_program{{
      {GL_VERTEX_SHADER,m_resource_path + "shaders/simple.vert"},
      {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  //shader for stars
  m_shaders.emplace("wirenet", shader_program{{
      {GL_VERTEX_SHADER, m_resource_path + "shaders/vao.vert"},
      {GL_FRAGMENT_SHADER, m_resource_path + "shaders/vao.frag"}}});

  // setup uniform variables for each shader that can be used to send information to shaders
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;

  m_shaders.at("wirenet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("wirenet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("wirenet").u_locs["ProjectionMatrix"] = -1;
}

// load 3D models
void ApplicationSolar::initializeGeometry() {
  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL);

  // generate vertex array object - a collection of arrays of e.g. vertices, normals or indices
  glGenVertexArrays(1, &planet_object.vertex_AO);
  // bind the array for attaching buffers - new buffer objects will belong to this VAO
  glBindVertexArray(planet_object.vertex_AO);

  // generate generic buffer - an array for floats in this case
  glGenBuffers(1, &planet_object.vertex_BO);
  // bind this as a vertex array buffer containing all attributes - vertices, normals and maybe texture coordinates
  glBindBuffer(GL_ARRAY_BUFFER, planet_object.vertex_BO);
  // configure currently bound array buffer - define what size it has and what data is stored in it
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * planet_model.data.size(), planet_model.data.data(), GL_STATIC_DRAW);

  // activate first attribute on gpu - they are called "layout" in shaders and are fed with the VBO data
  glEnableVertexAttribArray(0);
  // define how the VBO should be distributed to the gpu attributes
  // offset: offset from 0th element of the array
  // stride: how many elements are in between the start of this attribute value (e.g. a vertex) and the next one of the same type

  //in this configuration first 3 floats are given to a shader as vertex and the next 3 as normal, then repeat
  // first attribute is 3 floats with 0 offset & 6 stride - vertex position
  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::POSITION]);
  // activate second attribute on gpu - normal
  glEnableVertexAttribArray(1);
  // second attribute is 3 floats with 3 offset & 6 stride
  glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::NORMAL]);

   // generate generic buffer
  glGenBuffers(1, &planet_object.element_BO);
  // bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_object.element_BO);
  // configure currently bound array buffer - fill it with indices defining which vertices to use for drawing
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size * planet_model.indices.size(), planet_model.indices.data(), GL_STATIC_DRAW);

  // store type of primitive to draw - draw these vertices as triangles
  planet_object.draw_mode = GL_TRIANGLES;
  // transfer number of indices to model object
  planet_object.num_elements = GLsizei(planet_model.indices.size());
  planet_object.has_indices = true;

  initializeStars();
}

//Assignment 2.1 - create and display stars geometry

//creates a geometry object with many single vertices rendered as colored stars
void ApplicationSolar::initializeStars() {
  int starCount = 4000;
  float maxDistance = 50;
  std::vector<GLfloat> starData{};

  for (int i = 0; i < starCount; ++i) {
    //create random star xyz position in a certain range
    starData.emplace_back(glm::linearRand(-maxDistance, maxDistance));
    starData.emplace_back(glm::linearRand(-maxDistance, maxDistance));
    starData.emplace_back(glm::linearRand(-maxDistance, maxDistance));
    //assign a random RGB color to each star
    starData.emplace_back(glm::linearRand(0.f, 1.f));
    starData.emplace_back(glm::linearRand(0.f, 1.f));
    starData.emplace_back(glm::linearRand(0.f, 1.f));
  }
  stars_object.draw_mode = GL_POINTS;
  stars_object.num_elements = starCount;
  uploadGeometryObject(stars_object, starData, std::vector<GLuint>{}, std::vector<ShaderAttribute>{
      ShaderAttribute{0, 3, 6, 0},
      ShaderAttribute{1, 3, 6, 3}
  });
}

//helper function it easily upload a geometry object to the gpu
void ApplicationSolar::uploadGeometryObject(
    model_object &geometryObject,
    const std::vector<GLfloat> &modelData,
    const std::vector<GLuint> &indices,
    const std::vector<ShaderAttribute> &attributes) {

  glGenVertexArrays(1, &geometryObject.vertex_AO);
  glBindVertexArray(geometryObject.vertex_AO);

  glGenBuffers(1, &geometryObject.vertex_BO);
  glBindBuffer(GL_ARRAY_BUFFER, geometryObject.vertex_BO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * modelData.size(), modelData.data(), GL_STATIC_DRAW);

  for (ShaderAttribute const& attrib : attributes) {
    glEnableVertexAttribArray(attrib.index);
    glVertexAttribPointer(attrib.index, attrib.size, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * attrib.stride, (GLvoid *) (attrib.offset * sizeof(GLfloat)));
  }
  //only upload indices if the object has any
  if (indices.empty()) {
    geometryObject.has_indices = false;
  } else {
    glGenBuffers(1, &geometryObject.element_BO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometryObject.element_BO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
  }
}

//Assignment 1.2 - display planets revolving around sun

//set up a map with dimensions data of each planet
void ApplicationSolar::initializePlanets() {
  // create the planet data with name, diameter, orbit radius and orbital period in seconds
  planetData.emplace("mercury", Planet{.2f, 6, 4, 1});
  planetData.emplace("venus", Planet{.3f, 7, 8, 1.5});
  planetData.emplace("earth", Planet{.5, 9, 15, 2});
  planetData.emplace("mars", Planet{.4f, 11, 17, 1.5});
  planetData.emplace("jupiter", Planet{2, 14.5f, 20, 5});
  planetData.emplace("saturn", Planet{1.8f, 19, 30, 4});
  planetData.emplace("uranus", Planet{1, 22, 45, 3});
  planetData.emplace("neptune", Planet{.9f, 24, 60, 3});
  planetData.emplace("moon", Planet{.2f, 1, 5, 4});
  planetData.emplace("sun", Planet{5, 0, 120, 100});
}

//Assignment 1.2 - display planets revolving around sun

// add all planets as geometry nodes to to the scene graph
void ApplicationSolar::initialSceneGraph() {
  // Create the sun GeometryNode
  std::shared_ptr<Node> root = SceneGraph::get().getRoot();

  for (auto const& pair : planetData) {
    std::string name = pair.first;
    Planet planet = pair.second;

    if (name == "moon") {
      continue;
    }
    //create a holder and a geometry node for each planet
    std::shared_ptr<Node> planetHolder = std::make_shared<Node>(name + "-hold");
    std::shared_ptr<GeometryNode> planetGeometry = std::make_shared<GeometryNode>(name + "-geom", planet_object, "wirenet");

    glm::fmat4 transform = glm::fmat4(1);
    //translate each planet away from the sun
    transform = glm::translate(transform, glm::vec3(planet.orbitRadius, 0, 0));
    planetHolder->setLocalTransform(transform);

    //scale the geometry node to the defined size of the planet
    planetGeometry->setLocalTransform(glm::scale(glm::fmat4(1), glm::vec3(planet.diameter)));

    //add planet to scene graph
    root->addChild(planetHolder);
    planetHolder->addChild(planetGeometry);
  }
  //create moon
  std::shared_ptr<Node> moonHolder = std::make_shared<Node>("moon-hold");
  std::shared_ptr<GeometryNode> moonGeometry = std::make_shared<GeometryNode>("moon-geom", planet_object, "planet");

  Planet moonData = planetData.at("moon");
  moonHolder->setLocalTransform(glm::translate(glm::fmat4(1), glm::fvec3(moonData.orbitRadius, -.3f, 0)));

  //add moon to scene graph rotating around earth
  auto earth = root->getChild("earth-hold");
  moonHolder->addChild(moonGeometry);
  earth->addChild(moonHolder);

  //Assignment 2.1 - create and display stars geometry

  std::shared_ptr<Node> stars = std::make_shared<GeometryNode>("stars", stars_object, "wirenet");
  root->addChild(stars);
}

//Assignment 1.3 - implement camera controls with mouse and keyboard

///////////////////////////// callback functions for window events ////////////
// handle key input
void ApplicationSolar::keyCallback(int key, int action, int mods) {
  if (action != GLFW_PRESS && action != GLFW_REPEAT) {
    return;
  }
  glm::fvec3 movement = glm::fvec3(0);
  float speed = 0.3f;

  //update movement vector if movement keys are pressed
  if (key == GLFW_KEY_W) {
    movement += camera->getForward() * speed;
  }
  if (key == GLFW_KEY_S) {
    movement -= camera->getForward() * speed;
  }
  if (key == GLFW_KEY_A) {
    movement -= camera->getRight() * speed;
  }
  if (key == GLFW_KEY_D) {
    movement += camera->getRight() * speed;
  }
  camera->translate(movement);
  uploadView();
}

//Assignment 1.3 - implement camera controls with mouse and keyboard

//convert mouse movement in screen X direction to yaw rotation and Y movement to pitch rotation
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  float sensitivity = .01f;
  camera->rotate(-pos_x * sensitivity, -pos_y * sensitivity);
  uploadView();
}

//handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
  // recalculate projection matrix for new aspect ration
  camera->setProjectionMatrix(utils::calculate_projection_matrix(float(width) / float(height)));
  // upload new projection matrix
  uploadProjection();
}


// exe entry point
int main(int argc, char* argv[]) {
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}