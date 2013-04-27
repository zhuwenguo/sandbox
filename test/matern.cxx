#include <boost/math/special_functions/bessel.hpp>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <iostream>
#include <sys/time.h>
#include "vec.h"
using boost::math::binomial_coefficient;
using boost::math::cyl_bessel_k;
using boost::math::tgamma;

const int P = 6;
const double NU = 1.5;
const int LTERM = (P+1)*(P+2)*(P+3)/6;

typedef double real_t;
typedef vec<3,real_t> vec3;
typedef vec<LTERM,real_t> vecL;

template<typename T, int nx, int ny, int nz>
struct Index {
  static const int                I = Index<T,nx,ny+1,nz-1>::I + 1;
  static const unsigned long long F = Index<T,nx,ny,nz-1>::F * nz;
};

template<typename T, int nx, int ny>
struct Index<T,nx,ny,0> {
  static const int                I = Index<T,nx+1,0,ny-1>::I + 1;
  static const unsigned long long F = Index<T,nx,ny-1,0>::F * ny;
};

template<typename T, int nx>
struct Index<T,nx,0,0> {
  static const int                I = Index<T,0,0,nx-1>::I + 1;
  static const unsigned long long F = Index<T,nx-1,0,0>::F * nx;
};

template<typename T>
struct Index<T,0,0,0> {
  static const int                I = 0;
  static const unsigned long long F = 1;
};


template<int kx, int ky , int kz, int d>
struct DerivativeTerm {
  static inline real_t kernel(const vecL &C, const vec3 &dX) {
    return dX[d] * C[Index<vecL,kx,ky,kz>::I];
  }
};

template<int kx, int ky , int kz>
struct DerivativeTerm<kx,ky,kz,-1> {
  static inline real_t kernel(const vecL &C, const vec3&) {
    return -C[Index<vecL,kx,ky,kz>::I];
  }
};


template<int nx, int ny, int nz, int kx=nx, int ky=ny, int kz=nz, int flag=5>
struct DerivativeSum {
  static const int nextflag = 5 - (kz < nz || kz == 1);
  static const int dim = kz == (nz-1) ? -1 : 2;
  static const int n = nx + ny + nz;
  static inline real_t loop(const vecL &C, const vec3 &dX) {
    return DerivativeSum<nx,ny,nz,nx,ny,kz-1,nextflag>::loop(C,dX)
      + DerivativeTerm<nx,ny,kz-1,dim>::kernel(C,dX);
  }
};

template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,4> {
  static const int nextflag = 3 - (ny == 0);
  static inline real_t loop(const vecL &C, const vec3 &dX) {
    return DerivativeSum<nx,ny,nz,nx,ny,nz,nextflag>::loop(C,dX);
  }
};

template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,3> {
  static const int nextflag = 3 - (ky < ny || ky == 1);
  static const int dim = ky == (ny-1) ? -1 : 1;
  static const int n = nx + ny + nz;
  static inline real_t loop(const vecL &C, const vec3 &dX) {
    return DerivativeSum<nx,ny,nz,nx,ky-1,nz,nextflag>::loop(C,dX)
      + DerivativeTerm<nx,ky-1,nz,dim>::kernel(C,dX);
  }
};

template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,2> {
  static const int nextflag = 1 - (nx == 0);
  static inline real_t loop(const vecL &C, const vec3 &dX) {
    return DerivativeSum<nx,ny,nz,nx,ny,nz,nextflag>::loop(C,dX);
  }
};

template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,1> {
  static const int nextflag = 1 - (kx < nx || kx == 1);
  static const int dim = kx == (nx-1) ? -1 : 0;
  static const int n = nx + ny + nz;
  static inline real_t loop(const vecL &C, const vec3 &dX) {
    return DerivativeSum<nx,ny,nz,kx-1,ny,nz,nextflag>::loop(C,dX)
      + DerivativeTerm<kx-1,ny,nz,dim>::kernel(C,dX);
  }
};

template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,0> {
  static inline real_t loop(const vecL&, const vec3&) {
    return 0;
  }
};

template<int nx, int ny, int nz, int kx, int ky>
struct DerivativeSum<nx,ny,nz,kx,ky,0,5> {
  static inline real_t loop(const vecL &C, const vec3 &dX) {
    return DerivativeSum<nx,ny,nz,nx,ny,0,4>::loop(C,dX);
  }
};


