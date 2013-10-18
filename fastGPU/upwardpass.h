#pragma once

namespace {
  static __device__ __forceinline__
    float3 setCenter(int numBodies, float4 * __restrict__ bodyPos) {
    float mass;
    float3 center;
    for (int i=0; i<numBodies; i++) {
      const float4 body = bodyPos[i];
      mass += body.w;
      center.x += body.w * body.x;
      center.y += body.w * body.y;
      center.z += body.w * body.z;
    }
    const float invM = 1.0f / mass;
    center.x *= invM;
    center.y *= invM;
    center.z *= invM;
    return center;
  }

  static __device__ __forceinline__
    void pairMinMax(float3 & xmin, float3 & xmax,
		    float4 reg_min, float4 reg_max) {
    xmin.x = fminf(xmin.x, reg_min.x);
    xmin.y = fminf(xmin.y, reg_min.y);
    xmin.z = fminf(xmin.z, reg_min.z);
    xmax.x = fmaxf(xmax.x, reg_max.x);
    xmax.y = fmaxf(xmax.y, reg_max.y);
    xmax.z = fmaxf(xmax.z, reg_max.z);
  }

  static __global__ __launch_bounds__(NTHREAD)
    void collectLeafs(const int numCells,
                      const CellData * sourceCells,
                      int * leafCells) {
    const int NWARP = 1 << (NTHREAD2 - WARP_SIZE2);
    const int laneIdx = threadIdx.x & (WARP_SIZE-1);
    const int warpIdx = threadIdx.x >> WARP_SIZE2;
    const int cellIdx = blockDim.x * blockIdx.x + threadIdx.x;
    const CellData cell = sourceCells[min(cellIdx, numCells-1)];
    const bool isLeaf = cellIdx < numCells & cell.isLeaf();
    const int numLeafsLane = exclusiveScanBool(isLeaf);
    const int numLeafsWarp = reduceBool(isLeaf);
    __shared__ int numLeafsBase[NWARP];
    int & numLeafsScan = numLeafsBase[warpIdx];
    if (laneIdx == 0 && numLeafsWarp > 0)
      numLeafsScan = atomicAdd(&numLeafsGlob, numLeafsWarp);
    if (isLeaf)
      leafCells[numLeafsScan+numLeafsLane] = cellIdx;
  }

  static __global__ __launch_bounds__(NTHREAD)
    void P2M(const int numLeafs,
	     const float invTheta,
	     int * leafCells,
	     CellData * cells,
	     float4 * sourceCenter,
	     float4 * bodyPos,
	     float4 * cellXmin,
	     float4 * cellXmax,
	     float4 * Multipole) {
    const int leafIdx = blockIdx.x * blockDim.x + threadIdx.x;
    if (leafIdx >= numLeafs) return;
    int cellIdx = leafCells[leafIdx];
    const CellData cell = cells[cellIdx];
    const int begin = cell.body();
    const int size = cell.nbody();
    const int end = begin + size;
    const float3 com = setCenter(size,bodyPos+begin);
    float4 M[3];
    const float huge = 1e10f;
    float3 Xmin = {+huge, +huge, +huge};
    float3 Xmax = {-huge, -huge, -huge};
    for( int i=begin; i<end; i++ ) {
      float4 body = bodyPos[i];
      M[0].x += body.w * body.x;
      M[0].y += body.w * body.y;
      M[0].z += body.w * body.z;
      M[0].w += body.w;
      M[1].x += body.w * body.x * body.x;
      M[1].y += body.w * body.y * body.y;
      M[1].z += body.w * body.z * body.z;
      M[1].w += body.w * body.x * body.y;
      M[2].x += body.w * body.x * body.z;
      M[2].y += body.w * body.y * body.z;
      pairMinMax(Xmin, Xmax, body, body);
    }
    float invM = 1.0 / M[0].w;
    if(M[0].w == 0) invM = 0;
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
    const float dx = X.x - com.x;
    const float dy = X.y - com.y;
    const float dz = X.z - com.z;
    const float  s = sqrt(dx*dx + dy*dy + dz*dz);
    const float  l = max(2.0f*max(R.x, max(R.y, R.z)), 1.0e-6f);
    const float cellOp = l*invTheta + s;
    const float cellOp2 = cellOp*cellOp;
    sourceCenter[cellIdx] = make_float4(com.x, com.y, com.z, cellOp2);
    for (int i=0; i<3; i++) Multipole[3*cellIdx+i] = M[i];
    cellXmin[cellIdx] = make_float4(Xmin.x, Xmin.y, Xmin.z, 0.0f);
    cellXmax[cellIdx] = make_float4(Xmax.x, Xmax.y, Xmax.z, 0.0f);
    return;
  }

