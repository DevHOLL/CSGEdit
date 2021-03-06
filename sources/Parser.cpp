#include <chaiscript/chaiscript.hpp>
#include <chaiscript/chaiscript_stdlib.hpp>
#include <stdexcept>
#include <functional>
#include <iostream>

#include "Config.hpp"
#include "Parser.hpp"
#include "SceneLibrary.hpp"

// CSG tree
#include "EmptyCsgTree.hpp"
#include "ExternCsgTree.hpp"
#include "BoundingCsgNode.hpp"
#include "DephCsgNode.hpp"
#include "DifferenceCsgNode.hpp"
#include "IntersectionCsgNode.hpp"
#include "MaterialCsgNode.hpp"
#include "MeshCsgNode.hpp"
#include "TransformationCsgNode.hpp"
#include "UnionCsgNode.hpp"
#include "BoxCsgLeaf.hpp"
#include "ConeCsgLeaf.hpp"
#include "MobiusCsgLeaf.hpp"
#include "SphereCsgLeaf.hpp"
#include "TangleCsgLeaf.hpp"
#include "TorusCsgLeaf.hpp"
#include "TriangleCsgLeaf.hpp"

// Light tree
#include "EmptyLightTree.hpp"
#include "ExternLightTree.hpp"
#include "DephLightNode.hpp"
#include "TransformationLightNode.hpp"
#include "UnionLightNode.hpp"
#include "DirectionalLightLeaf.hpp"
#include "OcclusionLightLeaf.hpp"
#include "PointLightLeaf.hpp"

