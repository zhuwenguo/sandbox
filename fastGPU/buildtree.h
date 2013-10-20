#pragma once
#include "warpscan.h"

extern void sort(const int size, int * key, int * value);

namespace {
  __constant__ int maxNodesGlob;
  __device__ float4 domainGlob;
  __device__ unsigned int counterGlob = 0;
  __device__ unsigned int numNodesGlob = 0;
  __device__ unsigned int numLeafsGlob = 0;
  __device__ unsigned int numLevelsGlob = 0;
  __device__ unsigned int numCellsGlob = 0;
  __device__ int * octantSizePool;
  __device__ int * octantSizeScanPool;
  __device__ int * subOctantSizeScanPool;
  __device__ int * blockCounterPool;
  __device__ int2 * bodyRangePool;
  __device__ CellData * sourceCells;

  static __device__ __forceinline__
    int getOctant(const float4 &box, const float4 &body) {
    return ((box.x <= body.x) << 0) + ((box.y <= body.y) << 1) + ((box.z <= body.z) << 2);
  }

  static __device__ __forceinline__
    float4 getChild(const float4 &box, const int octant) {
    const float R = 0.5f * box.w;
    return make_float4(box.x + R * (octant & 1 ? 1.0f : -1.0f),
		       box.y + R * (octant & 2 ? 1.0f : -1.0f),
		       box.z + R * (octant & 4 ? 1.0f : -1.0f),
		       R);
  }

  static __device__
    float2 getMinMax(float2 range) {
    __shared__ float sharedMin[NTHREAD];
    __shared__ float sharedMax[NTHREAD];
    sharedMin[threadIdx.x] = range.x;
    sharedMax[threadIdx.x] = range.y;
    __syncthreads();
#pragma unroll
    for (int i=NTHREAD2-1; i>=6; i--) {
      const int offset = 1 << i;
      if (threadIdx.x < offset) {
	sharedMin[threadIdx.x] = range.x = fminf(range.x, sharedMin[threadIdx.x + offset]);
	sharedMax[threadIdx.x] = range.y = fmaxf(range.y, sharedMax[threadIdx.x + offset]);
      }
      __syncthreads();
    }
    if (threadIdx.x < WARP_SIZE) {
      volatile float *warpMin = sharedMin;
      volatile float *warpMax = sharedMax;
#pragma unroll
      for (int i=5; i>=0; i--) {
	const int offset = 1 << i;
	warpMin[threadIdx.x] = range.x = fminf(range.x, warpMin[threadIdx.x + offset]);
	warpMax[threadIdx.x] = range.y = fmaxf(range.y, warpMax[threadIdx.x + offset]);
      }
    }
    __syncthreads();
    return range;
  }

