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
  return m_parent;
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
  // return name of the node
  return m_name;
}

std::string const &Node::getPath() {
  // return path of the node
  return m_path;
}

int Node::getDepth() {
  // return depth of the node
  return m_depth;
}

glm::mat4 Node::getLocalTransform() {
  // return the local transform of the node
  return m_localTransform;
}

void Node::setLocalTransform(const glm::mat4 &newTransform) {
  m_localTransform = newTransform;

  for (auto& pair: m_children) {
    // update global transform of children based on relative transform
    pair.second->m_globalTransform = getWorldTransform();
  }
}

glm::mat4 Node::getWorldTransform() {
  /* return the world transform of the node as a combination of global transform (mostly influenced by parent node)
    and local transform */
  return m_globalTransform * m_localTransform;
}

void Node::setWorldTransform(glm::mat4 const& newTransform) {
  // set the new global transform
  m_globalTransform = newTransform;

  for (auto &pair: m_children) {
    // update global transform of children
    pair.second->m_globalTransform = getWorldTransform();
  }
}

void Node::addChild(std::shared_ptr<Node> child) {
  //add node as child object that gets transformed with parent node together
  m_children.emplace(child->getName(), child);
  child->m_globalTransform = getWorldTransform();
  child->m_depth = m_depth + 1;
}

std::shared_ptr<Node> Node::removeChild(const std::string &name) {
  //find and erase the child
  auto iter = m_children.find(name);

  if (iter != m_children.end()) {
    std::shared_ptr<Node> value = std::move(iter->second);
    m_children.erase(iter);
    return value;
  }
  return nullptr;
}

void Node::render(std::map<std::string, shader_program> const& m_shaders, glm::mat4 const& view_transform) {
  //render nothing by default and only call render on children
  for (auto& pair : m_children) {
    pair.second->render(m_shaders, view_transform);
  }
}

void Node::iterate(std::function<void(std::shared_ptr<Node> node)> func) {
  //execute the passed function once with each child as parameter, then repeat down the node tree
  for (auto& pair : m_children) {
    func(pair.second);
    pair.second->iterate(func);
  }
}

// print the names of the objects in the scene graph
void Node::printGraph(std::ostream& os) {
  // print the name of the node with indentation based on depth
  for (int i = 0; i < m_depth; ++i) {
    os << "  ";
  }
  os << m_name << std::endl;
  for (auto& pair : m_children) {
    pair.second->printGraph(os);
  }
}
