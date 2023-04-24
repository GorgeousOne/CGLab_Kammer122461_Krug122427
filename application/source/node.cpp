#include "node.hpp"


Node::Node(std::string const& name, std::shared_ptr<Node> parent) :
    m_parent{parent},
    m_name{name},
    m_localTransform{glm::mat4()},
    m_children{std::map<std::string, std::shared_ptr<Node>>{}} {

  if (nullptr != parent) {
    m_path = parent->getPath() + parent->getName();
    m_depth = parent->getDepth() + 1;
    m_globalTransform = parent->getWorldTransform();

  } else {
    m_path = "";
    m_depth = 0;
    m_globalTransform = glm::mat4();
  }
}

std::shared_ptr<Node> Node::getParent() {
  return std::shared_ptr<Node>();
}

void Node::setParent(std::shared_ptr<Node> node) {
  m_parent = node;
  m_globalTransform = m_parent->getWorldTransform() * m_localTransform;
}

std::shared_ptr<Node> Node::getChild(std::string const& name) {
  auto iter = m_children.find(name);

  if (iter != m_children.end()) {
    return iter->second;
  }
  return nullptr;
}

std::map<std::string, std::shared_ptr<Node>> const &Node::getChildrenList() {
  return m_children;
}

std::string const &Node::getName() {
  return m_name;
}

std::string const &Node::getPath() {
  return m_path;
}

int Node::getDepth() {
  return m_depth;
}

glm::mat4 Node::getLocalTransform() {
  return m_localTransform;
}

void Node::setLocalTransform(const glm::mat4 &newTransform) {
  glm::mat4 relTransform = newTransform * glm::inverse(m_localTransform);
  m_localTransform = newTransform;

  for (auto& pair: m_children) {
    pair.second->m_globalTransform *= relTransform;
  }
}

glm::mat4 const &Node::getWorldTransform() {
  return m_globalTransform;
}

void Node::setWorldTransform(glm::mat4 const& newTransform) {
  glm::mat4 relTransform = newTransform * glm::inverse(m_globalTransform);
  m_globalTransform = newTransform;

  if (nullptr != m_parent) {
    m_localTransform = newTransform * glm::inverse(m_parent->getWorldTransform());
  }
  for (auto &pair: m_children) {
    pair.second->m_globalTransform *= relTransform;
  }
}

void Node::addChildren(std::shared_ptr<Node> child) {
  m_children.emplace(child->getName(), child);
}

std::shared_ptr<Node> Node::removeChildren(const std::string &name) {
  auto iter = m_children.find(name);

  if (iter != m_children.end()) {
    std::shared_ptr<Node> value = std::move(iter->second);
    m_children.erase(iter);
    return value;
  }
  return nullptr;
}