  static __global__
    void getBounds(const int numBodies,
		   float3 * bounds,
		   const float4 * bodyPos) {
    const int NBLOCK = NTHREAD;
    const int begin = blockIdx.x * blockDim.x + threadIdx.x;
    float3 Xmin = {bodyPos[0].x, bodyPos[0].y, bodyPos[0].z};
    float3 Xmax = Xmin;
    for (int i=begin; i<numBodies; i+=NBLOCK*NTHREAD) {
      if (i < numBodies) {
	const float4 pos = bodyPos[i];
	Xmin.x = fmin(Xmin.x, pos.x);
	Xmin.y = fmin(Xmin.y, pos.y);
	Xmin.z = fmin(Xmin.z, pos.z);
	Xmax.x = fmax(Xmax.x, pos.x);
	Xmax.y = fmax(Xmax.y, pos.y);
	Xmax.z = fmax(Xmax.z, pos.z);
      }
    }
    float2 range;
    range = getMinMax(make_float2(Xmin.x, Xmax.x)); Xmin.x = range.x; Xmax.x = range.y;
    range = getMinMax(make_float2(Xmin.y, Xmax.y)); Xmin.y = range.x; Xmax.y = range.y;
    range = getMinMax(make_float2(Xmin.z, Xmax.z)); Xmin.z = range.x; Xmax.z = range.y;
    if (threadIdx.x == 0) {
      bounds[blockIdx.x         ] = Xmin;
      bounds[blockIdx.x + NBLOCK] = Xmax;
    }
    __shared__ bool lastBlock;
    __threadfence();
    __syncthreads();
    if (threadIdx.x == 0) {
      const int blockCount = atomicInc(&counterGlob, NBLOCK);
      lastBlock = (blockCount == NBLOCK-1);
    }
    __syncthreads();
    if (lastBlock) {
      Xmin = bounds[threadIdx.x];
      Xmax = bounds[threadIdx.x + NBLOCK];
      range = getMinMax(make_float2(Xmin.x, Xmax.x)); Xmin.x = range.x; Xmax.x = range.y;
      range = getMinMax(make_float2(Xmin.y, Xmax.y)); Xmin.y = range.x; Xmax.y = range.y;
      range = getMinMax(make_float2(Xmin.z, Xmax.z)); Xmin.z = range.x; Xmax.z = range.y;
      __syncthreads();
      if (threadIdx.x == 0) {
	const float3 X = {(Xmax.x+Xmin.x)*0.5f, (Xmax.y+Xmin.y)*0.5f, (Xmax.z+Xmin.z)*0.5f};
	const float3 R = {(Xmax.x-Xmin.x)*0.5f, (Xmax.y-Xmin.y)*0.5f, (Xmax.z-Xmin.z)*0.5f};
	const float r = fmax(R.x, fmax(R.y, R.z)) * 1.1f;
	const float4 box = {X.x, X.y, X.z, r};
	domainGlob = box;
	counterGlob = 0;
      }
    }
  }

