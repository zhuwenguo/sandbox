#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <omp.h>
#include <sys/time.h>
#include <xmmintrin.h>

#define LOG_POLY_DEGREE 5
#define POLY0(x, c0) _mm_set1_ps(c0)
#define POLY1(x, c0, c1) _mm_add_ps(_mm_mul_ps(POLY0(x, c1), x), _mm_set1_ps(c0))
#define POLY2(x, c0, c1, c2) _mm_add_ps(_mm_mul_ps(POLY1(x, c1, c2), x), _mm_set1_ps(c0))
#define POLY3(x, c0, c1, c2, c3) _mm_add_ps(_mm_mul_ps(POLY2(x, c1, c2, c3), x), _mm_set1_ps(c0))
#define POLY4(x, c0, c1, c2, c3, c4) _mm_add_ps(_mm_mul_ps(POLY3(x, c1, c2, c3, c4), x), _mm_set1_ps(c0))
#define POLY5(x, c0, c1, c2, c3, c4, c5) _mm_add_ps(_mm_mul_ps(POLY4(x, c1, c2, c3, c4, c5), x), _mm_set1_ps(c0))

__m128 log2f4(__m128 x) {
   __m128i exp = _mm_set1_epi32(0x7F800000);
   __m128i mant = _mm_set1_epi32(0x007FFFFF);
   __m128 one = _mm_set1_ps(1.0f);
   __m128i i = _mm_castps_si128(x);
   __m128 e = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_srli_epi32(_mm_and_si128(i, exp), 23), _mm_set1_epi32(127)));
   __m128 m = _mm_or_ps(_mm_castsi128_ps(_mm_and_si128(i, mant)), one);
   __m128 p;
   /* Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[ */
#if LOG_POLY_DEGREE == 6
   p = POLY5( m, 3.1157899f, -3.3241990f, 2.5988452f, -1.2315303f,  3.1821337e-1f, -3.4436006e-2f);
#elif LOG_POLY_DEGREE == 5
   p = POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
#elif LOG_POLY_DEGREE == 4
   p = POLY3(m, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
#elif LOG_POLY_DEGREE == 3
   p = POLY2(m, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
#else
#error
#endif
   /* This effectively increases the polynomial degree by one, but ensures that log2(1) == 0*/
   p = _mm_mul_ps(p, _mm_sub_ps(m, one));
   return _mm_add_ps(p, e);
}

const int N = 128*200;
const int NCRIT = 100;
const int MAXLEVEL = N >= NCRIT ? 1 + int(log(N / NCRIT)/M_LN2/2) : 0;
const float THETA = 0.75;
const float EPS2 = 0.00001;
const float X0 = .5;
const float R0 = .5;

double get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return double(tv.tv_sec+tv.tv_usec*1e-6);
}

struct Body {
  int ICELL;
  float X[2];
  float M;
  float P;
};

struct Cell {
  int ICELL;
  int NCHILD;
  int NCLEAF;
  int NDLEAF;
  int CHILD;
  int LEAF;
  float X[2];
  float R;
  float M[4];
};

void getIndex(Body *bodies) {
  float diameter = 2 * R0 / (1 << MAXLEVEL);
  for( int i=0; i<N; i++ ) {
    int ix = (bodies[i].X[0] + R0 - X0) / diameter;
    int iy = (bodies[i].X[1] + R0 - X0) / diameter;
    int icell = 0;
    for( int l=0; l!=MAXLEVEL; ++l ) {
      icell += (ix & 1) << (2 * l);
      icell += (iy & 1) << (2 * l + 1);
      ix >>= 1;
      iy >>= 1;
    }
    bodies[i].ICELL = icell;
  }
}

void sortBody(Body *bodies, Body *buffer) {
  int Imin = bodies[0].ICELL;
  int Imax = bodies[0].ICELL;
  for( int i=0; i<N; ++i ) {
    if     ( bodies[i].ICELL < Imin ) Imin = bodies[i].ICELL;
    else if( bodies[i].ICELL > Imax ) Imax = bodies[i].ICELL;
  }
  int numBucket = Imax - Imin + 1;
  int *bucket = new int [numBucket];
  for( int i=0; i<numBucket; i++ ) bucket[i] = 0;
  for( int i=0; i!=N; i++ ) bucket[bodies[i].ICELL-Imin]++;
  for( int i=1; i!=numBucket; i++ ) bucket[i] += bucket[i-1];
  for( int i=N-1; i>=0; i-- ) {
    bucket[bodies[i].ICELL-Imin]--;
    int inew = bucket[bodies[i].ICELL-Imin];
    buffer[inew] = bodies[i];
  }
  for( int i=0; i<N; i++ ) bodies[i] = buffer[i];
  delete[] bucket;
}

