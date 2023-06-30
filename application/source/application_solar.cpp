#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"
#include "texture_loader.hpp"

#include <glbinding/gl/gl.h>
#include <glm/gtc/random.hpp>

// use gl definitions from glbinding 
using namespace gl;

//don't load gl bindings from glfw
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

#include <memory>
#include <string>
#include <fstream>
#include "geometry_node.hpp"
#include "camera_node.hpp"
#include "shader_attrib.hpp"
#include "point_light_node.hpp"

ApplicationSolar::ApplicationSolar(std::string const &resource_path)
    : Application{resource_path},
      planet_object{},
      planet_object2{},
      stars_object{},
      orbit_object{},
      m_keys_down{},
      m_planetData{},
      m_cam{nullptr},
      m_last_frame{0} {
  initializePlanets();
  initializeGeometry();
  initializeShaderPrograms();
  initializeFrameBuffers();
  initializeSceneGraph();
  SceneGraph::get().printGraph(std::cout);

  noiseTex = loadTexture(m_resource_path + "textures/RGBA_noise_small_shadertoy.png");
}

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);

  glDeleteBuffers(1, &stars_object.vertex_BO);
  glDeleteBuffers(1, &stars_object.element_BO);
  glDeleteVertexArrays(1, &stars_object.vertex_AO);

  glDeleteBuffers(1, &orbit_object.vertex_BO);
  glDeleteBuffers(1, &orbit_object.element_BO);
  glDeleteVertexArrays(1, &orbit_object.vertex_AO);

  glDeleteBuffers(1, &skybox_object.vertex_BO);
  glDeleteBuffers(1, &skybox_object.element_BO);
  glDeleteVertexArrays(1, &skybox_object.vertex_AO);
}

void ApplicationSolar::render() {
  double time = glfwGetTime();
  //calculate delta time to last render for FPS independent planet speed
  double dTime = time - m_last_frame;

  rotatePlanets(dTime);
  moveView(dTime);

  glm::fmat4 view_transform = m_cam->getViewTransform();
  uploadUniforms();
  enableMsaaBuffer();

  glUseProgram(m_shaders.at("skybox").handle);
  skybox->render(m_shaders, view_transform);
  SceneGraph::get().getRoot()->render(m_shaders, view_transform);

  renderFrameBuffer();
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

void ApplicationSolar::enableMsaaBuffer() {
  //activate frame buffer
  glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);
  glEnable(GL_DEPTH_TEST);
  //clear buffer
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ApplicationSolar::copyMsaaBuffer() {
  int width = initial_resolution[0];
  int height = initial_resolution[1];

  // bind multisampled buffer for reading and normal buffer for writing
  glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, post_process_fbo);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  // blit color buffer
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

  //blit lighting buffer
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  glDrawBuffer(GL_COLOR_ATTACHMENT1);
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void ApplicationSolar::renderFrameBuffer() {
  copyMsaaBuffer();

  // Bind the default framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_DEPTH_TEST);

  // Use the framebuffer shader program
  glUseProgram(m_shaders.at("post_process").handle);
  // Bind the vertex array object for rendering the screen quad
  glBindVertexArray(screen_quad_object.vertex_AO);

  // Bind the post-processing texture to shader
  auto programID = m_shaders.at("post_process").handle;
  glUniform1i(glGetUniformLocation(programID, "ColorTex"), 0);
  glUniform1i(glGetUniformLocation(programID, "DepthTex"), 1);
  glUniform1i(glGetUniformLocation(programID, "LightTex"), 2);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pp_color_texture);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, pp_depth_texture);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, pp_light_texture);

  //upload noise texture for post-processing effects (theoretically only needed once)
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, noiseTex.handle);
  glUniform1i(m_shaders.at("post_process").u_locs.at("NoiseTex"), 3);
  glUniform1f(m_shaders.at("post_process").u_locs.at("Time"), glfwGetTime());

  // Render the quad with the post-processing texture over the entire screen
  glDrawArrays(screen_quad_object.draw_mode, 0, screen_quad_object.num_elements);
}