RT::Parser::Parser()
  : _scope({ { new RT::UnionCsgNode(), new RT::UnionLightNode() } }), _files(), _script(chaiscript::Std_Lib::library()), _scene()
{
  // Set up script parser
  // Scope CSG
  _script.add(chaiscript::fun(&RT::Parser::scopeDifference, this), "difference");
  _script.add(chaiscript::fun(&RT::Parser::scopeIntersection, this), "intersection");
  _script.add(chaiscript::fun(&RT::Parser::scopeUnion, this), "union");
  // Scope transformations
  _script.add(chaiscript::fun(&RT::Parser::scopeTransformation, this), "transformation");
  _script.add(chaiscript::fun(&RT::Parser::scopeTranslation, this), "translation");
  _script.add(chaiscript::fun(&RT::Parser::scopeMirror, this), "mirror");
  _script.add(chaiscript::fun(&RT::Parser::scopeRotation, this), "rotation");
  _script.add(chaiscript::fun(&RT::Parser::scopeScale, this), "scale");
  _script.add(chaiscript::fun(&RT::Parser::scopeShear, this), "shear");
  // Scope materials
  _script.add(chaiscript::fun(&RT::Parser::scopeMaterial, this), "material");
  _script.add(chaiscript::fun(&RT::Parser::scopeColor, this), "color");
  _script.add(chaiscript::fun(std::function<void(double, double, double)>(std::bind(&RT::Parser::scopeDirect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, 1.f, RT::Config::Material::Quality))), "direct");
  _script.add(chaiscript::fun(std::function<void(double, double, double, double)>(std::bind(&RT::Parser::scopeDirect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, RT::Config::Material::Quality))), "direct");
  _script.add(chaiscript::fun(std::function<void(double, double, double, double, unsigned int)>(std::bind(&RT::Parser::scopeDirect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5))), "direct");
  _script.add(chaiscript::fun(std::function<void(double, double, double)>(std::bind(&RT::Parser::scopeIndirect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, 1.f, RT::Config::Material::Quality))), "indirect");
  _script.add(chaiscript::fun(std::function<void(double, double, double, double)>(std::bind(&RT::Parser::scopeIndirect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, RT::Config::Material::Quality))), "indirect");
  _script.add(chaiscript::fun(std::function<void(double, double, double, double, unsigned int)>(std::bind(&RT::Parser::scopeIndirect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5))), "indirect");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::scopeTransparency, this, std::placeholders::_1, 1.f, 0.f, RT::Config::Material::Quality))), "transparency");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::scopeTransparency, this, std::placeholders::_1, std::placeholders::_2, 0.f, RT::Config::Material::Quality))), "transparency");
  _script.add(chaiscript::fun(std::function<void(double, double, double)>(std::bind(&RT::Parser::scopeTransparency, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, RT::Config::Material::Quality))), "transparency");
  _script.add(chaiscript::fun(std::function<void(double, double, double, unsigned int)>(std::bind(&RT::Parser::scopeTransparency, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))), "transparency");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::scopeReflection, this, std::placeholders::_1, 0.f, RT::Config::Material::Quality))), "reflection");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::scopeReflection, this, std::placeholders::_1, std::placeholders::_2, RT::Config::Material::Quality))), "reflection");
  _script.add(chaiscript::fun(std::function<void(double, double, unsigned int)>(std::bind(&RT::Parser::scopeReflection, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))), "reflection");
  // Scope others
  _script.add(chaiscript::fun(&RT::Parser::scopeBounding, this), "bounding");
  _script.add(chaiscript::fun(&RT::Parser::scopeMesh, this), "mesh");
  _script.add(chaiscript::fun(&RT::Parser::scopeDeph, this), "maxdeph");
  _script.add(chaiscript::fun(&RT::Parser::scopeDephCsg, this), "maxdeph_csg");
  _script.add(chaiscript::fun(&RT::Parser::scopeDephLight, this), "maxdeph_light");
  // Scope utilities
  _script.add(chaiscript::fun(&RT::Parser::scopeEnd, this), "end");
  // Primitives
  _script.add(chaiscript::fun(std::function<void()>(std::bind(&RT::Parser::primitiveBox, this, 1.f, 1.f, 1.f, false))), "box");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::primitiveBox, this, std::placeholders::_1, std::placeholders::_1, std::placeholders::_1, false))), "box");
  _script.add(chaiscript::fun(std::function<void(double, bool)>(std::bind(&RT::Parser::primitiveBox, this, std::placeholders::_1, std::placeholders::_1, std::placeholders::_1, std::placeholders::_2))), "box");
  _script.add(chaiscript::fun(std::function<void(double, double, double)>(std::bind(&RT::Parser::primitiveBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, false))), "box");
  _script.add(chaiscript::fun(std::function<void(double, double, double, bool)>(std::bind(&RT::Parser::primitiveBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))), "box");
  _script.add(chaiscript::fun(std::function<void()>(std::bind(&RT::Parser::primitiveCone, this, 0.5f, 0.f, 1.f, false))), "cone");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::primitiveCone, this, std::placeholders::_1, 0.f, 1.f, false))), "cone");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::primitiveCone, this, std::placeholders::_1, std::placeholders::_2, 1.f, false))), "cone");
  _script.add(chaiscript::fun(std::function<void(double, double, double)>(std::bind(&RT::Parser::primitiveCone, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, false))), "cone");
  _script.add(chaiscript::fun(std::function<void(double, double, double, bool)>(std::bind(&RT::Parser::primitiveCone, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))), "cone");
  _script.add(chaiscript::fun(std::function<void()>(std::bind(&RT::Parser::primitiveCone, this, 0.5f, 0.5f, 1.f, false))), "cylinder");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::primitiveCone, this, std::placeholders::_1, std::placeholders::_1, 1.f, false))), "cylinder");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::primitiveCone, this, std::placeholders::_1, std::placeholders::_1, std::placeholders::_2, false))), "cylinder");
  _script.add(chaiscript::fun(std::function<void(double, double, bool)>(std::bind(&RT::Parser::primitiveCone, this, std::placeholders::_1, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))), "cylinder");
  _script.add(chaiscript::fun(std::function<void()>(std::bind(&RT::Parser::primitiveMobius, this, 0.5f, 0.5f / 4.f, Math::Shift))), "mobius");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::primitiveMobius, this, std::placeholders::_1, std::placeholders::_2, Math::Shift))), "mobius");
  _script.add(chaiscript::fun(std::function<void(double, double, double)>(std::bind(&RT::Parser::primitiveMobius, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))), "mobius");
  _script.add(chaiscript::fun(std::function<void()>(std::bind(&RT::Parser::primitiveSphere, this, 0.5f))), "sphere");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::primitiveSphere, this, std::placeholders::_1))), "sphere");
  _script.add(chaiscript::fun(std::function<void()>(std::bind(&RT::Parser::primitiveTangle, this, 11.8f))), "tangle");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::primitiveTangle, this, std::placeholders::_1))), "tangle");
  _script.add(chaiscript::fun(std::function<void()>(std::bind(&RT::Parser::primitiveTorus, this, 1.f, 0.25f))), "torus");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::primitiveTorus, this, std::placeholders::_1, 0.25f))), "torus");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::primitiveTorus, this, std::placeholders::_1, std::placeholders::_2))), "torus");
  _script.add(chaiscript::fun(&RT::Parser::primitiveTriangle, this), "triangle");
  // Light
  _script.add(chaiscript::fun(std::function<void(RT::Color const &)>(std::bind(&RT::Parser::lightDirectional, this, std::placeholders::_1, 0.f))), "directional_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &, double)>(std::bind(&RT::Parser::lightDirectional, this, std::placeholders::_1, std::placeholders::_2))), "directional_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &)>(std::bind(&RT::Parser::lightOcclusion, this, std::placeholders::_1, 0.f))), "ambient_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &)>(std::bind(&RT::Parser::lightOcclusion, this, std::placeholders::_1, 0.f))), "occlusion_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &, double)>(std::bind(&RT::Parser::lightOcclusion, this, std::placeholders::_1, std::placeholders::_2))), "occlusion_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &)>(std::bind(&RT::Parser::lightPoint, this, std::placeholders::_1, 0.f, 0.f, 0.f, 0.f))), "point_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &, double)>(std::bind(&RT::Parser::lightPoint, this, std::placeholders::_1, std::placeholders::_2, 0.f, 0.f, 0.f))), "point_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &, double, double)>(std::bind(&RT::Parser::lightPoint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, 0.f, 0.f))), "point_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &, double, double, double)>(std::bind(&RT::Parser::lightPoint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_4))), "point_light");
  _script.add(chaiscript::fun(std::function<void(RT::Color const &, double, double, double, double)>(std::bind(&RT::Parser::lightPoint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5))), "point_light");
  // Settings
  _script.add(chaiscript::fun(&RT::Parser::settingCamera, this), "camera");
  _script.add(chaiscript::fun(&RT::Parser::settingResolution, this), "resolution");
  _script.add(chaiscript::fun(std::function<void(unsigned int)>(std::bind(&RT::Parser::settingAntiAliasing, this, std::placeholders::_1, 0))), "antialiasing");
  _script.add(chaiscript::fun(std::function<void(unsigned int, unsigned int)>(std::bind(&RT::Parser::settingAntiAliasing, this, std::placeholders::_1, std::placeholders::_2))), "antialiasing");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::settingDephOfField, this, std::placeholders::_1, std::placeholders::_2, RT::Config::DephOfField::Quality))), "deph_of_field");
  _script.add(chaiscript::fun(std::function<void(double, double, unsigned int)>(std::bind(&RT::Parser::settingDephOfField, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))), "deph_of_field");
  _script.add(chaiscript::fun(std::function<void(double)>(std::bind(&RT::Parser::settingVirtualReality, this, std::placeholders::_1, RT::Config::VirtualReality::Distortion, RT::Config::VirtualReality::Center))), "virtual_reality");
  _script.add(chaiscript::fun(std::function<void(double, double)>(std::bind(&RT::Parser::settingVirtualReality, this, std::placeholders::_1, std::placeholders::_2, RT::Config::VirtualReality::Center))), "virtual_reality");
  _script.add(chaiscript::fun(std::function<void(double, double, double)>(std::bind(&RT::Parser::settingVirtualReality, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))), "virtual_reality");
  _script.add(chaiscript::fun(std::function<void(unsigned int)>(std::bind(&RT::Parser::settingThread, this, std::placeholders::_1))), "thread");
  // Others
  _script.add(chaiscript::fun(&RT::Parser::import, this), "import");
  _script.add(chaiscript::fun(&RT::Parser::import, this), "mesh");
  _script.add(chaiscript::fun(&RT::Parser::include, this), "include");
  // Math functions
  _script.add(chaiscript::fun((double(*)(double))(&std::cos)), "cos");
  _script.add(chaiscript::fun((double(*)(double))(&std::sin)), "sin");
  _script.add(chaiscript::fun((double(*)(double))(&std::tan)), "tan");
  _script.add(chaiscript::fun((double(*)(double))(&std::acos)), "acos");
  _script.add(chaiscript::fun((double(*)(double))(&std::asin)), "asin");
  _script.add(chaiscript::fun((double(*)(double))(&std::atan)), "atan");
  _script.add(chaiscript::fun((double(*)(double))(&std::exp)), "exp");
  _script.add(chaiscript::fun((double(*)(double))(&std::log)), "log");
  _script.add(chaiscript::fun((double(*)(double))(&std::log10)), "log10");
  _script.add(chaiscript::fun((double(*)(double))(&std::sqrt)), "sqrt");
  _script.add(chaiscript::fun((double(*)(double))(&std::cbrt)), "cbrt");
  _script.add(chaiscript::fun((double(*)(double, double))(&std::pow)), "pow");
  _script.add(chaiscript::fun((double(*)(double))(&Math::Random::rand)), "rand");
  _script.add_global_const(chaiscript::const_var(Math::Pi), "pi");

  // Vector conversion: std::vector<double>
  _script.add(chaiscript::type_conversion<std::vector<chaiscript::Boxed_Value>, std::vector<double>>(
    [](const std::vector<chaiscript::Boxed_Value> & v)
  {
    std::vector<double>	result;
    for (chaiscript::Boxed_Value const & it : v)
      result.push_back(chaiscript::Boxed_Number(it).get_as<double>());
    return result;
  }
  ));

  // Vector conversion: std::vector<std::vector<double>>
  _script.add(chaiscript::type_conversion<std::vector<chaiscript::Boxed_Value>, std::vector<std::vector<double>>>(
    [](const std::vector<chaiscript::Boxed_Value> & v)
  {
    std::vector<std::vector<double>>	result;

    for (chaiscript::Boxed_Value const & it1 : v)
    {
      result.push_back(std::vector<double>());
      for (chaiscript::Boxed_Value const & it2 : chaiscript::boxed_cast<std::vector<chaiscript::Boxed_Value>>(it1))
	result.back().push_back(chaiscript::Boxed_Number(it2).get_as<double>());
    }

    return result;
  }
  ));

  // Vector conversion: RT::Color
  _script.add(chaiscript::type_conversion<std::vector<chaiscript::Boxed_Value>, RT::Color>(
    [](const std::vector<chaiscript::Boxed_Value> & v)
  {
    if (v.size() == 1)
      return RT::Color(chaiscript::Boxed_Number(v[0]).get_as<double>() / 255.f, chaiscript::Boxed_Number(v[0]).get_as<double>() / 255.f, chaiscript::Boxed_Number(v[0]).get_as<double>() / 255.f);
    else if (v.size() == 3)
      return RT::Color(chaiscript::Boxed_Number(v[0]).get_as<double>() / 255.f, chaiscript::Boxed_Number(v[1]).get_as<double>() / 255.f, chaiscript::Boxed_Number(v[2]).get_as<double>() / 255.f);
    else
      throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());
  }
  ));

  // Assign CSG / Light node to scene
  _scene.csg() = _scope.top().first;
  _scene.light() = _scope.top().second;
}

