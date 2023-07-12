
#include "geometry_node.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <glm/gtc/matrix_inverse.hpp>

//Assignment 1.1 - implementing a scene graph

GeometryNode::GeometryNode(std::string const &name, model_object& geometry, std::string const& shader) :
    Node(name),
    geometry{geometry},
    shader{shader} {}

//returns the geometry of the node
model_object const& GeometryNode::getGeometry() {
  return geometry;
}

//sets the geometry handle object of the node
void GeometryNode::setGeometry(model_object const& newGeometry) {
  geometry = newGeometry;
}

void GeometryNode::render(std::map<std::string, shader_program> const& shaders) {
  // bind shader to which to upload uniforms
  glUseProgram(shaders.at(shader).handle);

  glm::fmat4 model_matrix = getWorldTransform();
  //upload combined transformation matrices for geometry to the shader
  glUniformMatrix4fv(shaders.at(shader).u_locs.at("ModelMatrix"), 1, GL_FALSE, glm::value_ptr(model_matrix));

  if (shader == "planet") {
    //extra matrix for normal transformation to keep them orthogonal to surface
    glm::fmat4 normal_matrix = glm::inverseTranspose(model_matrix);
    //also transform normals
    glUniformMatrix4fv(shaders.at(shader).u_locs.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
  }
  //bind the VAO to draw
  glBindVertexArray(geometry.vertex_AO);

  //draw vertices accorinf to indices
  if (geometry.has_indices) {
    glDrawElements(geometry.draw_mode, geometry.num_elements, model::INDEX.type, NULL);
  //or just draw all vertices
  } else {
    glDrawArrays(geometry.draw_mode, 0, geometry.num_elements);
  }

  //continue with default behaviour, render all children
  Node::render(shaders);
}