// upload uniform values to new locations
void ApplicationSolar::uploadUniforms() {
  std::shared_ptr<PointLightNode> sun = std::dynamic_pointer_cast<PointLightNode>(SceneGraph::get().getRoot()->getChild("sun-light"));
  glm::fvec3 sunPos = glm::fvec3(sun->getWorldTransform()[3]);
  glm::fvec3 sunColor = sun->getColor() * sun->getIntensity();
  glm::fvec3 ambient = glm::fvec3(.5f);

  glUseProgram(m_shaders.at("planet").handle);
  glUniform3fv(m_shaders.at("planet").u_locs.at("AmbientLight"), 1, glm::value_ptr(ambient));
  glUniform3fv(m_shaders.at("planet").u_locs.at("PointLightPos"), 1, glm::value_ptr(sunPos));
  glUniform3fv(m_shaders.at("planet").u_locs.at("PointLightColor"), 1, glm::value_ptr(sunColor));
  glUniform3fv(m_shaders.at("planet").u_locs.at("CameraPos"), 1, glm::value_ptr(m_cam->getPos()));

  glUseProgram(m_shaders.at("skybox").handle);
  glUniform3fv(m_shaders.at("skybox").u_locs.at("CameraPos"), 1, glm::value_ptr(m_cam->getPos()));

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
  m_shaders.emplace("planet", shader_program{{
    {GL_VERTEX_SHADER, m_resource_path + "shaders/simple.vert"},
    {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  m_shaders.emplace("wirenet", shader_program{{
    {GL_VERTEX_SHADER, m_resource_path + "shaders/vao.vert"},
    {GL_FRAGMENT_SHADER, m_resource_path + "shaders/vao.frag"}}});
  m_shaders.emplace("skybox", shader_program{{
    {GL_VERTEX_SHADER, m_resource_path + "shaders/skybox.vert"},
    {GL_FRAGMENT_SHADER, m_resource_path + "shaders/skybox.frag"}}});
  m_shaders.emplace("post_process", shader_program{{
    {GL_VERTEX_SHADER, m_resource_path + "shaders/post_process.vert"},
    {GL_FRAGMENT_SHADER, m_resource_path + "shaders/post_process.frag"}}});

  // request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("planet").u_locs["Color"] = -1;
  m_shaders.at("planet").u_locs["PointLightColor"] = -1;
  m_shaders.at("planet").u_locs["PointLightPos"] = -1;
  m_shaders.at("planet").u_locs["AmbientLight"] = -1;
  m_shaders.at("planet").u_locs["CameraPos"] = -1;
  m_shaders.at("planet").u_locs["Tex"] = -1;
  m_shaders.at("planet").u_locs["NormalMap"] = -1;
  m_shaders.at("planet").u_locs["IsCelEnabled"] = -1;
  m_shaders.at("planet").u_locs["IsNormalMapEnabled"] = -1;

  //stars matrices
  m_shaders.at("wirenet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("wirenet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("wirenet").u_locs["ProjectionMatrix"] = -1;

  m_shaders.at("skybox").u_locs["ModelMatrix"] = -1;
  m_shaders.at("skybox").u_locs["ViewMatrix"] = -1;
  m_shaders.at("skybox").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("skybox").u_locs["Tex"] = -1;
  m_shaders.at("skybox").u_locs["CameraPos"] = -1;

  m_shaders.at("post_process").u_locs["ViewMatrix"] = -1;
  m_shaders.at("post_process").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("post_process").u_locs["ColorTex"] = -1;
  m_shaders.at("post_process").u_locs["DepthTex"] = -1;
  m_shaders.at("post_process").u_locs["LightTex"] = -1;
  m_shaders.at("post_process").u_locs["NoiseTex"] = -1;
  m_shaders.at("post_process").u_locs["Time"] = -1;

}

void ApplicationSolar::initializeFrameBuffers() {
  glGenFramebuffers(1, &msaa_fbo);
  glGenFramebuffers(1, &post_process_fbo);

  glGenTextures(1, &color_texture);
  glGenTextures(1, &depth_texture);
  glGenTextures(1, &light_texture);
  glGenTextures(1, &pp_color_texture);
  glGenTextures(1, &pp_depth_texture);
  glGenTextures(1, &pp_light_texture);

  updateBufferTextures(initial_resolution[0], initial_resolution[1]);

  //create quad covering window to render framebuffer to
  float rectVertices[] = {
      //coords  //texCoords
      1.f, -1.f, 1.f, 0.f,
      -1.f, -1.f, 0.f, 0.f,
      -1.f, 1.f, 0.f, 1.f,

      1.f, 1.f, 1.f, 1.f,
      1.f, -1.f, 1.f, 0.f,
      -1.f, 1.f, 0.f, 1.f
  };
  glGenVertexArrays(1, &screen_quad_object.vertex_AO);
  glGenBuffers(1, &screen_quad_object.vertex_BO);
  glBindVertexArray(screen_quad_object.vertex_AO);
  glBindBuffer(GL_ARRAY_BUFFER, screen_quad_object.vertex_BO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), &rectVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

  screen_quad_object.num_elements = 6;
  screen_quad_object.draw_mode = GL_TRIANGLES;
}

void ApplicationSolar::updateBufferTextures(int width, int height) {

  // Create multisample frame buffer object
  glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);
  // Create frame buffer texture
  createBufferTexture(color_texture, width, height, GL_TEXTURE_2D_MULTISAMPLE, GL_RGB, GL_COLOR_ATTACHMENT0);
  // create buffer for light rendering only
  createBufferTexture(light_texture, width, height, GL_TEXTURE_2D_MULTISAMPLE, GL_RGB, GL_COLOR_ATTACHMENT1);
  // add depth texture
  createBufferTexture(depth_texture, width, height, GL_TEXTURE_2D_MULTISAMPLE, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);

  //order where rendered output should be directed to
  GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, attachments);

  // Check framebuffer completeness
  auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Framebuffer error: " << fboStatus << std::endl;
  }
  //create normal buffer with same components for post-processing
  glBindFramebuffer(GL_FRAMEBUFFER, post_process_fbo);
  createBufferTexture(pp_color_texture, width, height, GL_TEXTURE_2D, GL_RGB, GL_COLOR_ATTACHMENT0);
  createBufferTexture(pp_light_texture, width, height, GL_TEXTURE_2D, GL_RGB, GL_COLOR_ATTACHMENT1);
  createBufferTexture(pp_depth_texture, width, height, GL_TEXTURE_2D, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);

  fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Post-processing framebuffer error: " << fboStatus << std::endl;
  }
}

void ApplicationSolar::createBufferTexture(
    GLuint texture,
    int width,
    int height,
    GLenum target,
    GLenum format,
    GLenum attachment) {
  // Create frame buffer texture
  glBindTexture(target, texture);

  if (target == GL_TEXTURE_2D_MULTISAMPLE) {
    glTexImage2DMultisample(target, 8, format, width, height, GL_TRUE);
  } else {
    glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, NULL);
  }
  // use nearest pixel when up/down scaling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Prevent edge bleeding
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // Attach texture to frame buffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target, texture, 0);
}