RT::Parser::~Parser()
{}

RT::Scene	RT::Parser::load(std::string const & path)
{
  return RT::Parser().parse(path);
}

RT::Scene	RT::Parser::parse(std::string const & path)
{
  // Add file to file scope
  _files.push(path);

  try
  {
    // If file is a mesh
    if (RT::MeshCsgNode::extension(path.substr(path.rfind('.'))))
    {
      // Clean scene
      _scene.clear();

      // Load mesh
      _scene.csg() = new RT::MeshCsgNode(path);
      _scene.light() = new RT::EmptyLightTree();
    }
    // If file is a standard scene description
    else
    {
      // Parsing file
      _script.eval_file(path);

      // Check for invalid scope at end of file
      if (_scope.size() != 1)
	throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

      // Check for empty scene
      if (_scope.top().first->children().empty())
      {
	delete _scene.csg();
	_scene.csg() = new RT::EmptyCsgTree();
      }
      if (_scope.top().second->children().empty())
      {
	delete _scene.light();
	_scene.light() = new RT::EmptyLightTree();
      }
    }
  }
  catch (std::exception e)
  {
    std::cout << "[Parser] ERROR: failed to parse file '" << path << "' (" << std::string(e.what()) << ")." << std::endl;

    // Clean scene
    _scene.clear();
    
    // Generate empty scene configuration
    _scene.csg() = new RT::EmptyCsgTree();
    _scene.light() = new RT::EmptyLightTree();
  }

  return _scene;
}

