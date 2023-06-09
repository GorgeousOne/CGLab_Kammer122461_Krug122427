
#include "geometry_node.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <iostream>

GeometryNode::GeometryNode(std::string const &name, model_object& geometry, glm::fvec3 color, std::string const& shader) :
    Node(name),
    m_geometry{geometry},
    m_texture{},
    m_normalMap{},
    m_hasNormalMap{false},
    m_color{color},
    m_shader{shader} {}

//returns the geometry of the node
model_object const& GeometryNode::getGeometry() {
  return m_geometry;
}

//sets the geometry handle object of the node
void GeometryNode::setGeometry(model_object const& geometry) {
  m_geometry = geometry;
}

void GeometryNode::setTexture(texture_object const& texture) {
  m_texture = texture;
}

void GeometryNode::setNormalMap(texture_object const& normalMap) {
  m_normalMap = normalMap;
  m_hasNormalMap = true;
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
    //activate 0th texture0
    glActiveTexture(GL_TEXTURE0); //default anyway
    glBindTexture(m_texture.target, m_texture.handle);
    //upload 0th texture to shader
    glUniform1i(shaders.at(m_shader).u_locs.at("Tex"), 0);

    //extra matrix for normal transformation to keep them orthogonal to surface
    glm::fmat4 normal_matrix = glm::inverseTranspose(model_matrix);
    //also transform normals
    glUniformMatrix4fv(shaders.at(m_shader).u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
    //upload color
    glUniform3fv(shaders.at(m_shader).u_locs.at("Color"), 1, glm::value_ptr(m_color));

    glUniform1i(shaders.at(m_shader).u_locs.at("IsNormalMapEnabled"), m_hasNormalMap ? 1 : 0);
  }
  if (m_hasNormalMap) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_normalMap.handle);
    glUniform1i(shaders.at(m_shader).u_locs.at("NormalMap"), 1);
  }
  if (m_geometry.has_indices) {
    glDrawElements(m_geometry.draw_mode, m_geometry.num_elements, model::INDEX.type, NULL);
  } else {
    glDrawArrays(m_geometry.draw_mode, 0, m_geometry.num_elements);
  }
  if (m_shader == "skybox") {
    glClear(GL_DEPTH_BUFFER_BIT);
  }
  //continue with default behaviour, render all children
  Node::render(shaders, view_transform);
}
