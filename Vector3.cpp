#include "Vector3.h"

Vector3::Vector3() : x(0), y(0), z(0) {}
Vector3::Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

long double& Vector3::operator[](int index) {
    if(index < 0 || index > 2) throw std::out_of_range("Index must be 0-2");
    return (index == 0 ? x : (index == 1 ? y : z));
}

const long double& Vector3::operator[](int index) const {
    if(index < 0 || index > 2) throw std::out_of_range("Index must be 0-2");
    return (index == 0 ? x : (index == 1 ? y : z));
}

Vector3 Vector3::operator+(const Vector3& other) const {
    return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3 Vector3::operator-(const Vector3& other) const {
    return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(long double scalar) const {
    return Vector3(x * scalar, y * scalar, z * scalar);
}

Vector3 Vector3::operator/(long double scalar) const {
    return Vector3(x / scalar, y / scalar, z / scalar);
}

Vector3& Vector3::operator+=(const Vector3& other) {
    x += other.x; y += other.y; z += other.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& other) {
    x -= other.x; y -= other.y; z -= other.z;
    return *this;
}

Vector3& Vector3::operator*=(double scalar) {
    x *= scalar; y *= scalar; z *= scalar;
    return *this;
}

long double Vector3::dot(const Vector3& other) const {
    return x * other.x + y * other.y + z * other.z;
}

Vector3 Vector3::cross(const Vector3& other) const {
    return Vector3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

Vector3 Vector3::elementwiseMultiply(const Vector3& other) const {
    return Vector3(x * other.x, y * other.y, z * other.z);
}

long double Vector3::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3 Vector3::normalize() const {
    long double mag = magnitude();
    if(mag == 0) throw std::runtime_error("Cannot normalize zero vector");
    return *this / mag;
}

void Vector3::print() const {
    std::cout << "[" << std::setprecision(3) << x << ", " 
              << y << ", " << z << "]" << std::endl;
}

Vector3 operator*(double scalar, const Vector3& vec) {
    return vec * scalar;
}