void	RT::Parser::import(std::string const & path)
{
  _scope.top().first->push(new RT::ExternCsgTree(&RT::SceneLibrary::Instance().get(directory(_files.top()).append(path))->csg()));
  _scope.top().second->push(new RT::ExternLightTree(&RT::SceneLibrary::Instance().get(directory(_files.top()).append(path))->light()));
}

void	RT::Parser::include(std::string const & path)
{
  // Fail if maximum include deph reached
  if (_files.size() > RT::Config::Parser::MaxFileDeph)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  // Parsing file
  _files.push(directory(_files.top()).append(path));
  _script.eval_file(_files.top());
  _files.pop();
}

void	RT::Parser::scopeDifference()
{
  scopeStart(new RT::DifferenceCsgNode());
}

void	RT::Parser::scopeIntersection()
{
  scopeStart(new RT::IntersectionCsgNode());
}

void	RT::Parser::scopeUnion()
{
  scopeStart(new RT::UnionCsgNode());
}

void	RT::Parser::scopeTransformation(std::vector<std::vector<double>> const & v)
{
  Math::Matrix<4, 4>	matrix;

  if (v.size() != 4)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());
  else
    for (unsigned int row = 0; row < 4; row++)
    {
      if (v[row].size() != 4)
	throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());
      else
	for (unsigned int col = 0; col < 4; col++)
	  matrix(row, col) = v[row][col];
    }

  scopeStart(new RT::TransformationCsgNode(matrix), new RT::TransformationLightNode(matrix));
}