template<int nx, int ny, int nz>
struct Kernels {
  static inline void power(vecL &C, const vec3 &dX) {
    Kernels<nx,ny+1,nz-1>::power(C,dX);
    C[Index<vecL,nx,ny,nz>::I] = C[Index<vecL,nx,ny,nz-1>::I] * dX[2] / nz;
  }
  static inline void scale(vecL &C) {
    Kernels<nx,ny+1,nz-1>::scale(C);
    C[Index<vecL,nx,ny,nz>::I] *= Index<vecL,nx,ny,nz>::F;
  }
  /*
  static inline void M2M(vecM &MI, const vecL &C, const vecM &MJ) {
    Kernels<nx,ny+1,nz-1>::M2M(MI,C,MJ);
    MI[Index<vecM,nx,ny,nz>::I] += MultipoleSum<nx,ny,nz>::kernel(C,MJ);
  }
  static inline void M2L(vecL &L, const vecL &C, const vecM &M) {
    Kernels<nx,ny+1,nz-1>::M2L(L,C,M);
    L[Index<vecL,nx,ny,nz>::I] += LocalSum<nx,ny,nz,vecM>::kernel(M,C);
  }
  static inline void L2L(vecL &LI, const vecL &C, const vecL &LJ) {
    Kernels<nx,ny+1,nz-1>::L2L(LI,C,LJ);
    LI[Index<vecL,nx,ny,nz>::I] += LocalSum<nx,ny,nz,vecL>::kernel(C,LJ);
  }
  static inline void L2P(B_iter B, const vecL &C, const vecL &L) {
    Kernels<nx,ny+1,nz-1>::L2P(B,C,L);
    B->TRG[Index<vecL,nx,ny,nz>::I] += LocalSum<nx,ny,nz,vecL>::kernel(C,L);
  }
  */
};

template<int nx, int ny>
struct Kernels<nx,ny,0> {
  static inline void power(vecL &C, const vec3 &dX) {
    Kernels<nx+1,0,ny-1>::power(C,dX);
    C[Index<vecL,nx,ny,0>::I] = C[Index<vecL,nx,ny-1,0>::I] * dX[1] / ny;
  }
  static inline void scale(vecL &C) {
    Kernels<nx+1,0,ny-1>::scale(C);
    C[Index<vecL,nx,ny,0>::I] *= Index<vecL,nx,ny,0>::F;
  }
  /*
  static inline void M2M(vecM &MI, const vecL &C, const vecM &MJ) {
    Kernels<nx+1,0,ny-1>::M2M(MI,C,MJ);
    MI[Index<vecM,nx,ny,0>::I] += MultipoleSum<nx,ny,0>::kernel(C,MJ);
  }
  static inline void M2L(vecL &L, const vecL &C, const vecM &M) {
    Kernels<nx+1,0,ny-1>::M2L(L,C,M);
    L[Index<vecL,nx,ny,0>::I] += LocalSum<nx,ny,0,vecM>::kernel(M,C);
  }
  static inline void L2L(vecL &LI, const vecL &C, const vecL &LJ) {
    Kernels<nx+1,0,ny-1>::L2L(LI,C,LJ);
    LI[Index<vecL,nx,ny,0>::I] += LocalSum<nx,ny,0,vecL>::kernel(C,LJ);
  }
  static inline void L2P(B_iter B, const vecL &C, const vecL &L) {
    Kernels<nx+1,0,ny-1>::L2P(B,C,L);
    B->TRG[Index<vecL,nx,ny,0>::I] += LocalSum<nx,ny,0,vecL>::kernel(C,L);
  }
  */
};

template<int nx>
struct Kernels<nx,0,0> {
  static inline void power(vecL &C, const vec3 &dX) {
    Kernels<0,0,nx-1>::power(C,dX);
    C[Index<vecL,nx,0,0>::I] = C[Index<vecL,nx-1,0,0>::I] * dX[0] / nx;
  }
  static inline void scale(vecL &C) {
    Kernels<0,0,nx-1>::scale(C);
    C[Index<vecL,nx,0,0>::I] *= Index<vecL,nx,0,0>::F;
  }
  /*
  static inline void M2M(vecM &MI, const vecL &C, const vecM &MJ) {
    Kernels<0,0,nx-1>::M2M(MI,C,MJ);
    MI[Index<vecM,nx,0,0>::I] += MultipoleSum<nx,0,0>::kernel(C,MJ);
  }
  static inline void M2L(vecL &L, const vecL &C, const vecM &M) {
    Kernels<0,0,nx-1>::M2L(L,C,M);
    L[Index<vecL,nx,0,0>::I] += LocalSum<nx,0,0,vecM>::kernel(M,C);
  }
  static inline void L2L(vecL &LI, const vecL &C, const vecL &LJ) {
    Kernels<0,0,nx-1>::L2L(LI,C,LJ);
    LI[Index<vecL,nx,0,0>::I] += LocalSum<nx,0,0,vecL>::kernel(C,LJ);
  }
  static inline void L2P(B_iter B, const vecL &C, const vecL &L) {
    Kernels<0,0,nx-1>::L2P(B,C,L);
    B->TRG[Index<vecL,nx,0,0>::I] += LocalSum<nx,0,0,vecL>::kernel(C,L);
  }
  */
};

