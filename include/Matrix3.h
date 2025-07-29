#ifndef MATRIX3_H
#define MATRIX3_H

#include "Vector3.h"

class Matrix3 {
public:
    long double data[3][3];

    Matrix3();
    Matrix3(long double d00, long double d01, long double d02,
            long double d10, long double d11, long double d12,
            long double d20, long double d21, long double d22);

    long double* operator[](int row);
    const long double* operator[](int row) const;
    long double& operator()(int row, int col);
    const long double& operator()(int row, int col) const;

    Matrix3 operator+(const Matrix3& other) const;
    Matrix3 operator-(const Matrix3& other) const;
    Matrix3 operator*(double scalar) const;
    Matrix3 operator*(const Matrix3& other) const;
    Vector3 operator*(const Vector3& vec) const;

    Matrix3& operator+=(const Matrix3& other);
    Matrix3& operator-=(const Matrix3& other);
    Matrix3& operator*=(double scalar);

    Matrix3 transpose() const;
    double determinant() const;

    static Matrix3 identity();
    static Matrix3 rotationXCos(long double c);
    static Matrix3 rotationXSin(long double s);
    static Matrix3 rotationYCos(long double c);
    static Matrix3 rotationZCos(long double c, bool clockwise);

    void print() const;
};

Matrix3 operator*(double scalar, const Matrix3& mat);

#endif