void initCell(const Body *bodies, Cell &cell, const int &icell, const int &leaf, const float &diameter) {
  cell.ICELL = icell;
  cell.NCHILD = 0;
  cell.NCLEAF = 0;
  cell.NDLEAF = 0;
  cell.LEAF   = leaf;
  int ix = (bodies[leaf].X[0] + R0 - X0) / diameter;
  int iy = (bodies[leaf].X[1] + R0 - X0) / diameter;
  cell.X[0]   = diameter * (ix + .5) + X0 - R0;
  cell.X[1]   = diameter * (iy + .5) + X0 - R0;
  cell.R      = diameter * .5;
  for( int i=0; i<4; i++ ) cell.M[i] = 0;
}

void buildCell(const Body *bodies, Cell *cells, int &ncell) {
  int oldcell = -1;
  ncell = 0;
  float diameter = 2 * R0 / (1 << MAXLEVEL);
  for( int i=0; i<N; i++ ) {
    int icell = bodies[i].ICELL;
    if( icell != oldcell ) {
      initCell(bodies,cells[ncell],icell,i,diameter);
      oldcell = icell;
      ncell++;
    }
    cells[ncell-1].NCLEAF++;
    cells[ncell-1].NDLEAF++;
  }
}

void buildTree(const Body *bodies, Cell *cells, int &ncell) {
  int begin = 0, end = ncell;
  float diameter = 2 * R0 / (1 << MAXLEVEL);
  for( int level=MAXLEVEL-1; level>=0; level-- ) {
    int oldcell = -1;
    diameter *= 2;
    for( int i=begin; i!=end; ++i ) {
      int icell = cells[i].ICELL / 8;
      if( icell != oldcell ) {
        initCell(bodies,cells[ncell],icell,cells[i].LEAF,diameter);
        cells[ncell].CHILD = i;
        oldcell = icell;
        ncell++;
      }
      cells[ncell-1].NCHILD++;
      cells[ncell-1].NDLEAF += cells[i].NDLEAF;
    }
    begin = end;
    end = ncell;
  }
}

float getBmax(const float *X, const Cell &cell) {
  float dx = cell.R + fabs(X[0] - cell.X[0]);
  float dy = cell.R + fabs(X[1] - cell.X[1]);
  return sqrtf( dx*dx + dy*dy );
}

void setCenter(Body *bodies, Cell *cells, Cell &cell) {
  float M = 0;
  float X[2] = {0};
  for( int i=0; i<cell.NCLEAF; i++ ) {
    Body body = bodies[cell.LEAF+i];
    M += body.M;
    X[0] += body.X[0] * body.M;
    X[1] += body.X[1] * body.M;
  }
  for( int c=0; c<cell.NCHILD; c++ ) {
    Cell child = cells[cell.CHILD+c];
    M += fabs(child.M[0]);
    X[0] += child.X[0] * fabs(child.M[0]);
    X[1] += child.X[1] * fabs(child.M[0]);
  }
  X[0] /= M;
  X[1] /= M;
  cell.R = getBmax(X,cell);
  cell.X[0] = X[0];
  cell.X[1] = X[1];
}

inline void P2M(Cell &cell, const Body *bodies) {
  for( int i=0; i<cell.NCLEAF; i++ ) {
    Body leaf = bodies[cell.LEAF+i];
    float dx = cell.X[0] - leaf.X[0];
    float dy = cell.X[1] - leaf.X[1];
    cell.M[0] += leaf.M;
    cell.M[1] += leaf.M * dx * dx / 2;
    cell.M[2] += leaf.M * dx * dy / 2;
    cell.M[3] += leaf.M * dy * dy / 2;
  }
}

inline void M2M(Cell &parent, const Cell *cells) {
  for( int c=0; c<parent.NCHILD; c++ ) {
    Cell child = cells[parent.CHILD+c];
    float dx = parent.X[0] - child.X[0];
    float dy = parent.X[1] - child.X[1];
    parent.M[0] += child.M[0];
    parent.M[1] += child.M[1] + dx * dx * child.M[0] / 2;
    parent.M[2] += child.M[2];
    parent.M[3] += child.M[3] + dy * dy * child.M[0] / 2;
  }
}