template<>
struct Kernels<0,0,0> {
  static inline void power(vecL&, const vec3&) {}
  static inline void scale(vecL&) {}
  /*
  static inline void M2M(vecM&, const vecL&, const vecM&) {}
  static inline void M2L(vecL&, const vecL&, const vecM&) {}
  static inline void L2L(vecL&, const vecL&, const vecL&) {}
  static inline void L2P(B_iter, const vecL&, const vecL&) {}
  */
};

template<int np, int nx, int ny, int nz>
struct Kernels2 {
  static inline void derivative(vecL &C, vecL &G, const vec3 &dX, real_t &coef) {
    static const int n = nx + ny + nz;
    Kernels2<np,nx,ny+1,nz-1>::derivative(C,G,dX,coef);
    C[Index<vecL,nx,ny,nz>::I] = DerivativeSum<nx,ny,nz>::loop(G,dX) / n * coef;
  }
};

template<int np, int nx, int ny>
struct Kernels2<np,nx,ny,0> {
  static inline void derivative(vecL &C, vecL &G, const vec3 &dX, real_t &coef) {
    static const int n = nx + ny;
    Kernels2<np,nx+1,0,ny-1>::derivative(C,G,dX,coef);
    C[Index<vecL,nx,ny,0>::I] = DerivativeSum<nx,ny,0>::loop(G,dX) / n * coef;
  }
};

template<int np, int nx>
struct Kernels2<np,nx,0,0> {
  static inline void derivative(vecL &C, vecL &G, const vec3 &dX, real_t &coef) {
    static const int n = nx;
    Kernels2<np,0,0,nx-1>::derivative(C,G,dX,coef);
    C[Index<vecL,nx,0,0>::I] = DerivativeSum<nx,0,0>::loop(G,dX) / n * coef;
  }
};

template<int np>
struct Kernels2<np,0,0,0> {
  static inline void derivative(vecL &C, vecL &G, const vec3 &dX, real_t &coef) {
    Kernels2<np-1,0,0,np-1>::derivative(G,C,dX,coef);
    static const double c = std::sqrt(2 * NU);
    double R = c * std::sqrt(norm(dX));
    double zR = (-0.577216-log(R/2)) * (R<0.413) + 1 * (R>=0.413);
    static const double u = NU - P + np;
    static const double gu = tgamma(1-u) / tgamma(u);
    static const double aum = std::abs(u-1);
    static const double gaum = 1 / tgamma(aum);
    static const double au = std::abs(u);
    static const double gau = 1 / tgamma(au);
    if (aum < 1e-12) {
      G[0] = cyl_bessel_k(0,R) / zR;
    } else {
      G[0] = std::pow(R/2,aum) * 2 * cyl_bessel_k(aum,R) * gaum;
    }
    if (au < 1e-12) {
      C[0] = cyl_bessel_k(0,R) / zR;
    } else {
      C[0] = std::pow(R/2,au) * 2 * cyl_bessel_k(au,R) * gau;
    }
    double hu = 0;
    if (u > 1) {
      hu = 0.5 / (u-1);
    } else if (NU == 0) {
      hu = zR;
    } else if (u > 0 && u < 1) {
      hu = std::pow(R/2,2*u-2) / 2 * gu;
    } else if (u == 0) {
      hu = 1 / (R * R * zR);
    } else {
      hu = -2 * u / (R * R);
    }
    coef = c * c * hu;
  }
};

template<>
struct Kernels2<0,0,0,0> {
  static inline void derivative(vecL&, vecL&, const vec3&, const real_t&) {}
};


template<int PP>
inline void getCoef(vecL &C, const vec3 &dX) {
  double coef;
  vecL G;
  Kernels2<PP,0,0,PP>::derivative(C,G,dX,coef);
  Kernels<0,0,PP>::scale(C);
}

typedef vec<3,double> vec3;
vec3 make_vec3(double a, double b, double c) {
  vec3 v;
  v[0] = a;
  v[1] = b;
  v[2] = c;
  return v;
}

double get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return double(tv.tv_sec+tv.tv_usec*1e-6);
}

void matern(int ni, int nj, vec3 * XiL, vec3 XLM, vec3 * XjM, double * f) {
  double temp = powf(2,NU-1) * tgamma(NU);
  for (int i=0; i<ni; i++) {
    f[i] = 0;
    for (int j=0; j<nj; j++) {
      vec3 dX = (XiL[i] + XLM - XjM[j]);
      double R = sqrt(norm(dX) * 2 * NU);
      f[i] += powf(R,NU) * cyl_bessel_k(NU,R) / temp;
      if (R < 1e-12) f[i] += 1;
    }
  }
}

