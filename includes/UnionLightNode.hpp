#ifndef _UNION_LIGHT_NODE_HPP_
#define _UNION_LIGHT_NODE_HPP_

#include "AbstractLightNode.hpp"

namespace RT
{
  class UnionLightNode : public RT::AbstractLightNode
  {
  public:
    UnionLightNode();
    virtual ~UnionLightNode();

    virtual RT::Color	preview(Math::Matrix<4, 4> const &, RT::Scene const *, RT::Ray const &, RT::Intersection const &, unsigned int, unsigned int) const override;	// Return preview lighting for given intersection
    virtual RT::Color	render(Math::Matrix<4, 4> const &, RT::Scene const *, RT::Ray const &, RT::Intersection const &, unsigned int, unsigned int) const override;	// Return rendered lighting for given intersection
  };
};

#endif
