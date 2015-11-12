#pragma once
#include <stdio.h>
#include "vec3.h"
// Vectors in Ozymandias are column vectors:
//                        [x]
// that is, vec3(x,y,z) = [y]
//                        [z]
// and NxN matrices are
//     [a_00 a_01 ... a_0N]
//     [a_10 a_11 ... a_1N]
// A = [          ...     ]
//     [a_N0 a_N1 ... a_NN]
// which means that, if x is vec3 and A Matrix3
//      [a_00*x + a_01*y + a_02*z]
// xA = [a_10*x + a_11*y + a_12*z]
//      [a_20*x + a_21*y + a_22*z]
// A 3x3 matrix is stored in memory as 
// m = {a_00, a_01, a_01, a_10, ..., a_22}

//TODO(Vidar): use clang vector instead??
typedef struct 
{
    float m[9];
} Matrix3;

// sets element a_xy
static inline 
void set_matrix(Matrix3 *m, int x, int y, float a)
{
    m->m[y+x*3] = a;
}

// returns element a_xy
static inline PURE
float get_matrix(Matrix3 m, int x, int y)
{
    return m.m[x+y*3];
}

// Uses a, b and c as column vectors
//     [a.x b.x c.x]
// A = [a.y b.y c.y]
//     [a.z b.z c.z]
static inline PURE
Matrix3 matrix3(vec3 a, vec3 b, vec3 c)
{
    Matrix3 ret={};
    for(int i=0;i<3;i++){
        set_matrix(&ret,i,0,a[i]);
        set_matrix(&ret,i,1,b[i]);
        set_matrix(&ret,i,2,c[i]);
    }
    return ret;
}

static inline CONST
Matrix3 add_matrix3(Matrix3 a, Matrix3 b)
{
    Matrix3 ret;
    for(int i=0;i<9;i++){
        ret.m[i] = a.m[i] + b.m[i];
    }
    return ret;
}

static inline CONST
Matrix3 sub_matrix3(Matrix3 a, Matrix3 b)
{
    Matrix3 ret;
    for(int i=0;i<9;i++){
        ret.m[i] = a.m[i] - b.m[i];
    }
    return ret;
}

//      [a_00*x + a_01*y + a_02*z]
// xA = [a_10*x + a_11*y + a_12*z]
//      [a_20*x + a_21*y + a_22*z]

// TODO(Vidar): maybe just call it mul?
static inline vec3 PURE
mul_matrix3(vec3 v,Matrix3 m)
{
    vec3 ret = (vec3){0.f, 0.f, 0.f};
    for(int i=0;i<3;i++){
        ret[i] = get_matrix(m,i,0) * v.x + get_matrix(m,i,1) * v.y +
                get_matrix(m,i,2) * v.z;
    }
    return ret;
}

//TODO(Vidar) make this use set_matrix() instead, to make it clearer what's
// happening...
static inline CONST
float det(Matrix3 m){
    float ret = 0;
    ret += m.m[0]*(m.m[4]*m.m[8] - m.m[5]*m.m[7]);
    ret += m.m[1]*(m.m[5]*m.m[6] - m.m[3]*m.m[8]);
    ret += m.m[2]*(m.m[3]*m.m[7] - m.m[4]*m.m[6]);
    return ret;
}

static inline CONST
Matrix3 transpose(Matrix3 m)
{ 
    Matrix3 tmp = {};
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            tmp.m[i+j*3] = m.m[j+i*3];
        }
    }
    return tmp;
}

static inline OVERLOADABLE
void print(Matrix3 m){
    for(int i=0;i<3;i++){
        printf("[%f, %f, %f]\n", get_matrix(m,0,i), get_matrix(m,1,i),
                get_matrix(m,2,i));
    }
}

