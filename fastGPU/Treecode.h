#pragma once

#include <assert.h>
#include "cudamem.h"
#include "plummer.h"
#include <string>
#include <sstream>
#include <sys/time.h>

#define WARP_SIZE2 5
#define WARP_SIZE 32

struct float6 {
  float xx;
  float yy;
  float zz;
  float xy;
  float xz;
  float yz;
};

struct double6 {
  double xx;
  double yy;
  double zz;
  double xy;
  double xz;
  double yz;
};

static inline double get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return double(tv.tv_sec + tv.tv_usec * 1e-6);
}

static void kernelSuccess(const char kernel[] = "kernel") {
  cudaDeviceSynchronize();
  const cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    fprintf(stderr,"%s launch failed: %s\n", kernel, cudaGetErrorString(err));
    assert(0);
  }
}

class CellData {
 private:
  static const int CHILD_SHIFT = 29;
  static const int CHILD_MASK  = ~(0x7U << CHILD_SHIFT);
  static const int LEVEL_SHIFT = 27;
  static const int LEVEL_MASK  = ~(0x1FU << LEVEL_SHIFT);
  uint4 data;
 public:
  __host__ __device__ CellData(const unsigned int level,
			       const unsigned int parent,
			       const unsigned int body,
			       const unsigned int nbody,
			       const unsigned int child = 0,
			       const unsigned int nchild = 0)
  {
    const unsigned int parentPack = parent | (level << LEVEL_SHIFT);
    const unsigned int childPack = child | (nchild << CHILD_SHIFT);
    data = make_uint4(parentPack, childPack, body, nbody);
  }

  __host__ __device__ CellData(const uint4 data) : data(data) {}

  __host__ __device__ int level()  const { return data.x >> LEVEL_SHIFT; }
  __host__ __device__ int parent() const { return data.x & LEVEL_MASK; }
  __host__ __device__ int child()  const { return data.y & CHILD_MASK; }
  __host__ __device__ int nchild() const { return (data.y >> CHILD_SHIFT)+1; }
  __host__ __device__ int body()   const { return data.z; }
  __host__ __device__ int nbody()  const { return data.w; }

  __host__ __device__ bool isLeaf() const { return data.y == 0; }
  __host__ __device__ bool isNode() const { return !isLeaf(); }

  __host__ __device__ void setParent(const unsigned int parent) {
    data.x = parent | (level() << LEVEL_SHIFT);
  }
  __host__ __device__ void setChild(const unsigned int child) {
    data.y = child | (nchild()-1 << CHILD_SHIFT);
  }
};

class Treecode {
 public:
  int2 buildTree(const int numBodies, float4 * d_bodyPos, float4 * d_bodyPos2, float4 * d_bosyAcc,
		 float4 * d_domain, int2 * d_levelRange, CellData * d_sourceCells, const int NLEAF = 16);
  void computeMultipoles(const int numBodies, const int numSources, const float theta,
			 float4 * d_bodyPos, CellData * d_sourceCells, float4 * d_sourceCenter,
			 float4 * d_Monopole, float4 * d_Quadrupole0, float2 * d_Quadrupole1);
  int groupTargets(const int numBodies, float4 * d_bodyPos, float4 * d_bodyPos2,
		   float4 * d_domain, int2 * d_targetCells, int levelSplit = 1, const int NCRIT = 64);
  float4 computeForces(const int numBodies, const int numTargets, const int numSources, const float eps,
		       float4 * d_bodyPos, float4 * d_bodyPos2, float4 * d_bodyAcc,
		       CellData * d_sourceCells, int2 * d_targetCells, float4 * d_sourceCenter, float4 * d_Monopole,
		       float4 * d_Quadrupole0, float2 * d_Quadrupole1, int2 * d_levelRange);
  void computeDirect(const int numBodies, const int numTarget, const int numBlock, const float eps,
		     float4 * d_bodyPos2, float4 * d_bodyAcc2);
};
