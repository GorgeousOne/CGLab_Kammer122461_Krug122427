#ifndef OPENGL_FRAMEWORK_SCENE_GRAPH_HPP
#define OPENGL_FRAMEWORK_SCENE_GRAPH_HPP

#include <string>
#include <memory>
#include <iostream>
#include "node.hpp"
#include "camera_node.hpp"

class SceneGraph {
public:
  // prevent sinletong instance from being copied
  SceneGraph(SceneGraph const&) = delete;

  // returns reference to static scene graph instance
  static SceneGraph& get() {
    // instantiates a static member on first call
    static SceneGraph instance;
    return instance;
  }
  std::string const& getName() {
    return m_name;
  };

  std::shared_ptr<Node> getRoot() {
    return m_root;
  }

  std::string printGraph() {
    m_root->iterate([] (Node& node) -> void {
      std::cout << std::string(2 * node.getDepth(), ' ') << node.getName() << std::endl;
    });
  }

private:
  SceneGraph() :
      m_name{"scene"},
      m_root{std::make_shared<Node>("root")} {}

  std::string m_name;
  std::shared_ptr<Node> m_root;
};


#endif //OPENGL_FRAMEWORK_SCENE_GRAPH_HPP
