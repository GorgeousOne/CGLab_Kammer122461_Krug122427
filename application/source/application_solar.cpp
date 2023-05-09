#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"

#include <glbinding/gl/gl.h>
#include <glm/gtc/random.hpp>

// use gl definitions from glbinding 
using namespace gl;

//don't load gl bindings from glfw
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include <memory>
#include <string>
#include "geometry_node.hpp"
#include "camera_node.hpp"
#include "shader_attrib.hpp"

ApplicationSolar::ApplicationSolar(std::string const &resource_path)
    : Application{resource_path},
      planet_object{},
      stars_object{},
      orbit_object{},
      planet_object2{},
      m_keys_down{},
      m_planetData{},
      m_cam{nullptr},
      m_last_frame{0} {
  initializePlanets();
  initializeGeometry();
  initializeShaderPrograms();
  initializeSceneGraph();
  SceneGraph::get().printGraph(std::cout);
}

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);

  glDeleteBuffers(1, &stars_object.vertex_BO);
  glDeleteBuffers(1, &stars_object.element_BO);
  glDeleteVertexArrays(1, &stars_object.vertex_AO);
}

void ApplicationSolar::render() {
  double time = glfwGetTime();
  //calculate delta time to last render for FPS independent planet speed
  double dTime = time - m_last_frame;

  rotatePlanets(dTime);
  moveView(dTime);

  glm::fmat4 view_transform = m_cam->getViewTransform();
  uploadUniforms();
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

// upload uniform values to new locations
void ApplicationSolar::uploadUniforms() {
  glm::fmat4 projection_transform = m_cam->getProjectionMatrix();
  glm::fmat4 view_transform = m_cam->getViewTransform();

  for (auto& iter : m_shaders) {
    // bind shader to which to upload uniforms
    glUseProgram(iter.second.handle);
    // vertices are transformed in camera space, so camera transform must be inverted
    // upload matrix to gpu
    glUniformMatrix4fv(iter.second.u_locs.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(view_transform)));
    // upload matrix to gpu
    glUniformMatrix4fv(iter.second.u_locs.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection_transform));
  }
}