inline void P2P(Body *bodies, const Cell &icell, const Cell &jcell) {
  Body *ileaf = &bodies[icell.LEAF];
  Body *jleaf = &bodies[jcell.LEAF];
  for( int i=0; i<icell.NDLEAF; i++ ) {
    for( int j=0; j<jcell.NDLEAF; j++ ) {
      float dx = ileaf[i].X[0] - jleaf[j].X[0];
      float dy = ileaf[i].X[1] - jleaf[j].X[1];
      float R = sqrtf(dx * dx + dy * dy + EPS2);
      ileaf[i].P += jleaf[j].M * logf(R);
    }
  }
}

inline void P2PSSE(Body *bodies, const Cell &icell, const Cell &jcell) {
  Body *ileaf = &bodies[icell.LEAF];
  Body *jleaf = &bodies[jcell.LEAF];

  __m128 BASE = _mm_set1_ps(M_E);
  BASE = log2f4(BASE);
  BASE = _mm_div_ps(_mm_set1_ps(1.0f),BASE);
  for( int i=0; i<icell.NDLEAF-4; i+=4 ) {
    __m128 PHI = _mm_setzero_ps();
    __m128 XI = _mm_setr_ps(ileaf[i].X[0],ileaf[i+1].X[0],ileaf[i+2].X[0],ileaf[i+3].X[0]);
    __m128 YI = _mm_setr_ps(ileaf[i].X[1],ileaf[i+1].X[1],ileaf[i+2].X[1],ileaf[i+3].X[1]);
    __m128 R2 = _mm_set1_ps(EPS2);

    __m128 X2 = _mm_set1_ps(jleaf[0].X[0]);
    X2 = _mm_sub_ps(XI, X2);
    __m128 Y2 = _mm_set1_ps(jleaf[0].X[1]);
    Y2 = _mm_sub_ps(YI, Y2);
    __m128 MJ = _mm_set1_ps(jleaf[0].M);

    __m128 XJ = X2;
    X2 = _mm_mul_ps(X2,X2);
    R2 = _mm_add_ps(R2,X2);
    __m128 YJ = Y2;
    Y2 = _mm_mul_ps(Y2,Y2);
    R2 = _mm_add_ps(R2,Y2);
    __m128 logR;

    X2 = _mm_set1_ps(jleaf[1].X[0]);
    Y2 = _mm_set1_ps(jleaf[1].X[1]);
    for( int j=0; j<jcell.NDLEAF-2; j++ ) {
      logR = _mm_rsqrt_ps(R2);
      R2 = _mm_set1_ps(EPS2);
      X2 = _mm_sub_ps(XI,X2);
      Y2 = _mm_sub_ps(YI,Y2);

      logR = log2f4(logR);
      logR = _mm_mul_ps(logR,BASE);
      MJ = _mm_mul_ps(MJ,logR);
      PHI = _mm_add_ps(PHI,MJ);
      MJ = _mm_set1_ps(jleaf[j+1].M);

      XJ = X2;
      X2 = _mm_mul_ps(X2,X2);
      R2 = _mm_add_ps(R2,X2);
      X2 = _mm_set1_ps(jleaf[j+2].X[0]);

      YJ = Y2;
      Y2 = _mm_mul_ps(Y2,Y2);
      R2 = _mm_add_ps(R2,Y2);
      Y2 = _mm_set1_ps(jleaf[j+2].X[1]);
    }
    logR = _mm_rsqrt_ps(R2);
    R2 = _mm_set1_ps(EPS2);
    X2 = _mm_sub_ps(XI,X2);
    Y2 = _mm_sub_ps(YI,Y2);

    logR = log2f4(logR);
    logR = _mm_mul_ps(logR,BASE);
    MJ = _mm_mul_ps(MJ,logR);
    PHI = _mm_add_ps(PHI,MJ);
    MJ = _mm_set1_ps(jleaf[jcell.NDLEAF-1].M);

    XJ = X2;
    X2 = _mm_mul_ps(X2,X2);
    R2 = _mm_add_ps(R2,X2);

    YJ = Y2;
    Y2 = _mm_mul_ps(Y2,Y2);
    R2 = _mm_add_ps(R2,Y2);

    logR = _mm_rsqrt_ps(R2);
    logR = log2f4(logR);
    logR = _mm_mul_ps(logR,BASE);
    MJ = _mm_mul_ps(MJ,logR);
    PHI = _mm_add_ps(PHI,MJ);
    for (int k=0; k<4; k++) {
      ileaf[i+k].P -= ((float*)&PHI)[k];
    }
  }
  for( int i=(icell.NDLEAF-1)/4*4; i<icell.NDLEAF; i++ ) {
    for( int j=0; j<jcell.NDLEAF; j++ ) {
      float dx = ileaf[i].X[0] - jleaf[j].X[0];
      float dy = ileaf[i].X[1] - jleaf[j].X[1];
      float R = sqrtf(dx * dx + dy * dy + EPS2);
      ileaf[i].P += jleaf[j].M * logf(R);
    }
  }
}