  template<int NCRIT, bool ISROOT>
    static __global__ __launch_bounds__(NTHREAD, 8)
    void buildOctant(float4 box,
		     const int cellParentIndex,
		     const int cellIndexBase,
		     const int packedOctant,
		     int * octantSizeBase,
		     int * octantSizeScanBase,
		     int * subOctantSizeScanBase,
		     int * blockCounterBase,
		     int2 * bodyRangeBase,
		     float4 * bodyPos,
		     float4 * bodyPos2,
		     const int level = 0) {
    const int NWARP2 = NTHREAD2 - WARP_SIZE2;
    const int NWARP = 1 << NWARP2;
    const int laneIdx = threadIdx.x & (WARP_SIZE-1);
    const int warpIdx = threadIdx.x >> WARP_SIZE2;
    int * octantSize = octantSizeBase + blockIdx.y * 8;
    int * octantSizeScan = octantSizeScanBase + blockIdx.y * 8;
    int * subOctantSizeScan = subOctantSizeScanBase + blockIdx.y * 64;
    int * blockCounter = blockCounterBase + blockIdx.y;
    int2 * bodyRange = bodyRangeBase + blockIdx.y;
    const int bodyBegin = bodyRange->x + blockIdx.x * blockDim.x + warpIdx * WARP_SIZE;
    const int bodyEnd   = bodyRange->y;
    const int numBodies = bodyRange->y - bodyRange->x;
    const int childOctant = (packedOctant >> (3*blockIdx.y)) & 0x7;
    __shared__ int subOctantSizeLane[NWARP*8*8];
    __shared__ int subOctantSize[8*8];
    float4 *childBox = (float4*)subOctantSize;

    for (int i=0; i<8*8*NWARP; i+=blockDim.x)
      if (i+threadIdx.x < 8*8*NWARP)
	subOctantSizeLane[i+threadIdx.x] = 0;
    if (!ISROOT) box = getChild(box, childOctant);
    if (laneIdx == 0)
      childBox[warpIdx] = getChild(box, warpIdx);               // One child per warp

    __syncthreads();

    for (int i=bodyBegin; i<bodyEnd; i+=gridDim.x*blockDim.x) {
      const int bodyIdx = min(i+laneIdx, bodyEnd-1);
      float4 pos = bodyPos[bodyIdx];
      int bodyOctant = getOctant(box, pos);
      int bodySubOctant = getOctant(childBox[bodyOctant], pos);
      if (i+laneIdx > bodyIdx)
	bodyOctant = bodySubOctant = 8;                         // Out of bounds lanes

      int octantSizeScanLane = 0;
#pragma unroll
      for (int octant=0; octant<8; octant++) {
	const int sumLane = reduceBool(bodyOctant == octant);   // Count current octant in warp
	if (octant == laneIdx)
	  octantSizeScanLane = sumLane;                          // Use lanes 0-7 for each octant
      }
      int octantOffset;
      if (laneIdx < 8)
	octantOffset = atomicAdd(&octantSizeScan[laneIdx], octantSizeScanLane);// Global scan

      int bodyIdx2 = -1;
#pragma unroll
      for (int octant=0; octant<8; octant++) {
	const int sumLane = reduceBool(bodyOctant == octant);
	if (sumLane > 0) {                                      // Avoid redundant instructions
	  const int index = exclusiveScanBool(bodyOctant == octant);// Sparse lane index
	  const int offset = __shfl(octantOffset, octant);      // Global offset
	  if (bodyOctant == octant)                             // Prevent overwrite
	    bodyIdx2 = offset + index;                          // Sorted index
	}
      }
      if (bodyIdx2 >= 0)
        bodyPos2[bodyIdx2] = pos;                               // Assign value to sort buffer

      int remainder = 32;
#pragma unroll
      for (int octant=0; octant<8; octant++) {
	if (remainder == 0) break;
	const int sumLane = reduceBool(bodyOctant == octant);
	if (sumLane > 0) {
	  const int bodySubOctantValid = bodyOctant == octant ? bodySubOctant : 8;
#pragma unroll
	  for (int subOctant=0; subOctant<8; subOctant+=4) {
	    const int4 sum4 = make_int4(reduceBool(subOctant+0 == bodySubOctantValid),
					reduceBool(subOctant+1 == bodySubOctantValid),
					reduceBool(subOctant+2 == bodySubOctantValid),
					reduceBool(subOctant+3 == bodySubOctantValid));
	    if (laneIdx == 0) {
	      int4 subOctantTemp = *(int4*)&subOctantSizeLane[warpIdx*64+octant*8+subOctant];
	      subOctantTemp.x += sum4.x;
	      subOctantTemp.y += sum4.y;
	      subOctantTemp.z += sum4.z;
	      subOctantTemp.w += sum4.w;
	      *(int4*)&subOctantSizeLane[warpIdx*64+octant*8+subOctant] = subOctantTemp;
	    }
	  }
	  remainder -= sumLane;
	}
      }
    }
    __syncthreads();                                            // Sync subOctantSizeLane

#pragma unroll
    for (int k=0; k<8; k+=4) {
      int4 subOctantTemp = laneIdx < NWARP ? (*(int4*)&subOctantSizeLane[laneIdx*64+warpIdx*8+k]) : make_int4(0,0,0,0);
#pragma unroll
      for (int i=NWARP2-1; i>=0; i--) {
	subOctantTemp.x += __shfl_xor(subOctantTemp.x, 1<<i, NWARP);
	subOctantTemp.y += __shfl_xor(subOctantTemp.y, 1<<i, NWARP);
	subOctantTemp.z += __shfl_xor(subOctantTemp.z, 1<<i, NWARP);
	subOctantTemp.w += __shfl_xor(subOctantTemp.w, 1<<i, NWARP);
      }
      if (laneIdx == 0)
	*(int4*)&subOctantSize[warpIdx*8+k] = subOctantTemp;
    }
    if (laneIdx < 8)
      if (subOctantSize[warpIdx*8+laneIdx] > 0)
	atomicAdd(&subOctantSizeScan[warpIdx*8+laneIdx], subOctantSize[warpIdx*8+laneIdx]);
    __syncthreads();                                            // Sync subOctantSizeScan, subOctantSize

    __shared__ bool lastBlock;
    if (threadIdx.x == 0) {
      const int blockCount = atomicAdd(blockCounter, 1);
      lastBlock = (blockCount == gridDim.x-1);
    }
    __syncthreads();                                            // Sync lastBlock

    if (!lastBlock) return;
    __syncthreads();                                            // Sync return

    if (threadIdx.x == 0)
      atomicCAS(&numLevelsGlob, level, level+1);
    __syncthreads();                                            // Sync numLevelsGlob

    const int numBodiesOctant = octantSize[warpIdx];
    const int bodyEndOctant = octantSizeScan[warpIdx];
    const int bodyBeginOctant = bodyEndOctant - numBodiesOctant;
    const int numBodiesOctantLane = laneIdx < 8 ? octantSize[laneIdx] : 0;
    const int numNodesLane = exclusiveScanBool(numBodiesOctantLane > NCRIT);
    const int numLeafsLane = exclusiveScanBool(0 < numBodiesOctantLane && numBodiesOctantLane <= NCRIT);
    int * numNodes = subOctantSize;                             // Reuse shared memory
    int * numLeafs = subOctantSize + 8;
    int & numNodesScan = subOctantSize[16];
    int & numCellsScan = subOctantSize[17];
    if (warpIdx == 0 && laneIdx < 8) {
      numNodes[laneIdx] = numNodesLane;
      numLeafs[laneIdx] = numLeafsLane;
    }

    int maxBodiesOctant = numBodiesOctantLane;
#pragma unroll
    for (int i=2; i>=0; i--)
      maxBodiesOctant = max(maxBodiesOctant, __shfl_xor(maxBodiesOctant, 1<<i));
    const int numNodesWarp = reduceBool(numBodiesOctantLane > NCRIT);
    if (threadIdx.x == 0 && numNodesWarp > 0) {
      numNodesScan = atomicAdd(&numNodesGlob, numNodesWarp);
      assert(numNodesScan < maxNodesGlob);
    }

    const int numChildWarp = reduceBool(numBodiesOctantLane > 0);
    if (threadIdx.x == 0 && numChildWarp > 0) {
      numCellsScan = atomicAdd(&numCellsGlob, numChildWarp);
      const CellData cellData(level, cellParentIndex, bodyRange->x, numBodies, numCellsScan, numChildWarp);
      sourceCells[cellIndexBase + blockIdx.y] = cellData;
    }
    __syncthreads();                                            // Sync numCellsScan, sourceCells

    octantSizeBase = octantSizePool + numNodesScan * 8;         // Global offset
    octantSizeScanBase = octantSizeScanPool + numNodesScan * 8;
    subOctantSizeScanBase = subOctantSizeScanPool + numNodesScan * 64;
    blockCounterBase = blockCounterPool + numNodesScan;
    bodyRangeBase = bodyRangePool + numNodesScan;

    const int nodeOffset = numNodes[warpIdx];
    const int leafOffset = numLeafs[warpIdx];

    if (numBodiesOctant > NCRIT) {
      octantSize = octantSizeBase + nodeOffset * 8;             // Warp offset
      octantSizeScan = octantSizeScanBase + nodeOffset * 8;
      blockCounter = blockCounterBase + nodeOffset;
      bodyRange = bodyRangeBase + nodeOffset;

      const int newOctantSize = laneIdx < 8 ? subOctantSizeScan[warpIdx*8+laneIdx] : 0;
      int newOctantSizeScan = inclusiveScanInt(newOctantSize);
      newOctantSizeScan -= newOctantSize;
      if (laneIdx < 8) {
        octantSizeScan[laneIdx] = bodyBeginOctant + newOctantSizeScan;
  	octantSize[laneIdx] = newOctantSize;
      }
      if (laneIdx == 0) {
        *blockCounter = 0;
        bodyRange->x = bodyBeginOctant;
        bodyRange->y = bodyEndOctant;
      }
    }

    if (numNodesWarp > 0 && warpIdx == 0) {
      int packedOctant = numBodiesOctantLane > NCRIT ? laneIdx << (3*numNodesLane) : 0;
#pragma unroll
      for (int i=4; i>=0; i--)
	packedOctant |= __shfl_xor(packedOctant, 1<<i);

      if (threadIdx.x == 0) {
	dim3 NBLOCK = min(max(maxBodiesOctant / NTHREAD, 1), 512);
        NBLOCK.y = numNodesWarp;
	buildOctant<NCRIT,false><<<NBLOCK,NTHREAD>>>
	  (box, cellIndexBase+blockIdx.y, numCellsScan, packedOctant, octantSizeBase, octantSizeScanBase,
	   subOctantSizeScanBase, blockCounterBase, bodyRangeBase, bodyPos2, bodyPos, level+1);
      }
    }

    if (numBodiesOctant <= NCRIT && numBodiesOctant > 0) {
      if (laneIdx == 0) {
	atomicAdd(&numLeafsGlob, 1);
	const CellData leafData(level+1, cellIndexBase+blockIdx.y, bodyBeginOctant, bodyEndOctant-bodyBeginOctant);
	sourceCells[numCellsScan + numNodesWarp + leafOffset] = leafData;
      }
      if (!(level & 1)) {
	for (int bodyIdx=bodyBeginOctant+laneIdx; bodyIdx<bodyEndOctant; bodyIdx+=WARP_SIZE) {
	  if (bodyIdx < bodyEndOctant) {
	    bodyPos[bodyIdx] = bodyPos2[bodyIdx];
	  }
        }
      }
    }
  }

