#include <unordered_map>

#include "MaterialCsgNode.hpp"

RT::MaterialCsgNode::MaterialCsgNode(RT::Material const & material)
  : _material(material)
{}

RT::MaterialCsgNode::~MaterialCsgNode()
{}

std::list<RT::Intersection> RT::MaterialCsgNode::renderChildren(RT::Ray const & ray, unsigned int deph) const
{
  std::list<RT::Intersection> intersect;

  // Iterate through sub-tree to get intersections
  for (RT::AbstractCsgTree const * it : _children)
    intersect.merge(it->render(ray, deph));

  std::unordered_map<RT::AbstractCsgTree const *, bool>	inside;
  std::list<RT::Intersection>				result;
  unsigned int						state = 0;

  // Iterate through intersections
  for (RT::Intersection const & it : intersect)
  {
    // If currently outside, push intersection
    if (state == 0)
      result.push_back(it);

    // Increment deepness if getting inside an object, decrement if getting outside
    state += inside[it.node] ? -1 : +1;
    inside[it.node] = !(inside[it.node]);

    // If currently outside, push intersection
    if (state == 0)
      result.push_back(it);
  }

  // Atribute intersections and apply material to children
  for (RT::Intersection & it : result)
    it.material *= _material;

  return result;
}