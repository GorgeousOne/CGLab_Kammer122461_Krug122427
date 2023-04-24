#ifndef OPENGL_FRAMEWORK_NODE_HPP
#define OPENGL_FRAMEWORK_NODE_HPP

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <map>

class Node {
public:
  Node(std::string const& name, std::shared_ptr<Node> parent);

  std::shared_ptr<Node> getParent();
  void setParent(std::shared_ptr<Node> node);
  std::shared_ptr<Node> getChild(std::string const& name);
  std::map<std::string, std::shared_ptr<Node>> const& getChildrenList();
  std::string const& getName();
  std::string const& getPath();
  int getDepth();
  glm::mat4 getLocalTransform();
  void setLocalTransform(glm::mat4 const& newTransform);
  glm::mat4 const& getWorldTransform();
  void setWorldTransform(glm::mat4 const& newTransform);
  void addChildren(std::shared_ptr<Node>);
  std::shared_ptr<Node> removeChildren(std::string const& name);

private:
  std::shared_ptr<Node> m_parent;
  std::map<std::string, std::shared_ptr<Node>> m_children;
  std::string m_name;
  std::string m_path;
  int m_depth;
  glm::mat4 m_localTransform;
  glm::mat4 m_globalTransform;
};
#endif //OPENGL_FRAMEWORK_NODE_HPP

