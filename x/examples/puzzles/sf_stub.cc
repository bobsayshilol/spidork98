// Simple trampolines from missing libgcc functions to SoftFP.
//
// Prototypes are in glibc/soft-fp as well as
// https://gcc.gnu.org/onlinedocs/gccint/Soft-float-library-routines.html
//

// This file needs to be C++ for some reason.
extern "C" {

#include "softfp/softfp.h"

#if 0
// Compiling this generates the same assembly as __mulsf3.
// gcc -O3 -c -D FE98_BUILD -msoft-float -mno-fp-ret-in-387 -x c++ sf_stub.cc
float test_func(float a, float b) {
  union U { float f; uint32_t u; };
  U a_, b_, c_;
  a_.f = a; b_.f = b;
  uint32_t flags = 0;
  c_.u = mul_sf32(a_.u, b_.u, RM_RNE, &flags);
  return c_.f;
}
#endif



//
// Binary ops
//

sfloat32 __addsf3(sfloat32 a, sfloat32 b) {
  uint32_t flags = 0;
  return add_sf32(a, b, RM_RNE, &flags);
}
sfloat32 __subsf3(sfloat32 a, sfloat32 b) {
  uint32_t flags = 0;
  return sub_sf32(a, b, RM_RNE, &flags);
}
sfloat32 __mulsf3(sfloat32 a, sfloat32 b) {
  uint32_t flags = 0;
  return mul_sf32(a, b, RM_RNE, &flags);
}
sfloat32 __divsf3(sfloat32 a, sfloat32 b) {
  uint32_t flags = 0;
  return div_sf32(a, b, RM_RNE, &flags);
}
sfloat32 __negsf2(sfloat32 a) {
  return a ^ FSIGN_MASK32;
}


sfloat64 __adddf3(sfloat64 a, sfloat64 b) {
  uint32_t flags = 0;
  return add_sf64(a, b, RM_RNE, &flags);
}
sfloat64 __subdf3(sfloat64 a, sfloat64 b) {
  uint32_t flags = 0;
  return sub_sf64(a, b, RM_RNE, &flags);
}
sfloat64 __muldf3(sfloat64 a, sfloat64 b) {
  uint32_t flags = 0;
  return mul_sf64(a, b, RM_RNE, &flags);
}
sfloat64 __divdf3(sfloat64 a, sfloat64 b) {
  uint32_t flags = 0;
  return div_sf64(a, b, RM_RNE, &flags);
}
sfloat64 __negdf2(sfloat64 a) {
  return a ^ FSIGN_MASK64;
}



//
// Conversion
//

int32_t __fixsfsi(sfloat32 a) {
  uint32_t flags = 0;
  return cvt_sf32_i32(a, RM_RNE, &flags);
}
sfloat32 __floatsisf(int32_t i) {
  uint32_t flags = 0;
  return cvt_i32_sf32(i, RM_RNE, &flags);
}

int32_t __fixdfsi(sfloat64 a) {
  uint32_t flags = 0;
  return cvt_sf64_i32(a, RM_RNE, &flags);
}
sfloat64 __floatsidf(int32_t i) {
  uint32_t flags = 0;
  return cvt_i32_sf64(i, RM_RNE, &flags);
}

sfloat32 __truncdfsf2(sfloat64 a) {
  uint32_t flags = 0;
  return cvt_sf64_sf32(a, RM_RNE, &flags);
}
sfloat64 __extendsfdf2(sfloat32 a) {
  uint32_t flags = 0;
  return cvt_sf32_sf64(a, &flags);
}


// libgcc.a implements these, but with an incorrect calling convention.
uint32_t __fixunssfsi(sfloat32 a) {
  uint32_t flags = 0;
  return cvt_sf32_u32(a, RM_RNE, &flags);
}
uint64_t __fixunssfdi(sfloat32 a) {
  uint32_t flags = 0;
  return cvt_sf32_u64(a, RM_RNE, &flags);
}
int64_t __fixsfdi(sfloat32 a) {
  uint32_t flags = 0;
  return cvt_sf32_i64(a, RM_RNE, &flags);
}
sfloat32 __floatdisf(int64_t i) {
  uint32_t flags = 0;
  return cvt_i64_sf32(i, RM_RNE, &flags);
}

uint32_t __fixunsdfsi(sfloat64 a) {
  uint32_t flags = 0;
  return cvt_sf64_u32(a, RM_RNE, &flags);
}
uint64_t __fixunsdfdi(sfloat64 a) {
  uint32_t flags = 0;
  return cvt_sf64_u64(a, RM_RNE, &flags);
}
int64_t __fixdfdi(sfloat64 a) {
  uint32_t flags = 0;
  return cvt_sf64_i64(a, RM_RNE, &flags);
}
sfloat64 __floatdidf(int64_t i) {
  uint32_t flags = 0;
  return cvt_i64_sf64(i, RM_RNE, &flags);
}



//
// Comparison
//

typedef int CMPtype;

CMPtype __eqsf2(sfloat32 a, sfloat32 b) { // a == b -> 0
  uint32_t flags = 0;
  return !eq_quiet_sf32(a, b, &flags);
}
CMPtype __eqdf2(sfloat64 a, sfloat64 b) { // a == b -> 0
  uint32_t flags = 0;
  return !eq_quiet_sf64(a, b, &flags);
}

CMPtype __nesf2(sfloat32 a, sfloat32 b) { // a != b -> !0
  return __eqsf2(a, b);
}
CMPtype __nesd2(sfloat64 a, sfloat64 b) { // a != b -> !0
  return __eqdf2(a, b);
}

CMPtype __gesf2(sfloat32 a, sfloat32 b) { // a >= b -> >=0
  uint32_t flags = 0;
  return (le_sf32(b, a, &flags) << 1) - 1;
}
CMPtype __gedf2(sfloat64 a, sfloat64 b) { // a >= b -> >=0
  uint32_t flags = 0;
  return (le_sf64(b, a, &flags) << 1) - 1;
}

CMPtype __gtsf2(sfloat32 a, sfloat32 b) { // a > b -> >0
  uint32_t flags = 0;
  return lt_sf32(b, a, &flags);
}
CMPtype __gtdf2(sfloat64 a, sfloat64 b) { // a > b -> >0
  uint32_t flags = 0;
  return lt_sf64(b, a, &flags);
}

CMPtype __ltsf2(sfloat32 a, sfloat32 b) { // a < b -> <0
  uint32_t flags = 0;
  return -lt_sf32(a, b, &flags);
}
CMPtype __ltdf2(sfloat64 a, sfloat64 b) { // a < b -> <0
  uint32_t flags = 0;
  return -lt_sf64(a, b, &flags);
}

CMPtype __lesf2(sfloat32 a, sfloat32 b) { // a <= b -> <=0
  uint32_t flags = 0;
  return -(le_sf32(a, b, &flags) << 1) + 1;
}
CMPtype __ledf2(sfloat64 a, sfloat64 b) { // a <= b -> <=0
  uint32_t flags = 0;
  return -(le_sf64(a, b, &flags) << 1) + 1;
}

} // extern "C"
