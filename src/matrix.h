#pragma once
#include <stdio.h>
#include "vec3.h"
#include "vec4.h"
// Vectors in Ozymandias are column vectors:
//                        [x]
// that is, vec3(x,y,z) = [y]
//                        [z]
// and nxm matrices are
//     [a₀₀ a₀₁ ⋯ a₀ₘ]
//     [a₁₀ a₁₁ ⋯ a₁ₘ]
// A = [        ⋯    ]
//     [aₙ₀ aₙ₁ ⋯ aₙₘ]
// which means that, if x ∈ ℝ³ and A ∈ ℝ³³
//      [a₀₀*x a₀₁*x ⋯ a₀ₘ*x]
// Ax = [a₁₀*x a₁₁*x ⋯ a₁ₘ*x]
//      [a₂₀*x a₂₁*x ⋯ a₂ₘ*x].
// A 3x3 matrix is stored in memory as 
// A = {a₀₀, a₀₁, a₀₂, a₁₀, ⋯, a₂₂}

//TODO(Vidar): alignment?
//TODO(Vidar): use clang vector instead??
typedef struct 
{
    float m[9];
} Matrix3;

typedef struct 
{
    float m[16];
} Matrix4;

// sets element a_xy
#define set_matrix3(mat, r, c, a) mat.m[(c)+(r)*3] = (a)
#define set_matrix4(mat, r, c, a) mat.m[(c)+(r)*4] = (a)

// returns element a_xy
static inline PURE
//TODO(Vidar): Fix this
float get_matrix3(Matrix3 m, int r, int c)
{
    return m.m[c+r*3];
}
static inline PURE
float get_matrix4(Matrix4 m, int r, int c)
{
    return m.m[c+r*4];
}


static inline CONST
Matrix3 identity_matrix3()
{
    Matrix3 ret = {{1.f,0.f,0.f,
                    0.f,1.f,0.f,
                    0.f,0.f,1.f}};
    return ret;
}
static inline CONST
Matrix4 identity_matrix4()
{
    Matrix4 ret = {{1.f,0.f,0.f,0.f,
                    0.f,1.f,0.f,0.f,
                    0.f,0.f,1.f,0.f,
                    0.f,0.f,0.f,1.f}};
    return ret;
}

// Uses a, b and c as column vectors
//     [a.x b.x c.x]
// A = [a.y b.y c.y]
//     [a.z b.z c.z]
static inline PURE
Matrix3 matrix3(Vec3 a, Vec3 b, Vec3 c)
{
    Matrix3 ret={};
    for(int i=0;i<3;i++){
        set_matrix3(ret,i,0,a.v[i]);
        set_matrix3(ret,i,1,b.v[i]);
        set_matrix3(ret,i,2,c.v[i]);
    }
    return ret;
}
static inline PURE
Matrix4 matrix4(Vec4 a, Vec4 b, Vec4 c, Vec4 d)
{
    Matrix4 ret={};
    for(int i=0;i<4;i++){
        set_matrix4(ret,i,0,a.v[i]);
        set_matrix4(ret,i,1,b.v[i]);
        set_matrix4(ret,i,2,c.v[i]);
        set_matrix4(ret,i,3,d.v[i]);
    }
    return ret;
}

static inline CONST
Matrix3 add_matrix3(Matrix3 a, Matrix3 b)
{
    Matrix3 ret = {{
        a.m[0]+b.m[0],a.m[1]+b.m[1],a.m[2]+b.m[2],
        a.m[3]+b.m[3],a.m[4]+b.m[4],a.m[5]+b.m[5],
        a.m[6]+b.m[6],a.m[7]+b.m[7],a.m[8]+b.m[8],
    }};
    return ret;
}

static inline CONST
Matrix3 sub_matrix3(Matrix3 a, Matrix3 b)
{
    Matrix3 ret = {{
        a.m[0]-b.m[0],a.m[1]-b.m[1],a.m[2]-b.m[2],
        a.m[3]-b.m[3],a.m[4]-b.m[4],a.m[5]-b.m[5],
        a.m[6]-b.m[6],a.m[7]-b.m[7],a.m[8]-b.m[8],
    }};
    return ret;
}

//      [a_00*x + a_01*y + a_02*z]
// xA = [a_10*x + a_11*y + a_12*z]
//      [a_20*x + a_21*y + a_22*z]

static inline Vec3 PURE
mul_matrix3(Matrix3 m,Vec3 v)
{
    Vec3 ret = vec3(0.f, 0.f, 0.f);
    for(int i=0;i<3;i++){
        ret.v[i] = get_matrix3(m,0,i) * v.x + get_matrix3(m,1,i) * v.y +
                get_matrix3(m,2,i) * v.z;
    }
    return ret;
}
static inline Vec3 PURE
mul_matrix4(Matrix4 m,Vec4 v)
{
    Vec4 ret = vec4(0.f, 0.f, 0.f, 0.f);
    for(int i=0;i<4;i++){
        ret.v[i] = get_matrix4(m,i,0) * v.x + get_matrix4(m,i,1) * v.y +
                get_matrix4(m,i,2) * v.z + get_matrix4(m,i,3) * v.w;
    }
    return ret;
}

