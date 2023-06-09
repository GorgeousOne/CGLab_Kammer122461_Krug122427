#ifndef OPENGL_FRAMEWORK_POINT_LIGHT_NODE_HPP
#define OPENGL_FRAMEWORK_POINT_LIGHT_NODE_HPP

#include "node.hpp"

class PointLightNode : public Node {
public:
  PointLightNode(std::string const &name, glm::fvec3 const& lightColor, float lightIntensity);
  void render(std::map<std::string, shader_program> const& m_shaders, glm::mat4 const& view_transform) override;
  glm::vec3 const& getColor() const;
  float getIntensity() const;

private:
  float m_lightIntensity;
  glm::fvec3 m_lightColor;
};

#endif //OPENGL_FRAMEWORK_POINT_LIGHT_NODE_HPP
