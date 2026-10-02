#include <iostream>
#include <numbers>
#include "Mat4.h"
#include "Vector3.h"
#include "Quaternion.h"

using namespace math;

/**
 * @file Mat4_Class_Examples.cpp
 * @brief Contains usage examples for the Mat4 class.
 */

 /** @name Mat4 Constructors and Basic Builders */
 //@{

 // [Mat4_DefaultConstructor]
void Mat4_DefaultConstructor()
{
    Mat4<float> m;
    std::cout << "Default (identity) element [0,0]: " << m(0, 0) << "\n";
    // Expect 1 at (0,0) for identity
}
// [/Mat4_DefaultConstructor]

// [Mat4_ParameterizedConstructor]
void Mat4_ParameterizedConstructor()
{
    Mat4<float> m(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    std::cout << "Element [3,2]: " << m(3, 2) << "\n";
    // Expect 15
}
// [/Mat4_ParameterizedConstructor]

// [Mat4_IdentityZero]
void Mat4_IdentityZero()
{
    Mat4<float> id = Mat4<float>::Identity();
    Mat4<float> z = Mat4<float>::Zero();
    std::cout << "Identity (0,0): " << id(0, 0) << ", Zero (0,0): " << z(0, 0) << "\n";
    // Output: 1 and 0
}
// [/Mat4_IdentityZero]

// [Mat4_TRS]
void Mat4_TRS()
{
    Vector3<float> pos(1.0f, 2.0f, 3.0f);
    Quaternion rot = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f });
    Vector3<float> scale(2.0f, 3.0f, 4.0f);

    Mat4<float> trs = Mat4<float>::TRS(pos, rot, scale);

    Vector3<float> extractedPos = trs.ExtractPosition();
    Vector3<float> extractedScale = trs.ExtractScale();

    std::cout << "TRS pos: (" << extractedPos.x << ", " << extractedPos.y << ", " << extractedPos.z << ")\n";
    std::cout << "TRS scale: (" << extractedScale.x << ", " << extractedScale.y << ", " << extractedScale.z << ")\n";
    // Expect pos (1,2,3) and scale (2,3,4)
}
// [/Mat4_TRS]

//@}


/** @name Mat4 Projection and View */
//@{

// [Mat4_Perspective]
void Mat4_Perspective()
{
    float fov = std::numbers::pi_v<float> / 4.0f; // 45 deg
    Mat4<float> p = Mat4<float>::Perspective(fov, 16.0f / 9.0f, 0.1f, 100.0f);
    std::cout << "Perspective[0,0]: " << p(0, 0) << ", [1,1]: " << p(1, 1) << "\n";
}
// [/Mat4_Perspective]

// [Mat4_Ortho]
void Mat4_Ortho()
{
    Mat4<float> o = Mat4<float>::Ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
    std::cout << "Ortho[0,0]: " << o(0, 0) << ", Ortho[3,2]: " << o(3, 2) << "\n";
}
// [/Mat4_Ortho]

// [Mat4_LookAt]
void Mat4_LookAt()
{
    Vector3<float> eye(0.0f, 1.0f, 5.0f);
    Vector3<float> target(0.0f, 0.0f, 0.0f);
    Vector3<float> up(0.0f, 1.0f, 0.0f);

    Mat4<float> view = Mat4<float>::LookAt(eye, target, up);
    Vector3<float> row0 = view.GetRow(0);
    std::cout << "View row0: (" << row0.x << ", " << row0.y << ", " << row0.z << ")\n";
}
// [/Mat4_LookAt]

//@}


/** @name Mat4 Arithmetic and Transforming Vectors */
//@{

// [Mat4_MultiplyMatrix]
void Mat4_MultiplyMatrix()
{
    Mat4<float> a = Mat4<float>::TRS({ 1,0,0 }, Quaternion::FromEuler({ 0,0,0 }), { 1,1,1 });
    Mat4<float> b = Mat4<float>::TRS({ 0,2,0 }, Quaternion::FromEuler({ 0,0,0 }), { 2,2,2 });
    Mat4<float> r = a * b;
    Vector3<float> pos = r.ExtractPosition();
    std::cout << "Result pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
    // Expect translation combined: roughly (1,2,0) depending on scale order
}
// [/Mat4_MultiplyMatrix]

// [Mat4_MultiplyPoint]
void Mat4_MultiplyPoint()
{
    Mat4<float> m = Mat4<float>::TRS({ 3,4,5 }, Quaternion::Identity(), { 1,1,1 });
    Vector3<float> p(1.0f, 1.0f, 1.0f);
    Vector3<float> tp = m.MultiplyPoint(p);
    std::cout << "Transformed point: (" << tp.x << ", " << tp.y << ", " << tp.z << ")\n";
    // Expect roughly (4,5,6)
}
// [/Mat4_MultiplyPoint]

