#ifndef OPENGL_FRAMEWORK_PLANET_HPP
#define OPENGL_FRAMEWORK_PLANET_HPP

//struct to store planet information
struct Planet {
  //scale of the planet model
  float diameter;
  //distance from sun (or respective planet) that is being orbited
  float orbitRadius;
  //duration for one orbit in seconds
  float orbitPeriod;
  //duration of rotation around self in seconds
  float rotationPeriod;
  glm::fvec3 color;
};
#endif //OPENGL_FRAMEWORK_PLANET_HPP