inline void M2P(Body *bodies, const Cell &icell, const Cell &jcell) {
  for( int i=0; i<icell.NDLEAF; i++ ) {
    Body *ileaf = &bodies[icell.LEAF];
    float dx = ileaf[i].X[0] - jcell.X[0];
    float dy = ileaf[i].X[1] - jcell.X[1];
    float R = sqrtf(dx * dx + dy * dy + EPS2);
    float invR = 1 / R;
    float invR2 = invR * invR;
    float invR4 = -2 * invR2 * invR2;
    ileaf[i].P += jcell.M[0] * log(R);
    ileaf[i].P += jcell.M[1] * (dx * dx * invR4 + invR2);
    ileaf[i].P += jcell.M[2] * dx * dy * invR4;
    ileaf[i].P += jcell.M[3] * (dy * dy * invR4 + invR2);
  }
}

inline void M2PSSE(Body *bodies, const Cell &icell, const Cell &jcell) {
  Body *ileaf = &bodies[icell.LEAF];
  Body *jleaf = &bodies[jcell.LEAF];

  __m128 BASE = _mm_set1_ps(M_E);
  BASE = log2f4(BASE);
  BASE = _mm_div_ps(_mm_set1_ps(1.0f),BASE);
  BASE = -BASE;
  __m128 XJ = _mm_set1_ps(jcell.X[0]);
  __m128 YJ = _mm_set1_ps(jcell.X[1]);
  __m128 S2 = _mm_set1_ps(-2);
  __m128 M0 = _mm_set1_ps(jcell.M[0]);
  __m128 M1 = _mm_set1_ps(jcell.M[1]);
  __m128 M2 = _mm_set1_ps(jcell.M[2]);
  __m128 M3 = _mm_set1_ps(jcell.M[3]);
  for( int i=0; i<icell.NDLEAF-4; i+=4 ) {
    __m128 PHI = _mm_setzero_ps();
    __m128 X2 = _mm_setr_ps(ileaf[i].X[0],ileaf[i+1].X[0],ileaf[i+2].X[0],ileaf[i+3].X[0]);
    X2 = _mm_sub_ps(X2, XJ);
    __m128 R2 = _mm_set1_ps(EPS2);
    __m128 XY = X2;
    X2 = _mm_mul_ps(X2,X2);
    R2 = _mm_add_ps(R2,X2);
    __m128 Y2 = _mm_setr_ps(ileaf[i].X[1],ileaf[i+1].X[1],ileaf[i+2].X[1],ileaf[i+3].X[1]);
    Y2 = _mm_sub_ps(Y2, YJ);
    XY = _mm_mul_ps(XY,Y2);
    Y2 = _mm_mul_ps(Y2,Y2);
    R2 = _mm_add_ps(R2,Y2);
    __m128 invR = _mm_rsqrt_ps(R2);
    __m128 logR = log2f4(invR);
    logR = _mm_mul_ps(logR,BASE);
    __m128 invR2 = _mm_mul_ps(invR,invR);
    logR = _mm_mul_ps(logR,M0);
    PHI = _mm_add_ps(PHI,logR);
    __m128 invR4 = _mm_mul_ps(invR2,invR2);
    invR4 = _mm_mul_ps(S2,invR4);
    X2 = _mm_mul_ps(X2,invR4);
    X2 = _mm_add_ps(X2,invR2);
    X2 = _mm_mul_ps(X2,M1);
    PHI = _mm_add_ps(PHI,X2);
    XY = _mm_mul_ps(XY,invR4);
    XY = _mm_mul_ps(XY,M2);
    PHI = _mm_add_ps(PHI,XY);
    Y2 = _mm_mul_ps(Y2,invR4);
    Y2 = _mm_add_ps(Y2,invR2);
    Y2 = _mm_mul_ps(Y2,M3);
    PHI = _mm_add_ps(PHI,Y2);
    for (int k=0; k<4; k++) {
      ileaf[i+k].P += ((float*)&PHI)[k];
    }
  }
  for( int i=(icell.NDLEAF-1)/4*4; i<icell.NDLEAF; i++ ) {
    float dx = ileaf[i].X[0] - jcell.X[0];
    float dy = ileaf[i].X[1] - jcell.X[1];
    float R = sqrtf(dx * dx + dy * dy + EPS2);
    float invR = 1 / R;
    float invR2 = invR * invR;
    float invR4 = -2 * invR2 * invR2;
    ileaf[i].P += jcell.M[0] * logf(R);
    ileaf[i].P += jcell.M[1] * (dx * dx * invR4 + invR2);
    ileaf[i].P += jcell.M[2] * dx * dy * invR4;
    ileaf[i].P += jcell.M[3] * (dy * dy * invR4 + invR2);
  }
}

