#ifndef MATRIX3_H
#define MATRIX3_H

#include "Vector3.h"

class Matrix3 {
public:
    double data[3][3];

    Matrix3();
    Matrix3(double d00, double d01, double d02,
            double d10, double d11, double d12,
            double d20, double d21, double d22);

    double* operator[](int row);
    const double* operator[](int row) const;
    double& operator()(int row, int col);
    const double& operator()(int row, int col) const;

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
    static Matrix3 rotationXCos(double c);
    static Matrix3 rotationXSin(double s);
    static Matrix3 rotationYCos(double c);
    static Matrix3 rotationZCos(double c, bool clockwise);

    void print() const;
};

Matrix3 operator*(double scalar, const Matrix3& mat);

#endif
