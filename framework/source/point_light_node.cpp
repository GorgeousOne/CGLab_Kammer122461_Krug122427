#include "point_light_node.hpp"

PointLightNode::PointLightNode(std::string const &name, float lightIntensity, glm::fvec3 const& lightColor) :
    Node(name),
    m_lightIntensity{lightIntensity},
    m_lightColor{lightColor} {}

void PointLightNode::render(std::map<std::string, shader_program> const &m_shaders, glm::mat4 const &view_transform) {
  Node::render(m_shaders, view_transform);
}