// [Mat4_MultiplyVector]
void Mat4_MultiplyVector()
{
    Quaternion r = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f });
    Mat4<float> m = Mat4<float>::TRS({ 0,0,0 }, r, { 1,1,1 });
    Vector3<float> v(1.0f, 0.0f, 0.0f);
    Vector3<float> rv = m.MultiplyVector(v);
    std::cout << "Rotated vector: (" << rv.x << ", " << rv.y << ", " << rv.z << ")\n";
    // Expect approximately (0,0,-1)
}
// [/Mat4_MultiplyVector]

//@}


/** @name Mat4 Properties and Utilities */
//@{

// [Mat4_Transpose]
void Mat4_Transpose()
{
    Mat4<float> m = Mat4<float>::TRS({ 1,2,3 }, Quaternion::Identity(), { 1,1,1 });
    Mat4<float> t = m.Transpose();
    std::cout << "Transposed element (0,3): " << t(0, 3) << "\n";
}
// [/Mat4_Transpose]

// [Mat4_Inverse]
void Mat4_Inverse()
{
    Mat4<float> m = Mat4<float>::TRS({ 1,2,3 }, Quaternion::Identity(), { 1,1,1 });
    Mat4<float> inv = m.Inverse();
    Vector3<float> p = inv.MultiplyPoint({ 1,2,3 });
    std::cout << "Inverse * original position gives: (" << p.x << ", " << p.y << ", " << p.z << ")\n";
    // Expect approx (0,0,0)
}
// [/Mat4_Inverse]

// [Mat4_Determinant]
void Mat4_Determinant()
{
    Mat4<float> s = Mat4<float>::TRS({ 0,0,0 }, Quaternion::Identity(), { 2,3,4 });
    float d = s.Determinant();
    std::cout << "Determinant (scale only): " << d << "\n";
    // Expect 24
}
// [/Mat4_Determinant]

// [Mat4_ExtractRotation]
void Mat4_ExtractRotation()
{
    Quaternion q = Quaternion::FromEuler({ 0.0f, std::numbers::pi_v<float> / 3.0f, 0.0f });
    Mat4<float> m = Mat4<float>::TRS({ 0,0,0 }, q, { 1,1,1 });
    Quaternion out = m.ExtractRotation();
    Vector3<float> e = out.ToEuler();
    std::cout << "Extracted Euler (Y): " << e.y << "\n";
    // Expect approx pi/3
}
// [/Mat4_ExtractRotation]

// [Mat4_ValidTRS]
void Mat4_ValidTRS()
{
    Mat4<float> good = Mat4<float>::TRS({ 1,2,3 }, Quaternion::Identity(), { 1,1,1 });
    Mat4<float> bad = Mat4<float>::TRS({ 1,2,3 }, Quaternion::Identity(), { 0,0,0 }); // zero scale
    std::cout << "Good TRS valid? " << (good.ValidTRS() ? "true" : "false") << "\n";
    std::cout << "Bad TRS valid? " << (bad.ValidTRS() ? "true" : "false") << "\n";
}
// [/Mat4_ValidTRS]

// [Mat4_ExtractPositionScale]
void Mat4_ExtractPositionScale()
{
    Mat4<float> m = Mat4<float>::TRS({ 7,8,9 }, Quaternion::Identity(), { 2,2,2 });
    Vector3<float> pos = m.ExtractPosition();
    Vector3<float> scale = m.ExtractScale();
    std::cout << "Pos: (" << pos.x << "," << pos.y << "," << pos.z << ") Scale: (" << scale.x << "," << scale.y << "," << scale.z << ")\n";
}
// [/Mat4_ExtractPositionScale]

// [Mat4_GetRowColumn]
void Mat4_GetRowColumn()
{
    Mat4<float> m = Mat4<float>::TRS({ 1,2,3 }, Quaternion::Identity(), { 1,1,1 });
    Vector3<float> row1 = m.GetRow(1);
    Vector3<float> col2 = m.GetColumn(2);
    std::cout << "Row1: (" << row1.x << "," << row1.y << "," << row1.z << ") Col2: (" << col2.x << "," << col2.y << "," << col2.z << ")\n";
}
// [/Mat4_GetRowColumn]

// [Mat4_OperatorsCompare]
void Mat4_OperatorsCompare()
{
    Mat4<float> a = Mat4<float>::Identity();
    Mat4<float> b = Mat4<float>::Identity();
    Mat4<float> c = Mat4<float>::TRS({ 1,0,0 }, Quaternion::Identity(), { 1,1,1 });
    std::cout << "a==b? " << (a == b ? "true" : "false") << " a!=c? " << (a != c ? "true" : "false") << "\n";
}
// [/Mat4_OperatorsCompare]

//@}
