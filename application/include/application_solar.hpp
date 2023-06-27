#ifndef APPLICATION_SOLAR_HPP
#define APPLICATION_SOLAR_HPP

#include <memory>
#include <set>

#include "application.hpp"
#include "model.hpp"
#include "structs.hpp"
#include "node.hpp"
#include "geometry_node.hpp"
#include "scene_graph.hpp"
#include "planet.hpp"
#include "shader_attrib.hpp"

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
  void initializePlanets();

  void bindObjModel(model_object &bound, model &model);
  void bindModel(
      model_object &bound,
      std::vector<GLfloat> const& modelData,
      std::vector<GLuint> const& indices,
      std::vector<ShaderAttrib> const& attribs);


protected:
  void initializeShaderPrograms();
  void initializeGeometry();
  // update uniform values
  void uploadUniforms() override;

  void rotatePlanets(double dTime);
  // move view based on key presses
  void moveView(double dTime);
  //
  bool isKeyDown(int key) const;

  texture_object loadTexture(std::string const& fileName);

  void initializeFrameBuffers();
  void updateBufferTextures(int width, int height);
  void enableMsaaBuffer();
  void copyMsaaBuffer();
  void renderFrameBuffer();
  void createBufferTexture(
      GLuint texture,
      int width,
      int height,
      GLenum target,
      GLenum format,
      GLenum attachment);

  unsigned int msaa_fbo;
  unsigned int color_texture;
  unsigned int depth_texture;
  unsigned int light_texture;

  unsigned int post_process_fbo;
  unsigned int pp_color_texture;
  unsigned int pp_depth_texture;
  unsigned int pp_light_texture;

  // cpu representation of model
  model_object screen_quad_object;
  model_object planet_object;
  model_object planet_object2;
  model_object stars_object;
  model_object orbit_object;
  model_object saturn_rings;
  model_object skybox_object;

  texture_object noiseTex;

  //pressed keys
  std::set<int> m_keys_down;
  std::map<std::string, Planet> m_planetData;
  std::shared_ptr<CameraNode> m_cam;
  std::shared_ptr<GeometryNode> skybox;

  //last time render was called
  double m_last_frame;

  texture_object loadCubeMap(const std::string &fileName);
};

#endif