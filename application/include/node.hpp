#ifndef OPENGL_FRAMEWORK_NODE_HPP
#define OPENGL_FRAMEWORK_NODE_HPP

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <map>
#include "structs.hpp"

class Node {
public:
  // constructor
  Node(std::string const& name);
  // get the parent node
  std::shared_ptr<Node> getParent();
  // set the parent node
  void setParent(std::shared_ptr<Node> node);
  // get a child node by name
  std::shared_ptr<Node> getChild(std::string const& name);
  // get a map of children nodes
  std::map<std::string, std::shared_ptr<Node>> const& getChildrenList();
  // get the name of the node
  std::string const& getName();
  // get the path of the node
  std::string const& getPath();
  // get the depth of the node in the tree
  int getDepth();
  // get the local transform matrix of the node
  glm::mat4 getLocalTransform();
  // set the local transform matrix of the node
  void setLocalTransform(glm::mat4 const& newTransform);
  // get the world transform matrix of the node
  glm::mat4 const& getWorldTransform();
  // set the world transform matrix of the node
  void setWorldTransform(glm::mat4 const& newTransform);
  // add a child node
  void addChild(std::shared_ptr<Node>);
  // remove a child node by name
  std::shared_ptr<Node> removeChild(std::string const& name);
  virtual void render(std::map<std::string, shader_program> m_shaders, glm::mat4 const& view_transform);

private:
  // parent node
  std::shared_ptr<Node> m_parent;
  // map of child nodes
  std::map<std::string, std::shared_ptr<Node>> m_children;
  // name of the node
  std::string m_name;
  // path of the node
  std::string m_path;
  // depth of the node in the tree
  int m_depth;
  // local transform matrix of the node
  glm::mat4 m_localTransform;
  // world transform matrix of the node
  glm::mat4 m_globalTransform;
};
#endif //OPENGL_FRAMEWORK_NODE_HPP