  static __global__ void getRootOctantSize(const int numBodies,
				           int * octantSize,
				           const float4 box,
				           const float4 *bodyPos) {
    const int laneIdx = threadIdx.x & (WARP_SIZE-1);
    const int begin = blockIdx.x * blockDim.x + threadIdx.x;
    int octantSizeLane[8] = {0};
    for (int i=begin; i<numBodies; i+=gridDim.x*blockDim.x) {
      const float4 pos = bodyPos[i];
      const int octant = getOctant(box, pos);
      octantSizeLane[0] += (octant == 0);
      octantSizeLane[1] += (octant == 1);
      octantSizeLane[2] += (octant == 2);
      octantSizeLane[3] += (octant == 3);
      octantSizeLane[4] += (octant == 4);
      octantSizeLane[5] += (octant == 5);
      octantSizeLane[6] += (octant == 6);
      octantSizeLane[7] += (octant == 7);
    }
#pragma unroll
    for (int k=0; k<8; k++) {
      int octantSizeTemp = octantSizeLane[k];
#pragma unroll
      for (int i=4; i>=0; i--)
	octantSizeTemp += __shfl_xor(octantSizeTemp, 1<<i);
      if (laneIdx == 0)
	atomicAdd(&octantSize[k], octantSizeTemp);
    }
  }

