#include <iostream>
#include <numbers>
#include "Quaternion.h"
#include "Vector3.h"

using namespace math;

/**
 * @file Quaternion_Class_Examples.cpp
 * @brief Contains usage examples for the Quaternion class.
 */

 /** @name Quaternion Constructors and Identity */
 //@{

 // [Quaternion_DefaultConstructor]
void Quaternion_DefaultConstructor()
{
    Quaternion q;
    std::cout << "Identity Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: (1, 0, 0, 0)
}
// [/Quaternion_DefaultConstructor]

// [Quaternion_ParameterizedConstructor]
void Quaternion_ParameterizedConstructor()
{
    Quaternion q(0.707f, 0.0f, 0.707f, 0.0f);
    std::cout << "Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: (0.707, 0.0, 0.707, 0.0)
}
// [/Quaternion_ParameterizedConstructor]

// [Quaternion_Identity]
void Quaternion_Identity()
{
    Quaternion q = Quaternion::Identity();
    std::cout << "Identity Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: (1, 0, 0, 0)
}
// [/Quaternion_Identity]

//@}


/** @name Quaternion Creation */
//@{

// [Quaternion_FromAxisAngle]
void Quaternion_FromAxisAngle()
{
    Vector3<float> axis(0.0f, 1.0f, 0.0f);
    float angle = std::numbers::pi_v<float> / 2.0f;
    Quaternion q = Quaternion::FromAxisAngle(axis, angle);
    std::cout << "FromAxisAngle Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: (~=0.707, 0.0, ~=0.707, 0.0)
}
// [/Quaternion_FromAxisAngle]

// [Quaternion_FromEuler]
void Quaternion_FromEuler()
{
    Vector3<float> euler(0.0f, std::numbers::pi_v<float> / 4.0f, 0.0f);
    Quaternion q = Quaternion::FromEuler(euler);
    std::cout << "FromEuler Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: (~=0.924, 0.0, ~=0.383, 0.0)
}
// [/Quaternion_FromEuler]

// [Quaternion_FromToRotation]
void Quaternion_FromToRotation()
{
    Vector3<float> from(1.0f, 0.0f, 0.0f);
    Vector3<float> to(0.0f, 0.0f, 1.0f);
    Quaternion q = Quaternion::FromToRotation(from, to);
    std::cout << "FromToRotation Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: Quaternion rotating +X to +Z
}
// [/Quaternion_FromToRotation]

// [Quaternion_LookRotation]
void Quaternion_LookRotation()
{
    Vector3<float> forward(0.0f, 0.0f, -1.0f);
    Vector3<float> up(0.0f, 1.0f, 0.0f);
    Quaternion q = Quaternion::LookRotation(forward, up);
    std::cout << "LookRotation Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: Quaternion facing forward (-Z)
}
// [/Quaternion_LookRotation]

//@}


/** @name Quaternion Operations */
//@{

// [Quaternion_Normalize]
void Quaternion_Normalize()
{
    Quaternion q(2.0f, 0.0f, 0.0f, 0.0f);
    q.Normalize();
    std::cout << "Normalized Quaternion: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: (1, 0, 0, 0)
}
// [/Quaternion_Normalize]

// [Quaternion_Conjugate]
void Quaternion_Conjugate()
{
    Quaternion q(0.707f, 0.0f, 0.707f, 0.0f);
    Quaternion c = q.Conjugate();
    std::cout << "Conjugate: (" << c.w << ", " << c.x << ", " << c.y << ", " << c.z << ")\n";
    // Output: (0.707, -0.0, -0.707, -0.0)
}
// [/Quaternion_Conjugate]

// [Quaternion_Inverse]
void Quaternion_Inverse()
{
    Quaternion q = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, std::numbers::pi_v<float> / 2.0f);
    Quaternion inv = q.Inverse();
    std::cout << "Inverse Quaternion: (" << inv.w << ", " << inv.x << ", " << inv.y << ", " << inv.z << ")\n";
    // Output: Quaternion representing the opposite rotation
}
// [/Quaternion_Inverse]

// [Quaternion_Dot]
void Quaternion_Dot()
{
    Quaternion a(1, 0, 0, 0);
    Quaternion b(0, 1, 0, 0);
    float dot = Quaternion::Dot(a, b);
    std::cout << "Dot Product: " << dot << "\n";
    // Output: 0
}
// [/Quaternion_Dot]

