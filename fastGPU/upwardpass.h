#pragma once

namespace {
  static __device__ __forceinline__
    void addMultipole(double4 * __restrict__ _M, const float4 body) {
    const float x = body.x;
    const float y = body.y;
    const float z = body.z;
    const float m = body.w;
    float4 M[3];
    M[0].x = m*x;
    M[0].y = m*y;
    M[0].z = m*z;
    M[0].w = m;
    M[1].x = m*x*x;
    M[1].y = m*y*y;
    M[1].z = m*z*z;
    M[1].w = m*x*y;
    M[2].x = m*x*z;
    M[2].y = m*y*z;
    M[2].z = 0.0f;
    M[2].w = 0.0f;
#pragma unroll
    for (int j=0; j<3; j++) {
#pragma unroll
      for (int i=WARP_SIZE2-1; i>=0; i--) {
	M[j].x += __shfl_xor(M[j].x, 1<<i);
	M[j].y += __shfl_xor(M[j].y, 1<<i);
	M[j].z += __shfl_xor(M[j].z, 1<<i);
	M[j].w += __shfl_xor(M[j].w, 1<<i);
      }
      _M[j].x += M[j].x;
      _M[j].y += M[j].y;
      _M[j].z += M[j].z;
      _M[j].w += M[j].w;
    }
  }

  static __global__ __launch_bounds__(NTHREAD)
    void getMultipoles(const int numBodies,
		       const int numSources,
		       const CellData * cells,
		       const float4 * __restrict__ bodyPos,
		       const float invTheta,
		       float4 * sourceCenter,
		       float4 * Multipole) {
    const int laneIdx = threadIdx.x & (WARP_SIZE-1);
    const int warpIdx = threadIdx.x >> WARP_SIZE2;
    const int NWARP2  = NTHREAD2 - WARP_SIZE2;
    const int cellIdx = (blockIdx.x<<NWARP2) + warpIdx;
    if (cellIdx >= numSources) return;

    const CellData cell = cells[cellIdx];
    const float huge = 1e10f;
    float3 Xmin = {+huge,+huge,+huge};
    float3 Xmax = {-huge,-huge,-huge};
    double4 M[3];
    const int bodyBegin = cell.body();
    const int bodyEnd = cell.body() + cell.nbody();
    for (int i=bodyBegin; i<bodyEnd; i+=WARP_SIZE) {
      float4 body = bodyPos[min(i+laneIdx,bodyEnd-1)];
      if (i + laneIdx >= bodyEnd) body.w = 0.0f;
      getMinMax(Xmin, Xmax, make_float3(body.x,body.y,body.z));
      addMultipole(M, body);
    }
    const double invM = 1.0/M[0].w;
    M[0].x *= invM;
    M[0].y *= invM;
    M[0].z *= invM;
    M[1].x = M[1].x * invM - M[0].x * M[0].x;
    M[1].y = M[1].y * invM - M[0].y * M[0].y;
    M[1].z = M[1].z * invM - M[0].z * M[0].z;
    M[1].w = M[1].w * invM - M[0].x * M[0].y;
    M[2].x = M[2].x * invM - M[0].x * M[0].z;
    M[2].y = M[2].y * invM - M[0].y * M[0].z;
    const float3 X = {(Xmax.x+Xmin.x)*0.5f, (Xmax.y+Xmin.y)*0.5f, (Xmax.z+Xmin.z)*0.5f};
    const float3 R = {(Xmax.x-Xmin.x)*0.5f, (Xmax.y-Xmin.y)*0.5f, (Xmax.z-Xmin.z)*0.5f};
    const float3 com = {M[0].x, M[0].y, M[0].z};
    const float dx = X.x - com.x;
    const float dy = X.y - com.y;
    const float dz = X.z - com.z;
    const float  s = sqrt(dx*dx + dy*dy + dz*dz);
    const float  l = max(2.0f*max(R.x, max(R.y, R.z)), 1.0e-6f);
    const float cellOp = l*invTheta + s;
    const float cellOp2 = cellOp*cellOp;
    if (laneIdx == 0) {
      sourceCenter[cellIdx] = (float4){com.x, com.y, com.z, cellOp2};
      for (int i=0; i<3; i++) Multipole[3*cellIdx+i] = (float4){M[i].x, M[i].y, M[i].z, M[i].w};
    }
  }
}

class Pass {
 public:
  void upward(const float theta,
	      cudaVec<float4> & bodyPos,
	      cudaVec<CellData> & sourceCells,
	      cudaVec<float4> & sourceCenter,
	      cudaVec<float4> & Multipole) {
    const int numBodies = bodyPos.size();
    const int numSources = sourceCells.size();
    const int NWARP = 1 << (NTHREAD2 - WARP_SIZE2);
    const int NBLOCK = (numSources-1) / NWARP + 1;
    CUDA_SAFE_CALL(cudaFuncSetCacheConfig(&getMultipoles,cudaFuncCachePreferL1));
    cudaDeviceSynchronize();
    const double t0 = get_time();
    getMultipoles<<<NBLOCK,NTHREAD>>>(numBodies, numSources, sourceCells.d(),
				      bodyPos.d(), 1.0 / theta,
				      sourceCenter.d(), Multipole.d());
    kernelSuccess("getMultipoles");
    const double dt = get_time() - t0;
    fprintf(stdout,"Upward pass          : %.7f s\n", dt);
  }
};
