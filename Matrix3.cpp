#include "Matrix3.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <iomanip>

Matrix3::Matrix3() {
    for(int i=0;i<3;i++)
        for(int j=0;j<3;j++)
            data[i][j]=0;
}

Matrix3::Matrix3(double d00, double d01, double d02,
                 double d10, double d11, double d12,
                 double d20, double d21, double d22) {
    data[0][0]=d00; data[0][1]=d01; data[0][2]=d02;
    data[1][0]=d10; data[1][1]=d11; data[1][2]=d12;
    data[2][0]=d20; data[2][1]=d21; data[2][2]=d22;
}

long double* Matrix3::operator[](int row) {
    if(row<0||row>=3) throw std::out_of_range("Row index 0-2");
    return data[row];
}

const long double* Matrix3::operator[](int row) const {
    if(row<0||row>=3) throw std::out_of_range("Row index 0-2");
    return data[row];
}

long double& Matrix3::operator()(int row,int col) {
    if(row<0||row>=3||col<0||col>=3) throw std::out_of_range("Indices 0-2");
    return data[row][col];
}

const long double& Matrix3::operator()(int row,int col) const {
    if(row<0||row>=3||col<0||col>=3) throw std::out_of_range("Indices 0-2");
    return data[row][col];
}

Matrix3 Matrix3::operator+(const Matrix3& other) const {
    Matrix3 res;
    for(int i=0;i<3;i++) for(int j=0;j<3;j++)
        res.data[i][j]=data[i][j]+other.data[i][j];
    return res;
}

Matrix3 Matrix3::operator-(const Matrix3& other) const {
    Matrix3 res;
    for(int i=0;i<3;i++) for(int j=0;j<3;j++)
        res.data[i][j]=data[i][j]-other.data[i][j];
    return res;
}

Matrix3 Matrix3::operator*(double scalar) const {
    Matrix3 res;
    for(int i=0;i<3;i++) for(int j=0;j<3;j++)
        res.data[i][j]=data[i][j]*scalar;
    return res;
}

Matrix3 Matrix3::operator*(const Matrix3& other) const {
    Matrix3 res;
    for(int i=0;i<3;i++) for(int j=0;j<3;j++) {
        res.data[i][j]=0;
        for(int k=0;k<3;k++)
            res.data[i][j]+=data[i][k]*other.data[k][j];
    }
    return res;
}

Vector3 Matrix3::operator*(const Vector3& vec) const {
    return Vector3(
        data[0][0]*vec.x+data[0][1]*vec.y+data[0][2]*vec.z,
        data[1][0]*vec.x+data[1][1]*vec.y+data[1][2]*vec.z,
        data[2][0]*vec.x+data[2][1]*vec.y+data[2][2]*vec.z
    );
}

Matrix3& Matrix3::operator+=(const Matrix3& other) {
    for(int i=0;i<3;i++) for(int j=0;j<3;j++)
        data[i][j]+=other.data[i][j];
    return *this;
}

Matrix3& Matrix3::operator-=(const Matrix3& other) {
    for(int i=0;i<3;i++) for(int j=0;j<3;j++)
        data[i][j]-=other.data[i][j];
    return *this;
}

Matrix3& Matrix3::operator*=(double scalar) {
    for(int i=0;i<3;i++) for(int j=0;j<3;j++)
        data[i][j]*=scalar;
    return *this;
}

Matrix3 Matrix3::transpose() const {
    return Matrix3(
        data[0][0],data[1][0],data[2][0],
        data[0][1],data[1][1],data[2][1],
        data[0][2],data[1][2],data[2][2]
    );
}

double Matrix3::determinant() const {
    return data[0][0]*(data[1][1]*data[2][2]-data[1][2]*data[2][1])
         -data[0][1]*(data[1][0]*data[2][2]-data[1][2]*data[2][0])
         +data[0][2]*(data[1][0]*data[2][1]-data[1][1]*data[2][0]);
}

Matrix3 Matrix3::identity() {
    return Matrix3(1,0,0,0,1,0,0,0,1);
}

Matrix3 Matrix3::rotationX(long double c) {
    long double s=sqrt(1-pow(c,2));
    return Matrix3(1,0,0,0,c,-s,0,s,c);
}

Matrix3 Matrix3::rotationY(long double c) {
    long double s=sqrt(1-pow(c,2));
    return Matrix3(c,0,s,0,1,0,-s,0,c);
}

Matrix3 Matrix3::rotationZ(long double c,bool clockwise) {
    long double s=sqrt(1-pow(c,2));
    if(clockwise) s*=-1;
    return Matrix3(c,-s,0,s,c,0,0,0,1);
}

void Matrix3::print() const {
    for(int i=0;i<3;i++) {
        std::cout<<"[";
        for(int j=0;j<3;j++) {
            std::cout<<std::setw(8)<<std::setprecision(3)<<data[i][j];
            if(j<2) std::cout<<", ";
        }
        std::cout<<"]"<<std::endl;
    }
}

Matrix3 operator*(double scalar,const Matrix3& mat) {
    return mat*scalar;
}
