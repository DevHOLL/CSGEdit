#include <sstream>

#include "Exception.hpp"
#include "Math.hpp"
#include "OcclusionLight.hpp"
#include "Scene.hpp"

RT::OcclusionLight::OcclusionLight(RT::Color const & color, double radius)
  : _color(color), _radius(radius)
{
  if (_radius < 0.f)
    throw RT::Exception(std::string(__FILE__) + ": l." + std::to_string(__LINE__));
}

RT::OcclusionLight::~OcclusionLight()
{}

RT::Color RT::OcclusionLight::preview(RT::Scene const * scene, RT::Ray const & ray, RT::Intersection const & intersection) const
{
  return intersection.material.color * intersection.material.light.ambient * _color;
}

RT::Color RT::OcclusionLight::render(RT::Scene const * scene, RT::Ray const & ray, RT::Intersection const & intersection) const
{
  // If no ambient light, stop
  if (_color == 0.f || intersection.material.light.ambient == 0.f || intersection.material.transparency.intensity == 1.f || intersection.material.reflection.intensity == 1.f)
    return RT::Color(0.f);

  // If quality to basic
  if (intersection.material.light.quality <= 1 || _radius == 0.f)
    return intersection.material.color * intersection.material.light.ambient * (1.f - intersection.material.transparency.intensity) * (1.f - intersection.material.reflection.intensity);

  // Inverse normal if necessary
  RT::Ray	n = intersection.normal;
  if (RT::Ray::cos(ray, intersection.normal) > 0)
    n.d() *= -1;

  // Calculate rotation angles of normal
  double	ry = -std::asin(n.d().z());
  double	rz = 0;

  if (n.d().x() != 0 || n.d().y() != 0)
    rz = n.d().y() > 0 ?
    +std::acos(n.d().x() / std::sqrt(n.d().x() * n.d().x() + n.d().y() * n.d().y())) :
    -std::acos(n.d().x() / std::sqrt(n.d().x() * n.d().x() + n.d().y() * n.d().y()));
  
  // Rotation matrix to get ray to point of view
  Math::Matrix<4, 4>  matrix = Math::Matrix<4, 4>::rotation(0, Math::Utils::RadToDeg(ry), Math::Utils::RadToDeg(rz));

  // Point origin right above intersection
  RT::Ray	occ;
  
  occ.p() = n.p() + n.d() * Math::Shift;
  
  // Generate and render occlusion rays
  unsigned int	nb_ray = 0;
  RT::Color	ambient = 0.f;

  for (double a = Math::Random::rand(Math::Pi / (2.f * intersection.material.light.quality)); a < Math::Pi / 2.f; a += Math::Pi / (2.f * intersection.material.light.quality))
    for (double b = Math::Random::rand(2.f * Math::Pi / (std::cos(a) * intersection.material.light.quality * 2.f + 1.f)); b < 2.f * Math::Pi; b += (2.f * Math::Pi) / (std::cos(a) * intersection.material.light.quality * 2.f + 1.f))
    {
      // Calculate ray according to point on the hemisphere
      occ.d().x() = std::sin(a);
      occ.d().y() = std::cos(b) * std::cos(a);
      occ.d().z() = std::sin(b) * std::cos(a);
      occ.d() = matrix * occ.d();
      
      std::list<RT::Intersection> intersect = scene->tree()->render(occ.normalize());
      
      // Occlusion above
      if (intersection.material.transparency.intensity != 1.f)
      {
	std::list<RT::Intersection>::const_iterator it = intersect.begin();
	RT::Color				    light = RT::Color(_color);

	while (it != intersect.end() && +it->distance < 0.f)
	  it++;
	while (it != intersect.end() && +it->distance < _radius && light != RT::Color(0.f))
	{
	  // NOTE: occlusion ray is not reflected
	  ambient += RT::Color(+it->distance / _radius) * light * it->material.color * (1.f - it->material.transparency.intensity) * (1.f - intersection.material.transparency.intensity);
	  light *= it->material.color * it->material.transparency.intensity;
	  it++;
	}
	ambient += light * (1.f - intersection.material.transparency.intensity);
      }

      // Occlusion transparency
      if (intersection.material.transparency.intensity != 0.f)
      {
	std::list<RT::Intersection>::const_reverse_iterator it = intersect.rbegin();
	RT::Color					    light = RT::Color(_color);
	
	while (it != intersect.rend() && -it->distance < 0.f)
	  it++;
	while (it != intersect.rend() && -it->distance < _radius && light != RT::Color(0.f))
	{
	  // NOTE: occlusion ray is not reflected
	  ambient += RT::Color(-it->distance / _radius) * light * it->material.color * (1.f - it->material.transparency.intensity) * (intersection.material.transparency.intensity);
	  light *= it->material.color * it->material.transparency.intensity;
	  it++;
	}
	ambient += light * (intersection.material.transparency.intensity);
      }

      nb_ray++;
    }

  return ambient / nb_ray * intersection.material.color * intersection.material.light.ambient * (1.f - intersection.material.transparency.intensity) * (1.f - intersection.material.reflection.intensity);
}

std::string	RT::OcclusionLight::dump() const
{
  std::stringstream	stream;

  stream << "occlusion_light(" << _color.dump() << ", " << _radius << ");";

  return stream.str();
}