void	RT::Parser::scopeTranslation(std::vector<double> const & v)
{
  if (v.size() != 3)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  scopeStart(new RT::TransformationCsgNode(Math::Matrix<4, 4>::translation(v[0], v[1], v[2])), new RT::TransformationLightNode(Math::Matrix<4, 4>::translation(v[0], v[1], v[2])));
}

void	RT::Parser::scopeMirror(std::vector<double> const & v)
{
  if (v.size() != 3)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  scopeStart(new RT::TransformationCsgNode(Math::Matrix<4, 4>::reflection(v[0], v[1], v[2])), new RT::TransformationLightNode(Math::Matrix<4, 4>::reflection(v[0], v[1], v[2])));
}

void	RT::Parser::scopeRotation(std::vector<double> const & v)
{
  if (v.size() != 3)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  scopeStart(new RT::TransformationCsgNode(Math::Matrix<4, 4>::rotation(v[0], v[1], v[2])), new RT::TransformationLightNode(Math::Matrix<4, 4>::rotation(v[0], v[1], v[2])));
}

void	RT::Parser::scopeScale(std::vector<double> const & v)
{
  if (v.size() == 1)
    scopeStart(new RT::TransformationCsgNode(Math::Matrix<4, 4>::scale(v[0], v[0], v[0])), new RT::TransformationLightNode(Math::Matrix<4, 4>::scale(v[0], v[0], v[0])));
  else if (v.size() == 3)
    scopeStart(new RT::TransformationCsgNode(Math::Matrix<4, 4>::scale(v[0], v[1], v[1])), new RT::TransformationLightNode(Math::Matrix<4, 4>::scale(v[0], v[1], v[1])));
  else
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());
}

