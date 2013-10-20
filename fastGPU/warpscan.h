#pragma once

static __device__ __forceinline__
void getMinMax(float3 &_Xmin, float3 &_Xmax, const fvec3 & pos) {
  fvec3 Xmin = pos;
  fvec3 Xmax = Xmin;
#pragma unroll
  for (int i=0; i<WARP_SIZE2; i++) {
    Xmin[0] = min(Xmin[0], __shfl_xor(Xmin[0], 1<<i));
    Xmin[1] = min(Xmin[1], __shfl_xor(Xmin[1], 1<<i));
    Xmin[2] = min(Xmin[2], __shfl_xor(Xmin[2], 1<<i));
    Xmax[0] = max(Xmax[0], __shfl_xor(Xmax[0], 1<<i));
    Xmax[1] = max(Xmax[1], __shfl_xor(Xmax[1], 1<<i));
    Xmax[2] = max(Xmax[2], __shfl_xor(Xmax[2], 1<<i));
  }
  _Xmin.x = min(_Xmin.x, Xmin[0]);
  _Xmin.y = min(_Xmin.y, Xmin[1]);
  _Xmin.z = min(_Xmin.z, Xmin[2]);
  _Xmax.x = max(_Xmax.x, Xmax[0]);
  _Xmax.y = max(_Xmax.y, Xmax[1]);
  _Xmax.z = max(_Xmax.z, Xmax[2]);
}

// Scan int

static __device__ __forceinline__
uint shflScan(uint partial, uint offset) {
  uint result;
  asm("{.reg .u32 r0;"
      ".reg .pred p;"
      "shfl.up.b32 r0|p, %1, %2, 0;"
      "@p add.u32 r0, r0, %3;"
      "mov.u32 %0, r0;}"
      : "=r"(result) : "r"(partial), "r"(offset), "r"(partial));
  return result;
}

static __device__ __forceinline__
uint inclusiveScanInt(const int value) {
  uint sum = value;
#pragma unroll
  for (int i=0; i<WARP_SIZE2; ++i)
    sum = shflScan(sum, 1 << i);
  return sum;
}

// Scan bool

static __device__ __forceinline__
int lanemask_lt() {
  int mask;
  asm("mov.u32 %0, %lanemask_lt;" : "=r" (mask));
  return mask;
}

static __device__ __forceinline__
int exclusiveScanBool(const bool p) {
  const uint b = __ballot(p);
  return __popc(b & lanemask_lt());
}

static __device__ __forceinline__
int reduceBool(const bool p) {
  const uint b = __ballot(p);
  return __popc(b);
}

// Segmented scan int

static __device__ __forceinline__
int lanemask_le() {
  int mask;
  asm("mov.u32 %0, %lanemask_le;" : "=r" (mask));
  return mask;
}

static __device__ __forceinline__
int shflSegScan(int partial, uint offset, uint distance) {
  asm("{.reg .u32 r0;"
      ".reg .pred p;"
      "shfl.up.b32 r0, %1, %2, 0;"
      "setp.le.u32 p, %2, %3;"
      "@p add.u32 %1, r0, %1;"
      "mov.u32 %0, %1;}"
      : "=r"(partial) : "r"(partial), "r"(offset), "r"(distance));
  return partial;
}

template<const int SIZE2>
static __device__ __forceinline__
int inclusiveSegscan(int value, const int distance) {
  for (int i=0; i<SIZE2; i++)
    value = shflSegScan(value, 1<<i, distance);
  return value;
}

static __device__ __forceinline__
int inclusiveSegscanInt(const int packedValue, const int carryValue) {
  const int flag = packedValue < 0;
  const int mask = -flag;
  const int value = (~mask & packedValue) + (mask & (-1-packedValue));
  const int flags = __ballot(flag);
  const int dist_block = __clz(__brev(flags));
  const int laneIdx = threadIdx.x & (WARP_SIZE - 1);
  const int distance = __clz(flags & lanemask_le()) + laneIdx - 31;
  const int val = inclusiveSegscan<WARP_SIZE2>(value, min(distance, laneIdx)) +
    (carryValue & (-(laneIdx < dist_block)));
  return val;
}