// load models
void ApplicationSolar::initializeGeometry() {
  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL | model::TEXCOORD);
  planet_object.draw_mode = GL_TRIANGLES;
  bindObjModel(planet_object, planet_model);

  model planet_model2 = model_loader::obj(m_resource_path + "models/sphere1.obj", model::NORMAL | model::TEXCOORD);
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
  //////////////// Saturn Rings ////////////////

  std::vector<GLfloat> ringVerts{};
  std::vector<GLuint> ringIndices{};
  float ringsScale = 1.5f;

  for (int i = 0; i <= orbitVertCount; ++i) {
    float phi = TWO_PI * i / orbitVertCount;
    //inner vertex
    //xyz
    ringVerts.emplace_back(glm::cos(phi));
    ringVerts.emplace_back(0);
    ringVerts.emplace_back(glm::sin(phi));
    //normal
    ringVerts.emplace_back(0);
    ringVerts.emplace_back(1);
    ringVerts.emplace_back(0);
    //uv somehow off by a few pixels
    ringVerts.emplace_back(0.1);
    ringVerts.emplace_back(0.5);
    //index
    ringIndices.emplace_back(2 * i);

    //outer vertex
    ringVerts.emplace_back(glm::cos(phi) * ringsScale);
    ringVerts.emplace_back(0);
    ringVerts.emplace_back(glm::sin(phi) * ringsScale);
    //normal
    ringVerts.emplace_back(0);
    ringVerts.emplace_back(1);
    ringVerts.emplace_back(0);
    //uv
    ringVerts.emplace_back(0.9);
    ringVerts.emplace_back(0.5);
    //index
    ringIndices.emplace_back(2 * i + 1);
  }
  saturn_rings.draw_mode = GL_TRIANGLE_STRIP;
  saturn_rings.num_elements = (GLsizei) ringIndices.size();
  bindModel(saturn_rings, ringVerts, ringIndices, std::vector<ShaderAttrib>{
      ShaderAttrib{0, 3, 8, 0},
      ShaderAttrib{1, 3, 8, 3},
      ShaderAttrib{2, 2, 8, 6}
  });

  //////////////// Skybox ////////////////

  //make 8 cube vertices
  std::vector<GLfloat> skyboxVerts = {
      //-Z back face
      -1.0f,  1.0f, -1.0f, //0
       1.0f,  1.0f, -1.0f, //1
      -1.0f, -1.0f, -1.0f, //2
       1.0f, -1.0f, -1.0f, //3
       //+Z front face
      -1.0f,  1.0f, 1.0f, //4
       1.0f,  1.0f, 1.0f, //5
      -1.0f, -1.0f, 1.0f, //6
       1.0f, -1.0f, 1.0f, //7
  };

  //triangulate cube faces
  //order and rotation do not seem to effect image rotation :shrug:
  std::vector<GLuint> skyboxIndices = {
      // +X right face
      1, 5, 3,
      3, 5, 7,
      // -X left face
      4, 0, 6,
      6, 0, 2,
      // +Y top face
      4, 5, 0,
      0, 5, 1,
      // -Y down face
      6, 7, 2,
      2, 7, 3,
      // +Z front face
      5, 4, 7,
      7, 4, 6,
      // -Z back face
      0, 1, 2,
      2, 1, 3,
  };

  skybox_object.draw_mode = GL_TRIANGLES;
  skybox_object.num_elements = (GLsizei) skyboxIndices.size();
  bindModel(skybox_object, skyboxVerts, skyboxIndices, std::vector<ShaderAttrib>{
      ShaderAttrib{0, 3, 0, 0}
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

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, model::TEXCOORD.components, model::TEXCOORD.type, GL_FALSE, model.vertex_bytes, model.offsets[model::TEXCOORD]);

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
  m_planetData.emplace("mercury", Planet{.2f, 6, 4, 1, glm::fvec3(0.73, 0.73, 0.73)});
  m_planetData.emplace("venus", Planet{.3f, 7, 8, 1.5, glm::fvec3(0.96, 0.64, 0.09)});
  m_planetData.emplace("earth", Planet{.5, 9, 15, 2, glm::fvec3(0.02, 0.36, 1.00)});
  m_planetData.emplace("mars", Planet{.4f, 11, 17, 1.5, glm::fvec3(0.79, 0.05, 0.05)});
  m_planetData.emplace("jupiter", Planet{2, 14.5f, 20, 5, glm::fvec3(1.00, 0.28, 0.08)});
  m_planetData.emplace("saturn", Planet{1.8f, 19, 30, 4, glm::fvec3(0.89, 0.67, 0.30)});
  m_planetData.emplace("uranus", Planet{1, 22, 45, 3, glm::fvec3(0.51, 0.74, 0.41)});
  m_planetData.emplace("neptune", Planet{.9f, 24, 60, 3, glm::fvec3(0.09, 0.14, 0.92)});
  m_planetData.emplace("moon", Planet{.2f, 1, 5, 4, glm::fvec3(.5f)});
  m_planetData.emplace("sun", Planet{5, 0, 120, 100, glm::fvec3(10000)});
}