  template<int NCRIT>
    static __global__
    void buildOctree(const int numBodies,
		     CellData * d_sourceCells,
		     int * d_octantSizePool,
		     int * d_octantSizeScanPool,
		     int * d_subOctantSizeScanPool,
		     int * d_blockCounterPool,
		     int2 * d_bodyRangePool,
		     float4 * d_bodyPos,
		     float4 * d_bodyPos2) {
    sourceCells = d_sourceCells;
    octantSizePool = d_octantSizePool;
    octantSizeScanPool = d_octantSizeScanPool;
    subOctantSizeScanPool = d_subOctantSizeScanPool;
    blockCounterPool = d_blockCounterPool;
    bodyRangePool = d_bodyRangePool;

    int * octantSize = new int[8];
    for (int k=0; k<8; k++)
      octantSize[k] = 0;
    getRootOctantSize<<<NTHREAD, NTHREAD>>>(numBodies, octantSize, domainGlob, d_bodyPos);
    assert(cudaGetLastError() == cudaSuccess);
    cudaDeviceSynchronize();

    int * octantSizeScan = new int[8];
    int * subOctantSizeScan = new int[64];
    int * blockCounter = new int;
    int2 * bodyRange = new int2;
#pragma unroll
    for (int k=0; k<8; k++)
      octantSizeScan[k] = k == 0 ? 0 : octantSizeScan[k-1] + octantSize[k-1];
#pragma unroll
    for (int k=0; k<64; k++)
      subOctantSizeScan[k] = 0;

    numNodesGlob = 0;
    numLeafsGlob = 0;
    numLevelsGlob = 0;
    numCellsGlob  = 0;
    *blockCounter = 0;
    bodyRange->x = 0;
    bodyRange->y = numBodies;
    const int NBLOCK = min(max(numBodies / NTHREAD, 1), 512);
    buildOctant<NCRIT,true><<<NBLOCK, NTHREAD>>>
      (domainGlob, 0, 0, 0, octantSize, octantSizeScan, subOctantSizeScan, blockCounter, bodyRange, d_bodyPos, d_bodyPos2);
    assert(cudaDeviceSynchronize() == cudaSuccess);
    delete [] octantSize;
    delete [] octantSizeScan;
    delete [] subOctantSizeScan;
    delete [] blockCounter;
    delete [] bodyRange;
  }


