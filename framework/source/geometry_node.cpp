
#include "geometry_node.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

GeometryNode::GeometryNode(std::string const &name, model_object& geometry) : Node(name), m_geometry{geometry} {}

//returns the geometry of the node
model_object const& GeometryNode::getGeometry() {
  return m_geometry;
}

//sets the geometry handle object of the node
void GeometryNode::setGeometry(model_object const& geometry) {
  m_geometry = geometry;
}

void GeometryNode::render(std::map<std::string, shader_program> shaders, glm::mat4 const& view_transform) {
  glm::fmat4 model_matrix = getWorldTransform();
  //upload combined transformation matrices for geometry to the shader
  glUniformMatrix4fv(shaders.at("planet").u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(model_matrix));
  //extra matrix for normal transformation to keep them orthogonal to surface
  glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(view_transform) * model_matrix);
  //also transform normals
  glUniformMatrix4fv(shaders.at("planet").u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));

  //bind the VAO to draw
  glBindVertexArray(m_geometry.vertex_AO);
  //render the geometry
  glDrawElements(m_geometry.draw_mode, m_geometry.num_elements, model::INDEX.type, NULL);
  //continue with default behaviour, render all children
  Node::render(shaders, view_transform);
}