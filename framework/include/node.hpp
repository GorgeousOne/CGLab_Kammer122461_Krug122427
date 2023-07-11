#ifndef OPENGL_FRAMEWORK_NODE_HPP
#define OPENGL_FRAMEWORK_NODE_HPP

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include "structs.hpp"

//Assignment 1.1 - implementing a scene graph
class Node {
  public:
  //constructor
  Node(std::string const& newName);
  //get the name of the node
  std::string const& getName() const;
  //set the parent node
  void setParent(std::shared_ptr<Node> node);
  //get the parent node
  std::shared_ptr<Node> getParent() const;
  //get the path of the node
  std::string const& getPath() const;
  //get the depth of the node in the tree
  int getDepth() const;
  //get a child node by childName
  std::shared_ptr<Node> getChild(std::string const& childName) const;
  //add a child node
  void addChild(std::shared_ptr<Node>);
  //remove a child node by childName
  std::shared_ptr<Node> removeChild(std::string const& childName);
  //get a map of children nodes
  std::map<std::string, std::shared_ptr<Node>> const& getChildren();
  //get the local transform matrix of the node
  glm::mat4 getLocalTransform() const;
  //set the local transform matrix of the node
  void setLocalTransform(glm::mat4 const& newTransform);
  //get the world transform matrix of the node
  glm::mat4 getWorldTransform() const;
  //set the world transform matrix of the node
  void setWorldTransform(glm::mat4 const& newTransform);

  virtual void render(std::map<std::string, shader_program> const& shaders);

  void traverse(std::function<void(std::shared_ptr<Node>)> func);
  //print information of node on the console
  friend std::ostream& operator<<(std::ostream& os, Node const& node);

protected:
  //name of the node
  std::string name;
  //parent node
  std::shared_ptr<Node> parent;
  //path of the node
  std::string path;
  //number of parents
  int depth;
  //map of child nodes
  std::map<std::string, std::shared_ptr<Node>> children;
  //local transform matrix of the node
  glm::mat4 localTransform;
  //world transform matrix of the node
  glm::mat4 worldTransform;
};
#endif //OPENGL_FRAMEWORK_NODE_HPP