void	RT::Parser::scopeShear(double xy, double xz, double yx, double yz, double zx, double zy)
{
  scopeStart(new RT::TransformationCsgNode(Math::Matrix<4, 4>::shear(xy, xz, yx, yz, zx, zy)), new RT::TransformationLightNode(Math::Matrix<4, 4>::shear(xy, xz, yx, yz, zx, zy)));
}

void	RT::Parser::scopeMaterial(std::string const & material)
{
  scopeStart(new RT::MaterialCsgNode(RT::Material::getMaterial(material)));
}

void	RT::Parser::scopeColor(RT::Color const & clr)
{
  RT::Material	material;

  material.color = clr;
  scopeStart(new RT::MaterialCsgNode(material));
}

void	RT::Parser::scopeDirect(double emission, double diffuse, double specular, double shininess, unsigned int quality)
{
  RT::Material	material;

  material.direct.emission = emission;
  material.direct.diffuse = diffuse;
  material.direct.specular = specular;
  material.direct.shininess = shininess;
  material.direct.quality = quality;
  scopeStart(new RT::MaterialCsgNode(material));
}

void	RT::Parser::scopeIndirect(double emission, double diffuse, double specular, double shininess, unsigned int quality)
{
  RT::Material	material;

  material.indirect.emission = emission;
  material.indirect.diffuse = diffuse;
  material.indirect.specular = specular;
  material.indirect.shininess = shininess;
  material.indirect.quality = quality;
  scopeStart(new RT::MaterialCsgNode(material));
}

void	RT::Parser::scopeTransparency(double t, double r, double g, unsigned int q)
{
  RT::Material	material;

  material.transparency.intensity = t;
  material.transparency.refraction = r;
  material.transparency.glossiness = g;
  material.transparency.quality = q;
  scopeStart(new RT::MaterialCsgNode(material));
}

void	RT::Parser::scopeReflection(double r, double g, unsigned int q)
{
  RT::Material	material;

  material.reflection.intensity = r;
  material.reflection.glossiness = g;
  material.reflection.quality = q;
  scopeStart(new RT::MaterialCsgNode(material));
}

void	RT::Parser::scopeBounding()
{
  scopeStart(new RT::BoundingCsgNode());
}

void	RT::Parser::scopeMesh()
{
  scopeStart(new RT::MeshCsgNode());
}

void	RT::Parser::scopeDeph(unsigned int deph)
{
  scopeStart(new RT::DephCsgNode(deph), new RT::DephLightNode(deph));
}

void	RT::Parser::scopeDephCsg(unsigned int deph)
{
  scopeStart(new RT::DephCsgNode(deph));
}

void	RT::Parser::scopeDephLight(unsigned int deph)
{
  scopeStart(new RT::DephLightNode(deph));
}