void ApplicationSolar::initializeSceneGraph() {
  std::string planetsTexPath = m_resource_path + "textures/planets/";

  // create camera
  m_cam = std::make_shared<CameraNode>("camera", utils::calculate_projection_matrix(initial_aspect_ratio));
  m_cam->setPos(glm::fvec3(0, 10, 0));
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
    std::shared_ptr<Node> planetOrbit = std::make_shared<GeometryNode>(name + "-orbit", orbit_object, planet.color, "wirenet");
    std::shared_ptr<Node> planetHolder = std::make_shared<Node>(name + "-hold");
    std::shared_ptr<GeometryNode> planetGeometry = std::make_shared<GeometryNode>(name + "-geom", planet_object, planet.color, "planet");

    glm::fmat4 transform = glm::fmat4(1);
    //give each planet a random rotation at start
    transform = glm::rotate(transform, glm::linearRand(0.f, 2 * glm::pi<float>()), glm::fvec3(0, 1, 0));
    //translate each planet away from the sun
    transform = glm::translate(transform, glm::vec3(planet.orbitRadius, 0, 0));
    planetHolder->setLocalTransform(transform);

    //scale the geometry node to the defined size of the planet
    planetGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(planet.diameter)));
    planetOrbit->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(planet.orbitRadius)));

    planetGeometry->setTexture(loadTexture(planetsTexPath + name + ".jpg"));

    if (name == "earth") {
      planetGeometry->setNormalMap(loadTexture(planetsTexPath + "earth_normal.jpg"));
    }

    //add planet to scene graph
    root->addChild(planetHolder);
    root->addChild(planetOrbit);
    planetHolder->addChild(planetGeometry);

    if (name == "saturn") {
      std::shared_ptr<GeometryNode> rings = std::make_shared<GeometryNode>(name + "-rings", saturn_rings, planet.color, "planet");
      rings->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(0.5f * planet.diameter + 0.3)));
      rings->setTexture(loadTexture(planetsTexPath + name + "_ring.jpg"));
      planetGeometry->addChild(rings);
    }
  }

  //create stars
  std::shared_ptr<Node> stars = std::make_shared<GeometryNode>("stars", stars_object, glm::vec3(), "wirenet");
  root->addChild(stars);

  //create sun
  std::shared_ptr<Node> sunLight = std::make_shared<PointLightNode>("sun-light", glm::fvec3(1), 1000);
  std::shared_ptr<GeometryNode> sunGeometry = std::make_shared<GeometryNode>("sun-geom", planet_object, m_planetData.at("sun").color, "planet");
  sunGeometry->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(5)));
  sunGeometry->setTexture(loadTexture(planetsTexPath + "sun.jpg"));

  root->addChild(sunLight);
  sunLight->addChild(sunGeometry);

  //create moon
  std::shared_ptr<Node> moonHolder = std::make_shared<Node>("moon-hold");
  std::shared_ptr<GeometryNode> moonGeometry = std::make_shared<GeometryNode>("moon-geom", planet_object2, m_planetData.at("moon").color, "planet");
  std::shared_ptr<Node> moonOrbit = std::make_shared<GeometryNode>("moon-orbit", orbit_object, glm::vec3(), "wirenet");

  Planet moonData = m_planetData.at("moon");
  moonHolder->setLocalTransform(glm::translate(glm::mat4(1), glm::vec3(moonData.orbitRadius, -.3f, 0)));
  moonGeometry->setLocalTransform(glm::rotate(glm::mat4(1), glm::radians(20.f), glm::vec3(0, 0, 1)) * glm::scale(glm::mat4(1), glm::vec3(moonData.diameter)));
  moonGeometry->setTexture(loadTexture(planetsTexPath + "moon.jpg"));
  moonOrbit->setLocalTransform(glm::scale(glm::mat4(1), glm::vec3(moonData.orbitRadius)));

  //create skyboxes
  skybox = std::make_shared<GeometryNode>("skyboxes", skybox_object, glm::vec3(), "skybox");
  skybox->setTexture(loadCubeMap(m_resource_path + "/textures/skyboxes/nebula"));

  //add moon to scene graph rotating around earth
  auto earth = root->getChild("earth-hold");
  moonHolder->addChild(moonGeometry);
  earth->addChild(moonHolder);
  earth->addChild(moonOrbit);
}