// [Quaternion_Angle]
void Quaternion_Angle()
{
    Quaternion a = Quaternion::FromEuler({ 0.0f, 0.0f, 0.0f });
    Quaternion b = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f });
    float angle = Quaternion::Angle(a, b);
    std::cout << "Angle between rotations: " << angle << " radians\n";
    // Output: ~=1.5708 radians
}
// [/Quaternion_Angle]

//@}


/** @name Quaternion Multiplication and Comparison */
//@{

// [Quaternion_Multiply]
void Quaternion_Multiply()
{
    Quaternion q1 = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 4.0f, 0.0f });
    Quaternion q2 = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 4.0f, 0.0f });
    Quaternion result = q1 * q2;
    std::cout << "Combined rotation (Y 90 degrees): (" << result.w << ", " << result.x << ", " << result.y << ", " << result.z << ")\n";
    // Output: ~=90 degrees rotation around Y
}
// [/Quaternion_Multiply]

// [Quaternion_Equality]
void Quaternion_Equality()
{
    Quaternion q1 = Quaternion::Identity();
    Quaternion q2 = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    std::cout << "Equal? " << (q1 == q2 ? "true" : "false") << "\n";
    // Output: true
}
// [/Quaternion_Equality]

// [Quaternion_Inequality]
void Quaternion_Inequality()
{
    Quaternion q1 = Quaternion::FromEuler({ 0.0f, 0.0f, 0.0f });
    Quaternion q2 = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 4.0f, 0.0f });
    std::cout << "Different? " << (q1 != q2 ? "true" : "false") << "\n";
    // Output: true
}
// [/Quaternion_Inequality]

//@}


/** @name Quaternion and Vectors */
//@{

// [Quaternion_RotateVector]
void Quaternion_RotateVector()
{
    Quaternion rotation = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 4.0f, 0.0f });
    Vector3<float> forward(0.0f, 0.0f, -1.0f);
    Vector3<float> rotated = rotation.RotateVector(forward);
    std::cout << "Rotated vector: (" << rotated.x << ", " << rotated.y << ", " << rotated.z << ")\n";
    // Output: Vector diagonally between -Z and -X
}
// [/Quaternion_RotateVector]

//@}


/** @name Quaternion Interpolation */
//@{

// [Quaternion_Slerp]
void Quaternion_Slerp()
{
    Quaternion a = Quaternion::FromEuler({ 0.0f, 0.0f, 0.0f });
    Quaternion b = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f });
    Quaternion q = Quaternion::Slerp(a, b, 0.5f);
    std::cout << "Slerp halfway rotation: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: ~=45° rotation around Y
}
// [/Quaternion_Slerp]

// [Quaternion_SlerpUnclamped]
void Quaternion_SlerpUnclamped()
{
    Quaternion a = Quaternion::FromEuler({ 0.0f, 0.0f, 0.0f });
    Quaternion b = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f });
    Quaternion q = Quaternion::SlerpUnclamped(a, b, 1.5f);
    std::cout << "SlerpUnclamped beyond target: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: ~=135° rotation around Y
}
// [/Quaternion_SlerpUnclamped]

// [Quaternion_Lerp]
void Quaternion_Lerp()
{
    Quaternion a = Quaternion::FromEuler({ 0.0f, 0.0f, 0.0f });
    Quaternion b = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f });
    Quaternion q = Quaternion::Lerp(a, b, 0.5f);
    std::cout << "Lerp halfway rotation: (" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
    // Output: ~=45° rotation around Y
}
// [/Quaternion_Lerp]

//@}


/** @name Quaternion Conversion */
//@{

// [Quaternion_ToAxisAngle]
void Quaternion_ToAxisAngle()
{
    Quaternion q = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, std::numbers::pi_v<float> / 3.0f);
    Vector3<float> axis;
    float angle;
    q.ToAxisAngle(axis, angle);
    std::cout << "Axis: (" << axis.x << ", " << axis.y << ", " << axis.z << "), Angle: " << angle << " radians\n";
    // Output: Axis (0, 1, 0), Angle ~=1.047 rad
}
// [/Quaternion_ToAxisAngle]

// [Quaternion_ToEuler]
void Quaternion_ToEuler()
{
    Quaternion q = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, std::numbers::pi_v<float> / 3.0f);
    Vector3<float> euler = q.ToEuler();
    std::cout << "Euler Angles: (" << euler.x << ", " << euler.y << ", " << euler.z << ")\n";
    // Output: (0, ~=1.047, 0)
}
// [/Quaternion_ToEuler]

//@}