void	RT::Parser::scopeStart(RT::AbstractCsgNode * node)
{
  _scope.top().first->push(node);
  _scope.push({ node, _scope.top().second });
}

void	RT::Parser::scopeStart(RT::AbstractLightNode * node)
{
  _scope.top().second->push(node);
  _scope.push({ _scope.top().first, node });
}

void	RT::Parser::scopeStart(RT::AbstractCsgNode * csg, RT::AbstractLightNode * light)
{
  _scope.top().first->push(csg);
  _scope.top().second->push(light);
  _scope.push({ csg, light });
}

void	RT::Parser::scopeEnd()
{
  if (_scope.size() <= 1)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  RT::AbstractCsgNode *		csg = _scope.top().first;
  RT::AbstractLightNode *	light = _scope.top().second;

  _scope.pop();
  
  if (_scope.top().first != csg)
    if (csg->children().empty())
      _scope.top().first->pop();
    else
      scopeSimplify(csg);

  if (_scope.top().second != light)
    if (light->children().empty())
      _scope.top().second->pop();
    else
      scopeSimplify(light);
}

void	RT::Parser::scopeSimplify(RT::AbstractCsgNode * csg)
{
  // Simplify multiple transformation node
  if (dynamic_cast<RT::TransformationCsgNode *>(csg) != nullptr && csg->children().size() == 1 && dynamic_cast<RT::TransformationCsgNode *>(csg->children().front()) != nullptr)
  {
    dynamic_cast<RT::TransformationCsgNode *>(csg)->transformation() *= dynamic_cast<RT::TransformationCsgNode *>(csg->children().front())->transformation();
    csg->children().splice(csg->children().begin(), dynamic_cast<RT::TransformationCsgNode *>(csg->children().front())->children());
    csg->pop();
    return;
  }

  // Simplify multiple material node
  if (dynamic_cast<RT::MaterialCsgNode *>(csg) != nullptr && csg->children().size() == 1 && dynamic_cast<RT::MaterialCsgNode *>(csg->children().front()) != nullptr)
  {
    dynamic_cast<RT::MaterialCsgNode *>(csg)->material() *= dynamic_cast<RT::MaterialCsgNode *>(csg->children().front())->material();
    csg->children().splice(csg->children().begin(), dynamic_cast<RT::MaterialCsgNode *>(csg->children().front())->children());
    csg->pop();
    return;
  }
}

void	RT::Parser::scopeSimplify(RT::AbstractLightNode * light)
{
  // Simplify multiple transformation node
  if (dynamic_cast<RT::TransformationLightNode *>(light) != nullptr && light->children().size() == 1 && dynamic_cast<RT::TransformationLightNode *>(light->children().front()) != nullptr)
  {
    dynamic_cast<RT::TransformationLightNode *>(light)->transformation() *= dynamic_cast<RT::TransformationLightNode *>(light->children().front())->transformation();
    light->children().splice(light->children().begin(), dynamic_cast<RT::TransformationLightNode *>(light->children().front())->children());
    light->pop();
    return;
  }
}

void	RT::Parser::primitiveBox(double x, double y, double z, bool center)
{
  primitivePush(new RT::BoxCsgLeaf(x, y, z, center));
}

void	RT::Parser::primitiveCone(double r1, double r2, double h, bool center)
{
  primitivePush(new RT::ConeCsgLeaf(r1, r2, h, center));
}

void	RT::Parser::primitiveMobius(double r1, double r2, double t)
{
  primitivePush(new RT::MobiusCsgLeaf(r1, r2, t));
}

void	RT::Parser::primitiveSphere(double r)
{
  primitivePush(new RT::SphereCsgLeaf(r));
}

void	RT::Parser::primitiveTangle(double c)
{
  primitivePush(new RT::TangleCsgLeaf(c));
}

void	RT::Parser::primitiveTorus(double r1, double r2)
{
  primitivePush(new RT::TorusCsgLeaf(r1, r2));
}