///////////////////////////// intialisation functions /////////////////////////
// load shader sources
void ApplicationSolar::initializeShaderPrograms() {
  // store shader program objects in container
  m_shaders.emplace("planet", shader_program{{{GL_VERTEX_SHADER, m_resource_path + "shaders/simple.vert"},
                                              {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  m_shaders.emplace("wirenet", shader_program{{{GL_VERTEX_SHADER, m_resource_path + "shaders/vao.vert"},
                                              {GL_FRAGMENT_SHADER, m_resource_path + "shaders/vao.frag"}}});
  // request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;
  //stars matrices
  m_shaders.at("wirenet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("wirenet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("wirenet").u_locs["ProjectionMatrix"] = -1;
}

// load models
void ApplicationSolar::initializeGeometry() {
  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL);
  planet_object.draw_mode = GL_TRIANGLES;
  bindObjModel(planet_object, planet_model);

  model planet_model2 = model_loader::obj(m_resource_path + "models/sphere1.obj", model::NORMAL);
  planet_object2.draw_mode = GL_TRIANGLES;
  bindObjModel(planet_object2, planet_model2);

  //////////////// Stars ////////////////

  int starCount = 8000;
  float starRange = 100;
  std::vector<GLfloat> starData{};

  for (int i = 0; i < starCount; ++i) {
    //star xyz position
    starData.emplace_back(glm::linearRand(-starRange, starRange));
    starData.emplace_back(glm::linearRand(-starRange, starRange));
    starData.emplace_back(glm::linearRand(-starRange, starRange));
    //star RGB color
    starData.emplace_back(glm::linearRand(.5f, 1.f));
    starData.emplace_back(glm::linearRand(.5f, 1.f));
    starData.emplace_back(glm::linearRand(.5f, 1.f));
  }
  stars_object.draw_mode = GL_POINTS;
  stars_object.num_elements = starCount;
  bindModel(stars_object, starData, std::vector<GLuint>{}, std::vector<ShaderAttrib>{
    ShaderAttrib{0, 3, 6, 0},
    ShaderAttrib{1, 3, 6, 3}
  });

  //////////////// Orbit ////////////////

  float TWO_PI = 2 * glm::pi<float>();
  int orbitVertCount = 200;

  std::vector<GLfloat> orbitVerts{};
  std::vector<GLuint> orbitIndices{};

  for (int i = 0; i < orbitVertCount; ++i) {
    //orbit vertex position xyz
    orbitVerts.emplace_back(glm::cos(TWO_PI * i / orbitVertCount));
    orbitVerts.emplace_back(0);
    orbitVerts.emplace_back(glm::sin(TWO_PI * i / orbitVertCount));
    //orbit color
    orbitVerts.emplace_back(1);
    orbitVerts.emplace_back(1);
    orbitVerts.emplace_back(1);
    //vertex index
    orbitIndices.emplace_back(i);
  }
  orbit_object.draw_mode = GL_LINE_LOOP;
  orbit_object.num_elements = orbitVertCount;
  bindModel(orbit_object, orbitVerts, orbitIndices, std::vector<ShaderAttrib>{
      ShaderAttrib{0, 3, 6, 0},
      ShaderAttrib{1, 3, 6, 3}
  });
}

void ApplicationSolar::bindObjModel(model_object &bound, model &model) {
// generate vertex array object
  glGenVertexArrays(1, &bound.vertex_AO);
  // bind the array for attaching buffers
  glBindVertexArray(bound.vertex_AO);

  // generate generic buffer
  glGenBuffers(1, &bound.vertex_BO);
  // bind this as a vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, bound.vertex_BO);
  // configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * model.data.size(), model.data.data(), GL_STATIC_DRAW);

  // activate first attribute on gpu
  glEnableVertexAttribArray(0);
  // first attribute is 3 floats with no offset & stride
  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, model.vertex_bytes, model.offsets[model::POSITION]);
  // activate second attribute on gpu
  glEnableVertexAttribArray(1);
  // second attribute is 3 floats with no offset & stride
  glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, model.vertex_bytes, model.offsets[model::NORMAL]);

  // generate generic buffer
  glGenBuffers(1, &bound.element_BO);
  // bind this as a vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bound.element_BO);
  // configure currently bound array buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size * model.indices.size(), model.indices.data(), GL_STATIC_DRAW);

  // store type of primitive to draw
  bound.draw_mode = bound.draw_mode;
  // transfer number of indices to model object
  bound.num_elements = GLsizei(model.indices.size());
}

void ApplicationSolar::bindModel(
    model_object &bound,
    std::vector<GLfloat> const& modelData,
    std::vector<GLuint> const& indices,
    std::vector<ShaderAttrib> const& attribs) {

  // generate vertex array object
  glGenVertexArrays(1, &bound.vertex_AO);
  // bind the array for attaching buffers
  glBindVertexArray(bound.vertex_AO);

  // generate generic buffer
  glGenBuffers(1, &bound.vertex_BO);
  // bind this as a vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, bound.vertex_BO);
  // configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * modelData.size(), modelData.data(), GL_STATIC_DRAW);

  for (ShaderAttrib const& attrib : attribs) {
    // activate first attribute on gpu
    glEnableVertexAttribArray(attrib.index);
    // first attribute is 3 floats with no offset & stride every 6 floats
    glVertexAttribPointer(attrib.index, attrib.size, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * attrib.stride, (GLvoid *) (attrib.offset * sizeof(GLfloat)));
  }
  if (indices.empty()) {
    bound.has_indices = false;
  } else {
    glGenBuffers(1, &bound.element_BO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bound.element_BO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
  }
}


//define planet dimensions
void ApplicationSolar::initializePlanets() {
  // Create the planet data with name, diameter, orbit radius and orbital period in seconds
  m_planetData.emplace("mercury", Planet{.2f, 6, 4, 1});
  m_planetData.emplace("venus", Planet{.3f, 7, 8, 1});
  m_planetData.emplace("earth", Planet{.5, 9, 15, 1});
  m_planetData.emplace("mars", Planet{.4f, 11, 17, 1});
  m_planetData.emplace("jupiter", Planet{2, 14.5f, 20, 1});
  m_planetData.emplace("saturn", Planet{1.8f, 19, 30, 1});
  m_planetData.emplace("uranus", Planet{1, 22, 45, 1});
  m_planetData.emplace("neptune", Planet{.9f, 24, 60, 1});
  m_planetData.emplace("moon", Planet{.2f, 1, 3, 1});
  m_planetData.emplace("sun", Planet{5, 0, 120, 1});
}

void ApplicationSolar::initializeSceneGraph() {
  // create camera
  m_cam = std::make_shared<CameraNode>("camera", utils::calculate_projection_matrix(initial_aspect_ratio));
  m_cam->setPos(glm::fvec3(0, 30, 0));
  m_cam->setPitch(-.5f * glm::pi<float>());

  // Create the sun GeometryNode
  std::shared_ptr<Node> root = SceneGraph::get().getRoot();

  // Add the child GeometryNodes to the sun GeometryNode
  for (auto const& pair : m_planetData) {
    std::string name = pair.first;
    Planet planet = pair.second;

    if (name == "moon" || name == "sun") {
      continue;
    }
    std::shared_ptr<Node> planetHolder = std::make_shared<Node>(name + "-hold");
    std::shared_ptr<Node> planetGeometry = std::make_shared<GeometryNode>(name + "-geom", planet_object, "planet");

    glm::fmat4 transform = glm::fmat4(1);
    //give each planet a random rotation at start
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

    std::shared_ptr<Node> planetOrbit = std::make_shared<GeometryNode>(name + "-orbit", orbit_object, "wirenet");
    planetOrbit->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(planet.orbitRadius)));
    root->addChild(planetOrbit);
  }
  //create stars
  std::shared_ptr<Node> stars = std::make_shared<GeometryNode>("stars", stars_object, "wirenet");
  root->addChild(stars);

  //create sun
  std::shared_ptr<Node> sunLight = std::make_shared<Node>("sun-light");
  std::shared_ptr<Node> sunGeometry = std::make_shared<GeometryNode>("sun-geom", planet_object, "planet");
  sunGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(5)));
  root->addChild(sunLight);
  sunLight->addChild(sunGeometry);

  //create moon
  std::shared_ptr<Node> moonHolder = std::make_shared<Node>("moon-hold");
  std::shared_ptr<Node> moonGeometry = std::make_shared<GeometryNode>("moon-geom", planet_object2, "planet");
  std::shared_ptr<Node> moonOrbit = std::make_shared<GeometryNode>("moon-orbit", orbit_object, "wirenet");

  Planet moonData = m_planetData.at("moon");
  moonHolder->setLocalTransform(glm::translate(glm::mat4(1), glm::vec3(moonData.orbitRadius, -.3f, 0)));
  moonGeometry->setLocalTransform(glm::rotate(glm::mat4(1), glm::radians(20.f), glm::vec3(0, 0, 1)) * glm::scale(glm::mat4(1), glm::vec3(moonData.diameter)));
  moonOrbit->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(moonData.orbitRadius)));

  //add moon to scene graph rotating around earth
  auto earth = root->getChild("earth-hold");
  moonHolder->addChild(moonGeometry);
  earth->addChild(moonHolder);
  earth->addChild(moonOrbit);
}

void ApplicationSolar::moveView(double dTime) {
  glm::fvec4 movement = glm::fvec4(0);
  float speed = 5;
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
}

// exe entry point
int main(int argc, char *argv[]) {
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}