static inline Matrix4 CONST 
mul_mat4_mat4(Matrix4 a, Matrix4 b)
{
    Matrix4 ret = {};
    for(u8 i=0; i<4; i++){
        for(u8 j=0; j<4; j++){
            set_matrix4(ret,i,j,get_matrix4(a,i,0)*get_matrix4(b,0,j)
                + get_matrix4(a,i,1)*get_matrix4(b,1,j)
                + get_matrix4(a,i,2)*get_matrix4(b,2,j)
                + get_matrix4(a,i,3)*get_matrix4(b,3,j));
        }
    }
    return ret;
}

static inline CONST
float det(Matrix3 m){
    float ret = 0;
    ret += m.m[0]*(m.m[4]*m.m[8] - m.m[5]*m.m[7]);
    ret += m.m[1]*(m.m[5]*m.m[6] - m.m[3]*m.m[8]);
    ret += m.m[2]*(m.m[3]*m.m[7] - m.m[4]*m.m[6]);
    return ret;
}

static inline CONST
Matrix3 transpose_matrix3(Matrix3 m)
{ 
    Matrix3 tmp = {};
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            tmp.m[i+j*3] = m.m[j+i*3];
        }
    }
    return tmp;
}
static inline CONST
Matrix4 transpose_matrix4(Matrix4 m)
{ 
    Matrix4 tmp = {};
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            tmp.m[i+j*4] = m.m[j+i*4];
        }
    }
    return tmp;
}

static inline CONST
Matrix4 translation_matrix4(Vec3 t)
{
    Matrix4 ret = {{1.f, 0.f, 0.f, t.x,
                    0.f, 1.f, 0.f, t.y,
                    0.f, 0.f, 1.f, t.z,
                    0.f, 0.f, 0.f, 1.f}};
    return ret;
}
static inline CONST
Matrix4 scale_matrix4(Vec3 s)
{
    Matrix4 ret = {{s.x, 0.f, 0.f, 0.f,
                    0.f, s.y, 0.f, 0.f,
                    0.f, 0.f, s.z, 0.f,
                    0.f, 0.f, 0.f, 1.f}};
    return ret;
}
static inline CONST
Matrix4 euler_xyz_rotation_matrix4(Vec3 r)
{
    float cx = cosf(r.x), sx = sinf(r.x);
    float cy = cosf(r.y), sy = sinf(r.y);
    float cz = cosf(r.z), sz = sinf(r.z);
    Matrix4 ret = {{cy*cz, cx*sz+sx*sy*cz, sx*sz-cx*sy*cz, 0.f,
                   -cy*sz, cx*cz-sx*sy*sz, sx*cz+cx*sy*sz, 0.f,
                       sy,         -sx*cy,          cx*cy, 0.f,
                      0.f,            0.f,            0.f, 1.f}};
    return ret;
}

//TODO(Vidar): Test this properly...
static inline CONST
Vec4 gauss_matrix4_vec4(Matrix4 m, Vec4 b)
{
    u32 pivot[4] = {0,1,2,3};
    for(u8 k=0; k<3; k++){
        // Partila pivoting
        u8 max_index = k;
        float max_val = fabsf(get_matrix4(m,k,k));
        for(u8 i=k+1;i<4;i++){
            float val = fabsf(get_matrix4(m,i,k));
            if(val > max_val){
                max_index = i;
            }
        }
        if(max_index != k){
            SWAP(pivot[k],pivot[max_index]);
            for(u8 i=0;i<4;i++){
                SWAP(m.m[i+k*4],m.m[i+max_index*4]);
            }
        }
        // Row operations
        for(u8 i=k+1;i<4;i++){
            set_matrix4(m,i,k, -get_matrix4(m,i,k)/get_matrix4(m,k,k));
            for(u8 j=k+1;j<4;j++){
                set_matrix4(m,i,j, get_matrix4(m,i,j)
                        + get_matrix4(m,i,k)*get_matrix4(m,k,j));
            }
        }
    }
    // Now, m is in upper triangular form
    // Perform the same row operations on b
    Vec3 x;
    for(u8 k=0; k<4; k++){
        x.v[k] = b.v[pivot[k]];
    }
    for(u8 k=0; k<3; k++){
        for(u8 i=k+1; i<4; i++){
            x.v[i] = x.v[i] + get_matrix4(m,i,k)*x.v[k];
        }
    }
    //Back substitute
    x.v[3] = x.v[3]/get_matrix4(m,3,3);
    for(s8 i=2;i>=0;i--){
        float val = 0.f;
        for(s8 j=i+1;j<4;j++){
            val += get_matrix4(m,i,j)*x.v[j];
        }
        x.v[i] = (x.v[i] - val)/get_matrix4(m,i,i);
    }
    return x;
}

/*static inline OVERLOADABLE
void print(Matrix3 m){
    for(int i=0;i<3;i++){
        printf("[%f, %f, %f]\n", get_matrix3(m,i,0), get_matrix3(m,i,1),
                get_matrix3(m,i,2));
    }
}
static inline OVERLOADABLE
void print(Matrix4 m){
    for(int i=0;i<4;i++){
        printf("[%f, %f, %f, %f]\n", get_matrix4(m,i,0), get_matrix4(m,i,1),
                get_matrix4(m,i,2), get_matrix4(m,i,3));
    }
}
*/
