#include "point_light_node.hpp"

PointLightNode::PointLightNode(std::string const &name, glm::fvec3 const& lightColor, float lightIntensity) :
    Node(name),
    m_lightIntensity{lightIntensity},
    m_lightColor{lightColor} {}

void PointLightNode::render(std::map<std::string, shader_program> const &m_shaders, glm::mat4 const &view_transform) {
  Node::render(m_shaders, view_transform);
}

// get the color of the light
glm::vec3 const& PointLightNode::getColor() const {
  return m_lightColor;
}

// get the intensity of the light
float PointLightNode::getIntensity() const {
  return m_lightIntensity;
}

