#include "Matrix4x4.hpp"

Math::Matrix4x4		Math::Matrix4x4::inverse() const
{
  Math::Matrix4x4	matrix;

  float	s0 = (*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0);
  float s1 = (*this)(0, 0) * (*this)(2, 1) - (*this)(0, 1) * (*this)(2, 0);
  float s2 = (*this)(0, 0) * (*this)(3, 1) - (*this)(0, 1) * (*this)(3, 0);
  float s3 = (*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0);
  float s4 = (*this)(1, 0) * (*this)(3, 1) - (*this)(1, 1) * (*this)(3, 0);
  float s5 = (*this)(2, 0) * (*this)(3, 1) - (*this)(2, 1) * (*this)(3, 0);

  float c0 = (*this)(0, 2) * (*this)(1, 3) - (*this)(0, 3) * (*this)(1, 2);
  float c1 = (*this)(0, 2) * (*this)(2, 3) - (*this)(0, 3) * (*this)(2, 2);
  float c2 = (*this)(0, 2) * (*this)(3, 3) - (*this)(0, 3) * (*this)(3, 2);
  float c3 = (*this)(1, 2) * (*this)(2, 3) - (*this)(1, 3) * (*this)(2, 2);
  float c4 = (*this)(1, 2) * (*this)(3, 3) - (*this)(1, 3) * (*this)(3, 2);
  float c5 = (*this)(2, 2) * (*this)(3, 3) - (*this)(2, 3) * (*this)(3, 2);

#ifdef _DEBUG
  // Not supposed to happen
  if (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0 == 0.f)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());
#endif

  float det = 1.f / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

  return Math::Matrix4x4(
    Math::Vector4((+(*this)(1, 1) * c5 - (*this)(2, 1) * c4 + (*this)(3, 1) * c3) * det, (-(*this)(0, 1) * c5 + (*this)(2, 1) * c2 - (*this)(3, 1) * c1) * det, (+(*this)(0, 1) * c4 - (*this)(1, 1) * c2 + (*this)(3, 1) * c0) * det, (-(*this)(0, 1) * c3 + (*this)(1, 1) * c1 - (*this)(2, 1) * c0) * det),
    Math::Vector4((-(*this)(1, 0) * c5 + (*this)(2, 0) * c4 - (*this)(3, 0) * c3) * det, (+(*this)(0, 0) * c5 - (*this)(2, 0) * c2 + (*this)(3, 0) * c1) * det, (-(*this)(0, 0) * c4 + (*this)(1, 0) * c2 - (*this)(3, 0) * c0) * det, (+(*this)(0, 0) * c3 - (*this)(1, 0) * c1 + (*this)(2, 0) * c0) * det),
    Math::Vector4((+(*this)(1, 3) * s5 - (*this)(2, 3) * s4 + (*this)(3, 3) * s3) * det, (-(*this)(0, 3) * s5 + (*this)(2, 3) * s2 - (*this)(3, 3) * s1) * det, (+(*this)(0, 3) * s4 - (*this)(1, 3) * s2 + (*this)(3, 3) * s0) * det, (-(*this)(0, 3) * s3 + (*this)(1, 3) * s1 - (*this)(2, 3) * s0) * det),
    Math::Vector4((-(*this)(1, 2) * s5 + (*this)(2, 2) * s4 - (*this)(3, 2) * s3) * det, (+(*this)(0, 2) * s5 - (*this)(2, 2) * s2 + (*this)(3, 2) * s1) * det, (-(*this)(0, 2) * s4 + (*this)(1, 2) * s2 - (*this)(3, 2) * s0) * det, (+(*this)(0, 2) * s3 - (*this)(1, 2) * s1 + (*this)(2, 2) * s0) * det)
  );
}

Math::Matrix4x4		Math::Matrix4x4::reflection(float x, float y, float z)
{
  return Math::Matrix4x4(
    Math::Vector4(1.f - 2.f * x * x, -2.f * y * x, -2.f * z * x, 0.f),
    Math::Vector4(-2.f * x * y, 1.f - 2.f * y * y, -2.f * z * y, 0.f),
    Math::Vector4(-2.f * x * z, -2.f * y * z, 1.f - 2.f * z * z, 0.f),
    Math::Vector4(0.f, 0.f, 0.f, 1.f)
  );
}

Math::Matrix4x4		Math::Matrix4x4::rotation(float x, float y, float z)
{
  // Convert parameters to radian
  x = (float)Math::Utils::DegToRad(x);
  y = (float)Math::Utils::DegToRad(y);
  z = (float)Math::Utils::DegToRad(z);

  return Math::Matrix4x4(Math::Vector4(+std::cos(z), -std::sin(z), 0.f, 0.f), Math::Vector4(+std::sin(z), +std::cos(z), 0.f, 0.f), Math::Vector4(0.f, 0.f, 1.f, 0.f), Math::Vector4(0.f, 0.f, 0.f, 1.f))
    * Math::Matrix4x4(Math::Vector4(+std::cos(y), 0.f, +std::sin(y), 0.f), Math::Vector4(0.f, 1.f, 0.f, 0.f), Math::Vector4(-std::sin(y), 0.f, +std::cos(y), 0.f), Math::Vector4(0.f, 0.f, 0.f, 1.f))
    * Math::Matrix4x4(Math::Vector4(1.f, 0.f, 0.f, 0.f), Math::Vector4(0.f, +std::cos(x), -std::sin(x), 0.f), Math::Vector4(0.f, +std::sin(x), +std::cos(x), 0.f), Math::Vector4(0.f, 0.f, 0.f, 1.f));
}

Math::Matrix4x4		Math::Matrix4x4::rotation(float a, float x, float y, float z)
{
  float			l = std::sqrt(x * x + y * y + z * z);

#ifdef _DEBUG
  // Should not happen
  if (l == 0)
    throw std::runtime_error((std::string(__FILE__) + ": l." + std::to_string(__LINE__)).c_str());
#endif

  // Normalize vector
  x /= l;
  y /= l;
  z /= l;

  // Convert input to radian
  a = (float)Math::Utils::DegToRad(a);

  return Math::Matrix4x4(
    Math::Vector4(x * x * (1.f - std::cos(a)) + std::cos(a), y * x * (1.f - std::cos(a)) - z * std::sin(a), z * x * (1.f - std::cos(a)) + y * std::sin(a), 0.f),
    Math::Vector4(x * y * (1.f - std::cos(a)) + z * std::sin(a), y * y * (1.f - std::cos(a)) + std::cos(a), z * y * (1.f - std::cos(a)) - x * std::sin(a), 0.f),
    Math::Vector4(x * z * (1.f - std::cos(a)) - y * std::sin(a), y * z * (1.f - std::cos(a)) + x * std::sin(a), z * z * (1.f - std::cos(a)) + std::cos(a), 0.f),
    Math::Vector4(0.f, 0.f, 0.f, 1.f)
  );
}

Math::Matrix4x4		Math::Matrix4x4::shear(float xy, float xz, float yx, float yz, float zx, float zy)
{
  return Math::Matrix4x4(Math::Vector4(1.f, xy, xz, 0.f), Math::Vector4(yx, 1.f, yz, 0.f), Math::Vector4(zx, zy, 1.f, 0.f), Math::Vector4(0.f, 0.f, 0.f, 1.f));
}
