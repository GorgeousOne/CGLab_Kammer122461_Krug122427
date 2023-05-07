
#include "geometry_node.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <iostream>

GeometryNode::GeometryNode(std::string const &name, model_object& geometry, std::string const& shader) :
    Node(name),
    m_geometry{geometry},
    m_shader{shader} {}

//returns the geometry of the node
model_object const& GeometryNode::getGeometry() {
  return m_geometry;
}

//sets the geometry handle object of the node
void GeometryNode::setGeometry(model_object const& geometry) {
  m_geometry = geometry;
}

void GeometryNode::render(std::map<std::string, shader_program> const& shaders, glm::mat4 const& view_transform) {
  // bind shader to which to upload uniforms
  glUseProgram(shaders.at(m_shader).handle);

  glm::fmat4 model_matrix = getWorldTransform();
  //upload combined transformation matrices for geometry to the shader
  glUniformMatrix4fv(shaders.at(m_shader).u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(model_matrix));

  //bind the VAO to draw
  glBindVertexArray(m_geometry.vertex_AO);

  if (m_shader == "planet") {
    //extra matrix for normal transformation to keep them orthogonal to surface
    glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(view_transform) * model_matrix);
    //also transform normals
    glUniformMatrix4fv(shaders.at(m_shader).u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
  }
  if (m_geometry.has_indices) {
    glDrawElements(m_geometry.draw_mode, m_geometry.num_elements, model::INDEX.type, NULL);
  } else {
    glDrawArrays(m_geometry.draw_mode, 0, m_geometry.num_elements);
  }
  //continue with default behaviour, render all children
  Node::render(shaders, view_transform);
}