  static __global__
    void getKeys(const int numCells,
		 const CellData * sourceCells,
		 CellData * sourceCells2,
		 int * key,
		 int * value) {
    const int cellIdx = blockIdx.x * blockDim.x + threadIdx.x;
    if (cellIdx >= numCells) return;
    const CellData cell = sourceCells[cellIdx];
    key  [cellIdx] = cell.level();
    value[cellIdx] = cellIdx;
    sourceCells2[cellIdx] = cell;
  }

  static __global__
    void getLevelRange(const int numCells,
		       const int * levels,
		       int2 * levelRange) {
    const int cellIdx = blockIdx.x * blockDim.x + threadIdx.x;
    if (cellIdx >= numCells) return;
    const int nextCellIdx = min(cellIdx+1, numCells-1);
    const int prevCellIdx = max(cellIdx-1, 0);
    const int level = levels[cellIdx];
    if (levels[prevCellIdx] < level || cellIdx == 0)
      levelRange[level].x = cellIdx;
    if (level < levels[nextCellIdx] || cellIdx == numCells-1)
      levelRange[level].y = cellIdx+1;
  }

  static __global__
    void getPermutation(const int numCells,
			const int * value,
			int * key) {
    const int newIdx = blockIdx.x * blockDim.x + threadIdx.x;
    if (newIdx >= numCells) return;
    const int oldIdx = value[newIdx];
    key[oldIdx] = newIdx;
  }

  static __global__
    void permuteCells(const int numCells,
		      const int * value,
		      const int * key,
		      const CellData * sourceCells2,
		      CellData * sourceCells) {
    const int cellIdx = blockIdx.x * blockDim.x + threadIdx.x;
    if (cellIdx >= numCells) return;
    const int mapIdx = value[cellIdx];
    CellData cell = sourceCells2[mapIdx];
    if (cell.isNode())
      cell.setChild(key[cell.child()]);
    if (cell.parent() > 0)
      cell.setParent(key[cell.parent()]);
    sourceCells[cellIdx] = cell;
    if (cellIdx == 0) numLeafsGlob = 0;
  }
}