int main() {
  Body *bodies = new Body [N];
  Body *bodies2 = new Body [N];
// Initialize
  std::cout << "N     : " << N << std::endl << std::endl;
  for( int i=0; i!=N; ++i ) {
    bodies[i].X[0] = drand48();
    bodies[i].X[1] = drand48();
    bodies[i].M = 1. / N;
    bodies[i].P = -bodies[i].M / sqrtf(EPS2);
  }
// Set root cell
  double tic = get_time();
// Build tree
  getIndex(bodies);
  Body *buffer = new Body [N];
  sortBody(bodies,buffer);
  delete[] buffer; 
  Cell *cells = new Cell [N];
  int ncell = 0;
  buildCell(bodies,cells,ncell);
  double toc = get_time();
  std::cout << "Index : " << toc-tic << std::endl;
  tic = get_time();
  int ntwig = ncell;
  buildTree(bodies,cells,ncell);
// Upward sweep
  for( int i=0; i<ncell-1; i++ ) {
    setCenter(bodies,cells,cells[i]);
    P2M(cells[i],bodies);
    M2M(cells[i],cells);
  }
  toc = get_time();
  std::cout << "Build : " << toc-tic << std::endl;
// Direct summation
  tic = get_time();
#if 1
  Cell root = cells[ncell-1];
  P2PSSE(bodies,root,root);
  for( int i=0; i<N; i++ ) {
    bodies2[i].P = bodies[i].P;
    bodies[i].P = -bodies[i].M / sqrtf(EPS2);
  }
#endif
  toc = get_time();
  std::cout << "Direct: " << toc-tic << std::endl;
// Evaluate
  tic = get_time();
  int NP2P = 0, NM2P = 0;
  int stack[20];
  int nstack = 0;
  for( int i=0; i<ntwig; i++ ) {
    Cell icell = cells[i];
    stack[nstack++] = ncell-1;
    while( nstack ) {
      Cell jparent = cells[stack[--nstack]];
      for( int j=0; j<jparent.NCHILD; j++ ) {
        Cell jcell = cells[jparent.CHILD+j];
        float dx = icell.X[0] - jcell.X[0];
        float dy = icell.X[1] - jcell.X[1];
        float R = sqrtf(dx * dx + dy * dy);
        if( jcell.R < THETA * R ) {
          M2PSSE(bodies,icell,jcell);
          NM2P++;
        } else if( jcell.NCHILD == 0 ) {
          P2PSSE(bodies,icell,jcell);
          NP2P++;
        } else {
          stack[nstack++] = jparent.CHILD+j;
        }
      }
    }
  }
  toc = get_time();
  std::cout << "FMM   : " << toc-tic << std::endl << std::endl;
  std::cout << "NP2P  : " << NP2P << std::endl;
  std::cout << "NM2P  : " << NM2P << std::endl << std::endl;
// Check accuracy
  float err = 0, rel = 0;
  for( int i=0; i<N; i++ ) {
    Body body = bodies[i];
    Body body2 = bodies2[i];
    err += (body2.P - body.P) * (body2.P - body.P);
    rel += body2.P * body2.P;
  }
  std::cout << "P err : " << sqrtf(err/rel) << std::endl;
  delete[] bodies;
  delete[] bodies2;
  delete[] cells;
}
