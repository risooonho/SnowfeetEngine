﻿/*
Matrix4.cpp
Copyright (C) 2010-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#include <cstring> // For memcpy
#include <cmath>
#include "Matrix4.h"

namespace sn
{

// Coordinates systems / left-handed / right-handed
// http://viz.aset.psu.edu/gho/sem_notes/3d_fundamentals/html/3d_coordinates.html

//------------------------------------------------------------------------------
const f32 g_identity[16] =
{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

//------------------------------------------------------------------------------
Matrix4::Matrix4()
{
    // Note: the IEEE 754 representation of 0 is 0
    //memset(m_v, 0, 16 * sizeof(f32));

    loadIdentity();
}

//------------------------------------------------------------------------------
Matrix4::Matrix4(const Matrix4 & other)
{
    set(other);
}

//------------------------------------------------------------------------------
Matrix4::Matrix4(const f32 values[16])
{
    set(values);
}

//------------------------------------------------------------------------------
void Matrix4::operator=(const Matrix4 & other)
{
    set(other);
}

//------------------------------------------------------------------------------
void Matrix4::set(const Matrix4 & other)
{
    memcpy(m_v, other.m_v, sizeof(m_v));
}

//------------------------------------------------------------------------------
void Matrix4::set(const f32 values[16])
{
    memcpy(m_v, values, sizeof(m_v));
}

//------------------------------------------------------------------------------
void Matrix4::loadIdentity()
{
    set(g_identity);
}

//------------------------------------------------------------------------------
void Matrix4::loadPerspectiveProjection(
        const f32 fovY, const f32 ratio,
        const f32 near, const f32 far)
{
    //  0   1   2   3
    //  4   5   6   7
    //  8   9  10  11
    // 12  13  14  15
    memset(m_v, 0, 16 * sizeof(f32)); // zeros
#if 0
    // Right-handed
    m_v[5] = 1.f / tan(fovY * 0.5f);
    m_v[0] = m_v[5] / ratio;
    m_v[10] = (near + far) / (near - far);
    m_v[11] = -1;
    m_v[14] = (2.f*near*far) / (near - far);
#else
    // Left-handed
    m_v[5] = 1.f / tan(fovY * 0.5f);
    m_v[0] = m_v[5] / ratio;
    m_v[10] = (near + far) / (far - near);
    m_v[11] = 1.f;
    m_v[14] = -2.f * near * far / (far - near);
#endif
}

//------------------------------------------------------------------------------
void Matrix4::loadPerspectiveProjection(Fov fov, f32 near, f32 far)
{
    m_v[0] = 2.f / (fov.tanRight + fov.tanLeft);
    m_v[5] = 2.f / (fov.tanUp + fov.tanDown);
    m_v[8] = (fov.tanLeft - fov.tanRight) / (fov.tanRight + fov.tanLeft);
    m_v[9] = (fov.tanUp - fov.tanDown) / (fov.tanUp + fov.tanDown);
    m_v[10] = (near + far) / (far - near);
    m_v[11] = 1.f;
    m_v[14] = -2.f * near * far / (far - near);

    //m_v[0] = 2 * near / (right - left);
    //m_v[5] = 2 * near / (top - bottom);
    //m_v[8] = (right + left) / (right - left);
    //m_v[9] = (top + bottom) / (top - bottom);
    //m_v[10] = -(far + near) / (far - near);
    //m_v[11] = -1;
    //m_v[14] = -2.f * near * far / (far - near);
}

//------------------------------------------------------------------------------
void Matrix4::loadOrtho2DProjection(
        const f32 left, const f32 top,
        const f32 right, const f32 bottom,
        const f32 near, const f32 far)
{
    memset(m_v, 0, 16 * sizeof(f32)); // zeros

    //  0   1   2   3
    //  4   5   6   7
    //  8   9  10  11
    // 12  13  14  15
    // TODO FIXME this doesn't works with assymetric view volumes
    m_v[0] = 2.f / (right - left);
	m_v[5] = 2.f / (top - bottom);
	m_v[10] = 2.f / (far - near);
	m_v[14] = near / (near - far);
	m_v[15] = 1;
}

//------------------------------------------------------------------------------
void Matrix4::loadLookAt(
        const Vector3f & eye,
        const Vector3f & target,
        const Vector3f & up)
{
    // Left-handed

    Vector3f zaxis = target - eye;
    zaxis.normalize();

    Vector3f xaxis = crossProduct(up, zaxis);
    xaxis.normalize();

    Vector3f yaxis = crossProduct(zaxis, xaxis);

    m_v[0] = xaxis.x();
    m_v[1] = yaxis.x();
    m_v[2] = zaxis.x();
    m_v[3] = 0;

    m_v[4] = xaxis.y();
    m_v[5] = yaxis.y();
    m_v[6] = zaxis.y();
    m_v[7] = 0;

    m_v[8] = xaxis.z();
    m_v[9] = yaxis.z();
    m_v[10] = zaxis.z();
    m_v[11] = 0;

    m_v[12] = -xaxis.dotProduct(eye);
    m_v[13] = -yaxis.dotProduct(eye);
    m_v[14] = -zaxis.dotProduct(eye);
    m_v[15] = 1;
}

//------------------------------------------------------------------------------
void Matrix4::loadTranslation(const f32 vx, const f32 vy, const f32 vz)
{
    loadIdentity();
    m_v[12] = vx;
    m_v[13] = vy;
    m_v[14] = vz;
}

//------------------------------------------------------------------------------
void Matrix4::loadScale(const f32 sx, const f32 sy, const f32 sz)
{
    memset(m_v, 0, 16 * sizeof(f32)); // zeros
    m_v[0] = sx;
    m_v[5] = sy;
    m_v[10] = sz;
    m_v[15] = 1;
}

//------------------------------------------------------------------------------
void Matrix4::loadRotation(const f32 t, const f32 x, const f32 y, const f32 z)
{
    const f32 cost = cos(t);
    const f32 sint = sin(t);

    m_v[0] = x*x*(1.f-cost)+cost;
    m_v[4] = x*y*(1.f-cost)+z*sint;
    m_v[8] = x*z*(1.f-cost)+y*sint;
    m_v[12] = 0;

    m_v[1] = x*y*(1.f-cost)-z*sint;
    m_v[5] = y*y*(1.f-cost)+cost;
    m_v[9] = y*z*(1.f-cost)+x*sint;
    m_v[13] = 0;

    m_v[2] = x*z*(1.f-cost)+y*sint;
    m_v[6] = y*z*(1.f-cost)+x*sint;
    m_v[10] = z*z*(1.f-cost)+cost;
    m_v[14] = 0;

    m_v[3] = 0;
    m_v[7] = 0;
    m_v[11] = 0;
    m_v[15] = 1;
}

//------------------------------------------------------------------------------
void Matrix4::setTranslation(const Vector3f & v)
{
    m_v[12] = v.x();
    m_v[13] = v.y();
    m_v[14] = v.z();
}

//------------------------------------------------------------------------------
void Matrix4::setRotation(const Quaternion & q)
{
#if 0
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm

    //1 - 2 * qy2 - 2 * qz2 	2 * qx*qy - 2 * qz*qw 	2 * qx*qz + 2 * qy*qw
    //2 * qx*qy + 2 * qz*qw 	1 - 2 * qx2 - 2 * qz2 	2 * qy*qz - 2 * qx*qw
    //2 * qx*qz - 2 * qy*qw 	2 * qy*qz + 2 * qx*qw 	1 - 2 * qx2 - 2 * qy2

    m_v[0 ] = 1.f - 2.f * q.getY()*q.getY() - 2.f * q.getZ()*q.getZ();
    m_v[1 ] =       2.f * q.getX()*q.getY() - 2.f * q.getZ()*q.getW();
    m_v[2 ] =       2.f * q.getX()*q.getZ() + 2.f * q.getY()*q.getW();

    m_v[4 ] =       2.f * q.getX()*q.getY() + 2.f * q.getZ()*q.getW();
    m_v[5 ] = 1.f - 2.f * q.getX()*q.getX() - 2.f * q.getZ()*q.getZ();
    m_v[6 ] =       2.f * q.getY()*q.getZ() - 2.f * q.getX()*q.getW();

    m_v[8 ] =       2.f * q.getX()*q.getZ() - 2.f * q.getY()*q.getW();
    m_v[9 ] =       2.f * q.getY()*q.getZ() + 2.f * q.getX()*q.getW();
    m_v[10] = 1.f - 2.f * q.getX()*q.getX() - 2.f * q.getY()*q.getY();

#else

    m_v[0 ] = 1.f - 2.f * q.getY()*q.getY() - 2.f * q.getZ()*q.getZ();
    m_v[1 ] =       2.f * q.getX()*q.getY() + 2.f * q.getZ()*q.getW();
    m_v[2 ] =       2.f * q.getX()*q.getZ() - 2.f * q.getY()*q.getW();

    m_v[4 ] =       2.f * q.getX()*q.getY() - 2.f * q.getZ()*q.getW();
    m_v[5 ] = 1.f - 2.f * q.getX()*q.getX() - 2.f * q.getZ()*q.getZ();
    m_v[6 ] =       2.f * q.getY()*q.getZ() + 2.f * q.getX()*q.getW();

    m_v[8 ] =       2.f * q.getX()*q.getZ() + 2.f * q.getY()*q.getW();
    m_v[9 ] =       2.f * q.getY()*q.getZ() - 2.f * q.getX()*q.getW();
    m_v[10] = 1.f - 2.f * q.getX()*q.getX() - 2.f * q.getY()*q.getY();

#endif
}

//------------------------------------------------------------------------------
void Matrix4::scaleTransform(const Vector3f & s)
{
    m_v[0] *= s[0];
    m_v[5] *= s[1];
    m_v[10] *= s[2];
}

//------------------------------------------------------------------------------
void Matrix4::setByProduct(const Matrix4 & in_a, const Matrix4 & in_b)
{
    //  0   1   2   3
    //  4   5   6   7
    //  8   9  10  11
    // 12  13  14  15

    const Matrix4 & a = in_a;
    const Matrix4 & b = in_b;

    m_v[0] = a[0]*b[0] + a[4]*b[1] + a[8]*b[2] + a[12]*b[3];
    m_v[1] = a[1]*b[0] + a[5]*b[1] + a[9]*b[2] + a[13]*b[3];
    m_v[2] = a[2]*b[0] + a[6]*b[1] + a[10]*b[2] + a[14]*b[3];
    m_v[3] = a[3]*b[0] + a[7]*b[1] + a[11]*b[2] + a[15]*b[3];

    m_v[4] = a[0]*b[4] + a[4]*b[5] + a[8]*b[6] + a[12]*b[7];
    m_v[5] = a[1]*b[4] + a[5]*b[5] + a[9]*b[6] + a[13]*b[7];
    m_v[6] = a[2]*b[4] + a[6]*b[5] + a[10]*b[6] + a[14]*b[7];
    m_v[7] = a[3]*b[4] + a[7]*b[5] + a[11]*b[6] + a[15]*b[7];

    m_v[8] = a[0]*b[8] + a[4]*b[9] + a[8]*b[10] + a[12]*b[11];
    m_v[9] = a[1]*b[8] + a[5]*b[9] + a[9]*b[10] + a[13]*b[11];
    m_v[10] = a[2]*b[8] + a[6]*b[9] + a[10]*b[10] + a[14]*b[11];
    m_v[11] = a[3]*b[8] + a[7]*b[9] + a[11]*b[10] + a[15]*b[11];

    m_v[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15];
    m_v[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15];
    m_v[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
    m_v[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];
}

//------------------------------------------------------------------------------
void Matrix4::setByProduct3x3(const Matrix4 & a, const Matrix4 & b)
{
    //  0   1   2
    //  4   5   6
    //  8   9  10

    m_v[0] = a[0]*b[0] + a[4]*b[1] + a[8]*b[2];
    m_v[1] = a[1]*b[0] + a[5]*b[1] + a[9]*b[2];
    m_v[2] = a[2]*b[0] + a[6]*b[1] + a[10]*b[2];
    m_v[3] = 0;

    m_v[4] = a[0]*b[4] + a[4]*b[5] + a[8]*b[6];
    m_v[5] = a[1]*b[4] + a[5]*b[5] + a[9]*b[6];
    m_v[6] = a[2]*b[4] + a[6]*b[5] + a[10]*b[6];
    m_v[7] = 0;

    m_v[8] = a[0]*b[8] + a[4]*b[9] + a[8]*b[10];
    m_v[9] = a[1]*b[8] + a[5]*b[9] + a[9]*b[10];
    m_v[10] = a[2]*b[8] + a[6]*b[9] + a[10]*b[10];
    m_v[11] = 0;

    m_v[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14];
    m_v[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14];
    m_v[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14];
    m_v[15] = 1;
}

//------------------------------------------------------------------------------
void Matrix4::getTransposed(Matrix4 & out_result) const
{
    out_result.m_v[0] = m_v[0];
    out_result.m_v[1] = m_v[4];
    out_result.m_v[2] = m_v[8];
    out_result.m_v[3] = m_v[12];

    out_result.m_v[4] = m_v[1];
    out_result.m_v[5] = m_v[5];
    out_result.m_v[6] = m_v[9];
    out_result.m_v[7] = m_v[13];

    out_result.m_v[8] = m_v[2];
    out_result.m_v[9] = m_v[6];
    out_result.m_v[10] = m_v[10];
    out_result.m_v[11] = m_v[14];

    out_result.m_v[12] = m_v[3];
    out_result.m_v[13] = m_v[7];
    out_result.m_v[14] = m_v[11];
    out_result.m_v[15] = m_v[15];
}

//------------------------------------------------------------------------------
void Matrix4::transpose()
{
    std::swap(m_v[1], m_v[4]);
    std::swap(m_v[2], m_v[8]);
    std::swap(m_v[3], m_v[12]);

    std::swap(m_v[4], m_v[1]);
    std::swap(m_v[6], m_v[9]);
    std::swap(m_v[7], m_v[13]);

    std::swap(m_v[8], m_v[2]);
    std::swap(m_v[9], m_v[6]);
    std::swap(m_v[11], m_v[14]);

    std::swap(m_v[12], m_v[3]);
    std::swap(m_v[13], m_v[7]);
    std::swap(m_v[14], m_v[11]);
}

//------------------------------------------------------------------------------
void Matrix4::transpose3x3()
{
    std::swap(m_v[1], m_v[4]);
    std::swap(m_v[2], m_v[8]);

    std::swap(m_v[4], m_v[1]);
    std::swap(m_v[6], m_v[9]);

    std::swap(m_v[8], m_v[2]);
    std::swap(m_v[9], m_v[6]);
}

//------------------------------------------------------------------------------
f32 Matrix4::getD() const
{
    const Matrix4 & m = *this;
    return 
        (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * (m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2)) -
        (m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) * (m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1)) +
        (m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0)) * (m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1)) +
        (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * (m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0)) -
        (m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1)) * (m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0)) +
        (m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2)) * (m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0));
}

//------------------------------------------------------------------------------
bool Matrix4::getInverse(Matrix4 & out_result) const
{
    f32 d = getD();
    if (math::isZero(d))
    {
        // The matrix cannot be inverted
        return false;
    }

    d = 1.f / d;

    const Matrix4 & m = *this;

    out_result(0, 0) = d*(
        m(1, 1) * (m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2)) +
        m(1, 2) * (m(2, 3) * m(3, 1) - m(2, 1) * m(3, 3)) +
        m(1, 3) * (m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1))
    );
    out_result(0, 1) = d*(
        m(2, 1) * (m(0, 2) * m(3, 3) - m(0, 3) * m(3, 2)) +
        m(2, 2) * (m(0, 3) * m(3, 1) - m(0, 1) * m(3, 3)) +
        m(2, 3) * (m(0, 1) * m(3, 2) - m(0, 2) * m(3, 1))
    );
    out_result(0, 2) = d*(
        m(3, 1) * (m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2)) +
        m(3, 2) * (m(0, 3) * m(1, 1) - m(0, 1) * m(1, 3)) +
        m(3, 3) * (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1))
    );
    out_result(0, 3) = d*(
        m(0, 1) * (m(1, 3) * m(2, 2) - m(1, 2) * m(2, 3)) +
        m(0, 2) * (m(1, 1) * m(2, 3) - m(1, 3) * m(2, 1)) +
        m(0, 3) * (m(1, 2) * m(2, 1) - m(1, 1) * m(2, 2))
    );
    out_result(1, 0) = d*(
        m(1, 2) * (m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0)) +
        m(1, 3) * (m(2, 2) * m(3, 0) - m(2, 0) * m(3, 2)) +
        m(1, 0) * (m(2, 3) * m(3, 2) - m(2, 2) * m(3, 3))
    );
    out_result(1, 1) = d*(
        m(2, 2) * (m(0, 0) * m(3, 3) - m(0, 3) * m(3, 0)) +
        m(2, 3) * (m(0, 2) * m(3, 0) - m(0, 0) * m(3, 2)) +
        m(2, 0) * (m(0, 3) * m(3, 2) - m(0, 2) * m(3, 3))
    );
    out_result(1, 2) = d*(
        m(3, 2) * (m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0)) +
        m(3, 3) * (m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2)) +
        m(3, 0) * (m(0, 3) * m(1, 2) - m(0, 2) * m(1, 3))
    );
    out_result(1, 3) = d*(
        m(0, 2) * (m(1, 3) * m(2, 0) - m(1, 0) * m(2, 3)) +
        m(0, 3) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
        m(0, 0) * (m(1, 2) * m(2, 3) - m(1, 3) * m(2, 2))
    );
    out_result(2, 0) = d*(
        m(1, 3) * (m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0)) +
        m(1, 0) * (m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1)) +
        m(1, 1) * (m(2, 3) * m(3, 0) - m(2, 0) * m(3, 3))
    );
    out_result(2, 1) = d*(
        m(2, 3) * (m(0, 0) * m(3, 1) - m(0, 1) * m(3, 0)) +
        m(2, 0) * (m(0, 1) * m(3, 3) - m(0, 3) * m(3, 1)) +
        m(2, 1) * (m(0, 3) * m(3, 0) - m(0, 0) * m(3, 3))
    );
    out_result(2, 2) = d*(
        m(3, 3) * (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) +
        m(3, 0) * (m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1)) +
        m(3, 1) * (m(0, 3) * m(1, 0) - m(0, 0) * m(1, 3))
    );
    out_result(2, 3) = d*(
        m(0, 3) * (m(1, 1) * m(2, 0) - m(1, 0) * m(2, 1)) +
        m(0, 0) * (m(1, 3) * m(2, 1) - m(1, 1) * m(2, 3)) +
        m(0, 1) * (m(1, 0) * m(2, 3) - m(1, 3) * m(2, 0))
    );
    out_result(3, 0) = d*(
        m(1, 0) * (m(2, 2) * m(3, 1) - m(2, 1) * m(3, 2)) +
        m(1, 1) * (m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0)) +
        m(1, 2) * (m(2, 1) * m(3, 0) - m(2, 0) * m(3, 1))
    );
    out_result(3, 1) = d*(
        m(2, 0) * (m(0, 2) * m(3, 1) - m(0, 1) * m(3, 2)) +
        m(2, 1) * (m(0, 0) * m(3, 2) - m(0, 2) * m(3, 0)) +
        m(2, 2) * (m(0, 1) * m(3, 0) - m(0, 0) * m(3, 1))
    );
    out_result(3, 2) = d*(
        m(3, 0) * (m(0, 2) * m(1, 1) - m(0, 1) * m(1, 2)) +
        m(3, 1) * (m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) +
        m(3, 2) * (m(0, 1) * m(1, 0) - m(0, 0) * m(1, 1))
    );
    out_result(3, 3) = d*(
        m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) +
        m(0, 1) * (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) +
        m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0))
    );

    return true;
}

Vector3f Matrix4::transformPoint(const Vector3f & p) const
{
	return Vector3f(
		m_v[0] * p[0] + m_v[4] * p[1] + m_v[8] * p[2] + m_v[12],
        m_v[1] * p[0] + m_v[5] * p[1] + m_v[9] * p[2] + m_v[13],
        m_v[2] * p[0] + m_v[6] * p[1] + m_v[10] * p[2] + m_v[14]
	);
}

} // namespace sn