class Build {
 public:
  template<int NCRIT>
    int3 tree(cudaVec<float4> & bodyPos,
	      cudaVec<float4> & bodyPos2,
	      float4 & domain,
	      cudaVec<int2> & levelRange,
	      cudaVec<CellData> & sourceCells) {
    const int numBodies = bodyPos.size();
    const int maxNode = numBodies / 10;
    cudaVec<float3> bounds(2*NTHREAD);
    cudaVec<int> octantSizePool(8*maxNode);
    cudaVec<int> octantSizeScanPool(8*maxNode);
    cudaVec<int> subOctantSizeScanPool(64*maxNode);
    cudaVec<int> blockCounterPool(maxNode);
    cudaVec<int2> bodyRangePool(maxNode);
    cudaVec<int> key(numBodies);
    cudaVec<int> value(numBodies);
    fprintf(stdout,"Stack size           : %g MB\n",83*maxNode*sizeof(int)/1024.0/1024.0);
    fprintf(stdout,"Cell data            : %g MB\n",numBodies*sizeof(CellData)/1024.0/1024.0);
    cudaDeviceSynchronize();

    double t0 = get_time();
    getBounds<<<NTHREAD,NTHREAD>>>(numBodies, bounds.d(), bodyPos.d());
    kernelSuccess("getBounds");
    double dt = get_time() - t0;
    fprintf(stdout,"Get bounds           : %.7f s\n",  dt);

    CUDA_SAFE_CALL(cudaMemcpyToSymbol(maxNodesGlob, &maxNode, sizeof(int), 0, cudaMemcpyHostToDevice));
    octantSizePool.zeros();
    octantSizeScanPool.zeros();
    subOctantSizeScanPool.zeros();
    blockCounterPool.zeros();
    bodyRangePool.zeros();
    cudaDeviceSynchronize();

    t0 = get_time();
    CUDA_SAFE_CALL(cudaDeviceSetLimit(cudaLimitDevRuntimePendingLaunchCount, 16384));
    CUDA_SAFE_CALL(cudaDeviceSetSharedMemConfig(cudaSharedMemBankSizeEightByte));
    CUDA_SAFE_CALL(cudaFuncSetCacheConfig(&buildOctant<NCRIT,true>,  cudaFuncCachePreferShared));
    CUDA_SAFE_CALL(cudaFuncSetCacheConfig(&buildOctant<NCRIT,false>, cudaFuncCachePreferShared));
    buildOctree<NCRIT><<<1,1>>>(numBodies, sourceCells.d(), octantSizePool.d(),
				octantSizeScanPool.d(), subOctantSizeScanPool.d(),
				blockCounterPool.d(), bodyRangePool.d(),
				bodyPos.d(), bodyPos2.d());
    kernelSuccess("buildOctree");
    dt = get_time() - t0;
    fprintf(stdout,"Grow tree            : %.7f s\n",  dt);
    int numLevels, numCells, numLeafs;
    CUDA_SAFE_CALL(cudaMemcpyFromSymbol(&domain, domainGlob, sizeof(float4)));
    CUDA_SAFE_CALL(cudaMemcpyFromSymbol(&numLevels, numLevelsGlob, sizeof(int)));
    CUDA_SAFE_CALL(cudaMemcpyFromSymbol(&numCells, numCellsGlob, sizeof(int)));
    CUDA_SAFE_CALL(cudaMemcpyFromSymbol(&numLeafs, numLeafsGlob, sizeof(int)));
    cudaDeviceSynchronize();

    t0 = get_time();
    const int NBLOCK = (numCells-1) / NTHREAD + 1;
    cudaVec<CellData> sourceCells2(numCells);
    getKeys<<<NBLOCK,NTHREAD>>>(numCells, sourceCells.d(), sourceCells2.d(), key.d(), value.d());
    kernelSuccess("getKeys");
    sort(numCells, key.d(), value.d());
    getLevelRange<<<NBLOCK,NTHREAD>>>(numCells, key.d(), levelRange.d());
    kernelSuccess("getLevelRange");
    getPermutation<<<NBLOCK,NTHREAD>>>(numCells, value.d(), key.d());
    kernelSuccess("getPermutation");
    sourceCells.alloc(numCells);
    permuteCells<<<NBLOCK,NTHREAD>>>(numCells, value.d(), key.d(), sourceCells2.d(), sourceCells.d());
    kernelSuccess("permuteCells");
    dt = get_time() - t0;
    fprintf(stdout,"Link tree            : %.7f s\n", dt);
    int3 counts = {numLevels, numCells, numLeafs};
    return counts;
  }
};