  static __global__ __launch_bounds__(NTHREAD)
    void M2M(const int level,
	     const float invTheta,
	     int2 * levelRange,
	     CellData * cells,
	     float4 * sourceCenter,
	     float4 * cellXmin,
	     float4 * cellXmax,
	     float4 * Multipole) {
    const int cellIdx = blockIdx.x * blockDim.x + threadIdx.x + levelRange[level].x;
    if (cellIdx >= levelRange[level].y) return;
    const CellData cell = cells[cellIdx];
    const int begin = cell.child();
    const int end = begin + cell.nchild();
    if (cell.isLeaf()) return;
    float4 Mi[3], Mj[3];
    const float huge = 1e10f;
    float3 Xmin = {+huge, +huge, +huge};
    float3 Xmax = {-huge, -huge, -huge};
    for( int i=begin; i<end; i++ ) {
      Mj[0] = Multipole[3*i];
      Mi[0].x += Mj[0].w * Mj[0].x;
      Mi[0].y += Mj[0].w * Mj[0].y;
      Mi[0].z += Mj[0].w * Mj[0].z;
      Mi[0].w += Mj[0].w;
      Mi[1].x += Mj[0].w * (Mj[0].x * Mj[0].x + Mj[1].x);
      Mi[1].y += Mj[0].w * (Mj[0].y * Mj[0].y + Mj[1].y);
      Mi[1].z += Mj[0].w * (Mj[0].z * Mj[0].z + Mj[1].z);
      Mi[1].w += Mj[0].w * (Mj[0].x * Mj[0].y + Mj[1].w);
      Mi[2].x += Mj[0].w * (Mj[0].x * Mj[0].z + Mj[2].x);
      Mi[2].y += Mj[0].w * (Mj[0].y * Mj[0].z + Mj[2].y);
      pairMinMax(Xmin, Xmax, cellXmin[i], cellXmax[i]);
    }
    float invM = 1.0 / Mi[0].w;
    if(Mi[0].w == 0) invM = 0;
    Mi[0].x *= invM;
    Mi[0].y *= invM;
    Mi[0].z *= invM;
    Mi[1].x = Mi[1].x * invM - Mi[0].x * Mi[0].x;
    Mi[1].y = Mi[1].y * invM - Mi[0].y * Mi[0].y;
    Mi[1].z = Mi[1].z * invM - Mi[0].z * Mi[0].z;
    Mi[1].w = Mi[1].w * invM - Mi[0].x * Mi[0].y;
    Mi[2].x = Mi[2].x * invM - Mi[0].x * Mi[0].z;
    Mi[2].y = Mi[2].y * invM - Mi[0].y * Mi[0].z;
    const float3 X = {(Xmax.x+Xmin.x)*0.5f, (Xmax.y+Xmin.y)*0.5f, (Xmax.z+Xmin.z)*0.5f};
    const float3 R = {(Xmax.x-Xmin.x)*0.5f, (Xmax.y-Xmin.y)*0.5f, (Xmax.z-Xmin.z)*0.5f};
    const float3 com = {Mi[0].x, Mi[0].y, Mi[0].z};
    const float dx = X.x - com.x;
    const float dy = X.y - com.y;
    const float dz = X.z - com.z;
    const float  s = sqrt(dx*dx + dy*dy + dz*dz);
    const float  l = max(2.0f*max(R.x, max(R.y, R.z)), 1.0e-6f);
    const float cellOp = l*invTheta + s;
    const float cellOp2 = cellOp*cellOp;
    sourceCenter[cellIdx] = make_float4(com.x, com.y, com.z, cellOp2);
    for (int i=0; i<3; i++) Multipole[3*cellIdx+i] = Mi[i];
    cellXmin[cellIdx] = make_float4(Xmin.x, Xmin.y, Xmin.z, 0.0f);
    cellXmax[cellIdx] = make_float4(Xmax.x, Xmax.y, Xmax.z, 0.0f);
    return;
  }
}

class Pass {
 public:
  void upward(const int numLeafs,
	      const int numLevels,
	      const float theta,
	      cudaVec<int2> & levelRange,
	      cudaVec<float4> & bodyPos,
	      cudaVec<CellData> & sourceCells,
	      cudaVec<float4> & sourceCenter,
	      cudaVec<float4> & Multipole) {
    int numCells = sourceCells.size();
    int NBLOCK = (numCells-1) / NTHREAD + 1;
    cudaVec<int> leafCells(numLeafs);
    collectLeafs<<<NBLOCK,NTHREAD>>>(numCells, sourceCells.d(), leafCells.d());
    kernelSuccess("collectLeafs");
    const double t0 = get_time();
    cudaVec<float4> cellXmin(numCells);
    cudaVec<float4> cellXmax(numCells);
    CUDA_SAFE_CALL(cudaFuncSetCacheConfig(&P2M,cudaFuncCachePreferL1));
    CUDA_SAFE_CALL(cudaFuncSetCacheConfig(&M2M,cudaFuncCachePreferL1));
    cudaDeviceSynchronize();
    NBLOCK = (numLeafs - 1) / NTHREAD + 1;
    P2M<<<NBLOCK,NTHREAD>>>(numLeafs,1.0/theta,leafCells.d(),sourceCells.d(),sourceCenter.d(),
			    bodyPos.d(),cellXmin.d(),cellXmax.d(),Multipole.d());
    kernelSuccess("P2M");
    levelRange.d2h();
    for( int level=numLevels; level>=1; level-- ) {
      numCells = levelRange[level].y - levelRange[level].x;
      NBLOCK = (numCells - 1) / NTHREAD + 1;
      M2M<<<NBLOCK,NTHREAD>>>(level,1.0/theta,levelRange.d(),sourceCells.d(),sourceCenter.d(),
			      cellXmin.d(),cellXmax.d(),Multipole.d());
      kernelSuccess("M2M");
    }
    const double dt = get_time() - t0;
    fprintf(stdout,"Upward pass          : %.7f s\n", dt);
  }
};
