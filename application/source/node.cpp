#include "node.hpp"


Node::Node(std::string const& name, std::shared_ptr<Node> parent) :
    m_name{name},
    m_path{parent->getPath() + name},
    m_parent{parent},
    m_depth{parent->getDepth() + 1},
    m_localTransform{glm::mat4()},
    m_globalTransform{parent->getWorldTransform()}
    {


};

std::shared_ptr<Node> Node::getParent() {

  return std::shared_ptr<Node>();
}

void Node::setParent(std::shared_ptr<Node> node) {
  m_parent = node;
}

std::shared_ptr<Node> Node::getChild(const std::string &name) {
  return std::shared_ptr<Node>();
}

std::vector<std::shared_ptr<Node>> const &Node::getChildrenList() {
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
  return glm::mat4();
}

void Node::setLocalTransform(const glm::mat4 &mat) {

}

glm::mat4 const &Node::getWorldTransform() {
  return m_globalTransform;
}

void Node::setWorldTransform(glm::mat4 const& mat) {
  m_globalTransform
}

void Node::addChildren(std::shared_ptr<Node>) {

}

std::shared_ptr<Node> Node::removeChildren(const std::string &name) {
  return nullptr;
}


