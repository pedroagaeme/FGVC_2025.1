#ifndef VECTOR3_H
#define VECTOR3_H

#include <iostream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

class Vector3 {
public:
    long double x, y, z;

    Vector3();
    Vector3(double x, double y, double z);

    long double& operator[](int index);
    const long double& operator[](int index) const;

    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(long double scalar) const;
    Vector3 operator/(long double scalar) const;

    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);
    Vector3& operator*=(double scalar);

    long double dot(const Vector3& other) const;
    Vector3 cross(const Vector3& other) const;
    Vector3 elementwiseMultiply(const Vector3& other) const;

    long double magnitude() const;
    Vector3 normalize() const;

    void print() const;
};

Vector3 operator*(double scalar, const Vector3& vec);

#endif