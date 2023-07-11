#include "node.hpp"

//Assignment 1.1 - implementing a scene graph

Node::Node(std::string const& name) :
    name{name},
    parent{nullptr},
    depth{0} {}
    
std::string const& Node::getName() const{
  // return name of the node
  return name;
}

std::shared_ptr<Node> Node::getParent() const {
  return parent;
}

void Node::setParent(std::shared_ptr<Node> node) {
  // set the parent of this node
  parent = node;
  path = parent->getPath() + parent->getName();
  // update global transform based on new parent and local transform
  worldTransform = parent->getWorldTransform() * localTransform;
}

//return all parent names combined
std::string const& Node::getPath() const {
  return path;
}

// return depth of the node
int Node::getDepth() const {
  return depth;
}

//add node as child object that gets transformed with parent node together
void Node::addChild(std::shared_ptr<Node> child) {
  children.emplace(child->getName(), child);
  child->worldTransform = getWorldTransform();
  child->depth = depth + 1;
}

// find child by name in children map
std::shared_ptr<Node> Node::getChild(std::string const& childName) const {
  auto iter = children.find(childName);

  if (iter != children.end()) {
    // if found, return the shared pointer to the child
    return iter->second;
  }
  // if not found, return nullptr
  return nullptr;
}

//find and erase the child
std::shared_ptr<Node> Node::removeChild(const std::string& childName) {
  auto iter = children.find(childName);

  if (iter != children.end()) {
    std::shared_ptr<Node> value = std::move(iter->second);
    children.erase(iter);
    return value;
  }
  return nullptr;
}

// return the children map
std::map<std::string, std::shared_ptr<Node>> const& Node::getChildren() {
  return children;
}

// return the local transform of the node
glm::mat4 Node::getLocalTransform() const {
  return localTransform;
}

void Node::setLocalTransform(const glm::mat4& newTransform) {
  localTransform = newTransform;

  // update world transform of children based on relative transform
  // if an object moves, all children objects will get moved with it
  for (auto& pair: children) {
    pair.second->worldTransform = getWorldTransform();
  }
}

// return the world transform of the node as a combination of own world transform (influenced by parent node) and local transform
// this way if the parent node gets moved around, the child will automatically move with it
glm::mat4 Node::getWorldTransform() const {
  return worldTransform * localTransform;
}

// set the new world transform
void Node::setWorldTransform(glm::mat4 const& newTransform) {
  worldTransform = newTransform;

  // update global transform of children
  for (auto& pair: children) {
    pair.second->worldTransform = getWorldTransform();
  }
}

void Node::render(std::map<std::string, shader_program> const& shaders) {
  //render nothing by default and only call render on children
  for (auto& pair : children) {
    pair.second->render(shaders);
  }
}

//helper function to help execute code for each node in a scene graph
void Node::traverse(std::function<void(std::shared_ptr<Node> node)> func) {
  //execute the passed function once with each child as parameter, then repeat down the node tree
  for (auto& pair : children) {
    func(pair.second);
    pair.second->traverse(func);
  }
}

std::ostream& operator<<(std::ostream& os, Node const& node) {
  os << node.name << std::endl;

  for (auto& pair : node.children) {
    os << *pair.second << std::endl;
  }
}