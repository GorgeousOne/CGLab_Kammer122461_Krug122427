#include <string>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "scene_graph.hpp"
#include "node.hpp"
#include "texture_loader.hpp"

//Assignment 1.1 - implementing a scene graph

// get name of the scene
const std::string &SceneGraph::getName() const {
  return name;
}

// set name of scene
void SceneGraph::setName(std::string const& newName) {
  name = newName;
}

// get root of scene
const std::shared_ptr<Node> &SceneGraph::getRoot() const {
  return root;
}

// print scene
std::ostream &operator<<(std::ostream &os, const SceneGraph &graph) {
  for (auto& pair : graph.getRoot()->getChildren()) {
    os << "\t" << *pair.second << std::endl;
  }
  return os;
}

SceneGraph::~SceneGraph() = default;