vecL P2M(double * factorial, vec3 XjM) {
  vecL M = 0;
  for (int sumi=0,ic=0; sumi<P; sumi++) {
    for (int ix=sumi; ix>=0; ix--) {
      for (int iz=0; iz<=sumi-ix; iz++,ic++) {
        int iy = sumi - ix - iz;
        M[ic] += powf(XjM[0],ix) / factorial[ix]
	  * powf(XjM[1],iy) / factorial[iy]
          * powf(XjM[2],iz) / factorial[iz];
      }
    }
  }
  return M;
}

vecL M2L(int *** I, vec3 XLM, vecL M) {
  vecL C, L=0;
  getCoef<P>(C,XLM);
  for (int sumi=0; sumi<P; sumi++) {
    for (int ix=sumi; ix>=0; ix--) {
      for (int iz=0; iz<=sumi-ix; iz++) {
        int iy = sumi - ix - iz;
        real_t Ld = 0;
        for (int sumj=0; sumj<P-sumi; sumj++) {
          for (int jx=sumj; jx>=0; jx--) {
            for (int jz=0; jz<=sumj-jx; jz++) {
              int jy = sumj - jx - jz;
              Ld += C[I[jx+ix][jy+iy][jz+iz]] * M[I[jx][jy][jz]];
	    }
	  }
	}
        L[I[ix][iy][iz]] = Ld;
      }
    }
  }
  return L;
}

double L2P(double * factorial, int *** I, vec3 XiL, vecL L) {
  double f = 0;
  for (int sumi=0; sumi<P; sumi++) {
    for (int ix=sumi; ix>=0; ix--) {
      for (int iz=0; iz<=sumi-ix; iz++) {
	int iy = sumi - ix - iz;
        f += 
	  1 / factorial[ix] * powf(-XiL[0],ix) *
	  1 / factorial[iy] * powf(-XiL[1],iy) *
	  1 / factorial[iz] * powf(-XiL[2],iz) * L[I[ix][iy][iz]];
      }
    }
  }
  return f;
}

int main() {
  const vec3 XLM = make_vec3(0.7,0.3,0.4);
  const double ri = 0.2;
  const double rj = 0.4;
  const int ni = 30;
  const int nj = 30;
  vec3 * XiL = new vec3 [ni];
  vec3 * XjM = new vec3 [nj];
  double * f = new double [ni];
  double * factorial = new double [2*P+2];
  int *** I = new int ** [2*P+2];
  for (int i=0; i<2*P+2; i++) {
    I[i] = new int * [2*P+2];
    for (int j=0; j<2*P+2; j++) {
      I[i][j] = new int [2*P+2];
    }
  }

  factorial[0] = 1;
  for (int i=1; i<2*P+2; i++) {
    factorial[i] = i * factorial[i-1];
  }

  for (int sumi=0,ic=0; sumi<2*P+2; sumi++) {
    for (int ix=sumi; ix>=0; ix--) {
      for (int iz=0; iz<=sumi-ix; iz++,ic++) {
        int iy = sumi - ix - iz;
        I[ix][iy][iz] = ic;
      }
    }
  }

  double RLM = sqrt(XLM[0]*XLM[0]+XLM[1]*XLM[1]+XLM[2]*XLM[2]);
  for (int i=0; i<ni; i++) {
    XiL[i][0] = (i*2*ri/(ni-1) - ri) * XLM[0] / RLM;
    XiL[i][1] = (i*2*ri/(ni-1) - ri) * XLM[1] / RLM;
    XiL[i][2] = (i*2*ri/(ni-1) - ri) * XLM[2] / RLM;
  }
  for (int i=0; i<nj; i++) {
    XjM[i][0] = (i*2*rj/(nj-1) - rj) * XLM[0] / RLM;
    XjM[i][1] = (i*2*rj/(nj-1) - rj) * XLM[1] / RLM;
    XjM[i][2] = (i*2*rj/(nj-1) - rj) * XLM[2] / RLM;
  }

  matern(ni,nj,XiL,XLM,XjM,f);

  vecL M = 0;
  for (int j=0; j<nj; j++) {
    M += P2M(factorial,XjM[j]);
  }

  vecL L = M2L(I,XLM,M);

  double dif = 0, val = 0;
  for (int i=0; i<ni; i++) {
    double f2 = L2P(factorial, I, XiL[i], L);
    dif += (f[i] - f2) * (f[i] - f2);
    val += f[i] * f[i];
  }
  std::cout << sqrt(dif/val) << std::endl;

  for (int i=0; i<2*P+2; i++) {
    for (int j=0; j<2*P+2; j++) {
      delete[] I[i][j];
    }
    delete[] I[i];
  }
  delete[] I;
  delete[] factorial;
  delete[] f;
  delete[] XjM;
  delete[] XiL;
}
