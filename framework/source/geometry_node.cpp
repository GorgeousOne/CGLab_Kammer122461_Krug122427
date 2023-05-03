
#include "geometry_node.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <iostream>


model_object const& GeometryNode::getGeometry() {
  return m_geometry;
}

void GeometryNode::setGeometry(model_object const& geometry) {
  m_geometry = geometry;
}

GeometryNode::GeometryNode(std::string const &name, model_object& geometry, std::string const& shaderName) :
    Node(name),
    m_geometry{geometry},
    m_shaderName{shaderName} {}

void GeometryNode::render(std::map<std::string, shader_program> shaders, glm::mat4 const& view_transform, glm::mat4 const& projection) {
  glm::fmat4 model_matrix = getWorldTransform();
  glUseProgram(shaders.at(m_shaderName).handle);
  glUniformMatrix4fv(shaders.at(m_shaderName).u_locs.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(view_transform)));
  glUniformMatrix4fv(shaders.at(m_shaderName).u_locs.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
  glUniformMatrix4fv(shaders.at(m_shaderName).u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(model_matrix));
  // bind the VAO to draw
  glBindVertexArray(m_geometry.vertex_AO);

  if (m_shaderName == "planet") {
    // extra matrix for normal transformation to keep them orthogonal to surface
    glm::fmat4 normal_matrix = glm::inverseTranspose(glm::inverse(view_transform) * model_matrix);
    //also transform normals
    glUniformMatrix4fv(shaders.at(m_shaderName).u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
    glDrawElements(m_geometry.draw_mode, m_geometry.num_elements, model::INDEX.type, NULL);
  } else {
    glDrawArrays(m_geometry.draw_mode, 0, m_geometry.num_elements);
  }
  Node::render(shaders, view_transform, projection);
}