void	RT::Parser::primitiveTriangle(std::vector<double> const & p0, std::vector<double> const & p1, std::vector<double> const & p2)
{
  // If not in a mesh scope, error
  if (dynamic_cast<RT::MeshCsgNode *>(_scope.top().first) == nullptr)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  if (p0.size() != 3 || p1.size() != 3 || p2.size() != 3)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  primitivePush(new RT::TriangleCsgLeaf(std::tuple<double, double, double>(p0[0], p0[1], p0[2]), std::tuple<double, double, double>(p1[0], p1[1], p1[2]), std::tuple<double, double, double>(p2[0], p2[1], p2[2])));
}

void	RT::Parser::primitivePush(RT::AbstractCsgTree * tree)
{
  _scope.top().first->push(tree);
}

void	RT::Parser::lightDirectional(RT::Color const & clr, double angle)
{
  lightPush(new RT::DirectionalLightLeaf(clr, angle));
}

void	RT::Parser::lightOcclusion(RT::Color const & clr, double radius)
{
  lightPush(new RT::OcclusionLightLeaf(clr, radius));
}

void	RT::Parser::lightPoint(RT::Color const & clr, double radius, double intensity, double angle1, double angle2)
{
  lightPush(new RT::PointLightLeaf(clr, radius, intensity, angle1, angle2));
}

void	RT::Parser::lightPush(RT::AbstractLightTree * light)
{
  _scope.top().second->push(light);
}

void	RT::Parser::settingCamera(std::vector<double> const & t, std::vector<double> const & r, std::vector<double> const & s)
{
  if (t.size() != 3 || r.size() != 3 || s.size() != 3)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());

  // Set camera only if in main file
  if (_files.size() == 1)
  {
    _scene.camera() = Math::Matrix<4, 4>::translation(t[0], t[1], t[2]) * Math::Matrix<4, 4>::rotation(r[0], r[1], r[2]) * Math::Matrix<4, 4>::scale(s[0], s[1], s[2]);
    _scene.original() = _scene.camera();
  }
}

void	RT::Parser::settingResolution(unsigned int width, unsigned int height)
{
  // Set resolution only if in main file
  if (_files.size() == 1)
  {
    _scene.image().create(width, height, RT::Color(0.f).sfml());
    _scene.hud().create(width, height, RT::Color(0.f).sfml(0.f));
  }
}

void	RT::Parser::settingAntiAliasing(unsigned int live, unsigned int post)
{
  // Set antialiasing only if in main file
  if (_files.size() == 1)
  {
    _scene.antialiasing().live = live;
    _scene.antialiasing().post = post;
  }
}

void	RT::Parser::settingDephOfField(double aperture, double focal, unsigned int quality)
{
  // Set deph of field configuration only if in main file
  if (_files.size() == 1)
  {
    _scene.dof().aperture = aperture;
    _scene.dof().focal = focal;
    _scene.dof().quality = quality;
  }
}

void	RT::Parser::settingVirtualReality(double offset, double distortion, double center)
{
  // Set virtual reality configuration only if in main file
  if (_files.size() == 1)
  {
    _scene.vr().offset = offset;
    _scene.vr().distortion = distortion;
    _scene.vr().center = center;
  }
}

void	RT::Parser::settingThread(unsigned int thread)
{
  // Set number of rendering thread only in main file
  if (_files.size() == 1)
  {
    if (thread == 0)
      _scene.config().threadNumber = RT::Config::ThreadNumber - 1;
    else
      _scene.config().threadNumber = thread;
  }
}

std::string	RT::Parser::directory(std::string const & file) const
{
#ifdef _WIN32
  return file.substr(0, file.find_last_of('\\')) + std::string("\\");
#else
  if (file.find_last_of('/') == std::string::npos)
    return std::string("./");
  else
    return file.substr(0, file.find_last_of('/')) + std::string("/");
#endif
}
