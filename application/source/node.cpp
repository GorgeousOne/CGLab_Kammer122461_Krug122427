#include "node.hpp"

Node::Node(std::string const& name) :
    m_parent{nullptr},
    m_children{std::map<std::string, std::shared_ptr<Node>>{}},
    m_name{name},
    m_path{""},
    m_depth{0},
    m_localTransform{glm::mat4()},
    m_globalTransform{glm::mat4()} {}

std::shared_ptr<Node> Node::getParent() {
  // return an empty shared pointer, indicating no parent
  return std::shared_ptr<Node>();
}

void Node::setParent(std::shared_ptr<Node> node) {
  // set the parent of this node
  m_parent = node;
  m_path = m_parent->getPath() + m_parent->getName();
  // update global transform based on new parent and local transform
  m_globalTransform = m_parent->getWorldTransform() * m_localTransform;
}

std::shared_ptr<Node> Node::getChild(std::string const& name) {
  // find child by name in children map
  auto iter = m_children.find(name);

  if (iter != m_children.end()) {
    // if found, return the shared pointer to the child
    return iter->second;
  }
  // if not found, return nullptr
  return nullptr;
}

std::map<std::string, std::shared_ptr<Node>> const &Node::getChildrenList() {
  // return the children map
  return m_children;
}

std::string const &Node::getName() {
  // return the name of the node
  return m_name;
}

std::string const &Node::getPath() {
  // return the path of the node
  return m_path;
}

int Node::getDepth() {
  // return the depth of the node
  return m_depth;
}

glm::mat4 Node::getLocalTransform() {
  // return the local transform of the node
  return m_localTransform;
}

void Node::setLocalTransform(const glm::mat4 &newTransform) {
  // calculate relative transform between new and old local transform
//  glm::mat4 relTransform = newTransform * glm::inverse(m_localTransform);
  // set the new local transform
  m_localTransform = newTransform;
  // update global transform obased on relative transform
//  m_globalTransform = relTransform * m_globalTransform;

  for (auto& pair: m_children) {
    // update global transform of children based on relative transform
    pair.second->m_globalTransform *= getWorldTransform();
  }
}

glm::mat4 Node::getWorldTransform() {
  // return the global transform of the node
  return m_globalTransform * m_localTransform;
}

void Node::setWorldTransform(glm::mat4 const& newTransform) {
  // calculate relative transform between new and old global transform
  glm::mat4 relTransform = newTransform * glm::inverse(getWorldTransform());
  // set the new global transform
  m_globalTransform = newTransform;
  //m_localTransform = relTransform * m_localTransform

//  if (nullptr != m_parent) {
    // update local transform based on new global transform and parent's world transform
//    m_localTransform = newTransform * glm::inverse(m_parent->getWorldTransform());
//  }
  for (auto &pair: m_children) {
    // update global transform of children based on relative transform
    pair.second->m_globalTransform = newTransform;
  }
}

void Node::addChild(std::shared_ptr<Node> child) {
  m_children.emplace(child->getName(), child);
  child->m_globalTransform = getWorldTransform();
}

std::shared_ptr<Node> Node::removeChild(const std::string &name) {
  auto iter = m_children.find(name);

  if (iter != m_children.end()) {
    std::shared_ptr<Node> value = std::move(iter->second);
    m_children.erase(iter);
    return value;
  }
  return nullptr;
}

void Node::render(std::map<std::string, shader_program> m_shaders, glm::mat4 const& view_transform) {
  for (auto& pair : m_children) {
    pair.second->render(m_shaders, view_transform);
  }
}
