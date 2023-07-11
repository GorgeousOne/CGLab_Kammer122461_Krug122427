#ifndef OPENGL_FRAMEWORK_SCENE_GRAPH_HPP
#define OPENGL_FRAMEWORK_SCENE_GRAPH_HPP

#include <string>
#include <memory>
#include <iostream>
#include "node.hpp"

//Assignment 1.1 - implementing a scene graph
class SceneGraph {

public:
  //delete copy constructor to prevent singleton from being copied
  SceneGraph(SceneGraph const&) = delete;

  // returns reference to singleton scene graph instance
  // great explanation by The Cherno: https://youtu.be/PPup1yeU45I?t=446
  static SceneGraph& get() {
    // instantiates a static member only on first call
    static SceneGraph instance;
    return instance;
  }

  std::string const& getName() const;
  void setName(std::string const& name);
  std::shared_ptr<Node> const& getRoot() const;
  void setRoot(std::shared_ptr<Node> const& root);
  friend std::ostream& operator<<(std::ostream& os, const SceneGraph &graph);
  ~SceneGraph();

private:
  SceneGraph() :
      name{"scene"},
      root{std::make_shared<Node>("root")} {}

  std::string name;
  std::shared_ptr<Node> root;
};

#endif //OPENGL_FRAMEWORK_SCENE_GRAPH_HPP