texture_object ApplicationSolar::loadTexture(std::string const& fileName) {
  pixel_data pixels = texture_loader::file(fileName, false);
  return utils::create_texture_object(pixels);
}

texture_object ApplicationSolar::loadCubeMap(const std::string &path) {
  texture_object textureObj{};
  glGenTextures(1, &textureObj.handle);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureObj.handle);

  std::vector<std::string> faces {
      "right.png",
      "left.png",
      "down.png",
      "up.png",
      "front.png",
      "back.png",
  };
  for (unsigned int i = 0; i < faces.size(); i++) {
    pixel_data pixels = texture_loader::file(path + "/" + faces.at(i), false);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, pixels.channels, pixels.width, pixels.height, 0, pixels.channels, pixels.channel_type, pixels.ptr());
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  textureObj.target = GL_TEXTURE_CUBE_MAP;
  return textureObj;
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

  if (action == GLFW_PRESS) {
    if(key == GLFW_KEY_1) {
      glUseProgram(m_shaders.at("planet").handle);
      glUniform1i(m_shaders.at("planet").u_locs.at("IsCelEnabled"), 0);
    } else if (key == GLFW_KEY_2) {
      glUseProgram(m_shaders.at("planet").handle);
      glUniform1i(m_shaders.at("planet").u_locs.at("IsCelEnabled"), 1);
    }
  }
}

//Returns true if a keyboard key is depressed right now
bool ApplicationSolar::isKeyDown(int key) const {
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