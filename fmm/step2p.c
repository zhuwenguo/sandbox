// Step 2. Near-far decomposition

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>

double get_time() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return (double)(tv.tv_sec+tv.tv_usec*1e-6);
}

int main() {
  int i, j, N = 10000;
  double x[N], y[N], u[N], q[N];
  for (i=0; i<N; i++) {
    x[i] = drand48();
    y[i] = drand48();
    u[i] = 0;
    q[i] = 1;
  }
  int ix, iy;
  double M[4][4], L[4][4];
  for (ix=0; ix<4; ix++) {
    for (iy=0; iy<4; iy++) {
      M[ix][iy] = 0;
      L[ix][iy] = 0;
    }
  } 
  double tic = get_time();
  // P2M
  for (i=0; i<N; i++) {
    ix = x[i] * 4;
    iy = y[i] * 4;
    M[ix][iy] += q[i];
  }
  double toc = get_time();
  printf("%f\n",toc-tic);
  // M2L
  int jx, jy;
  for (ix=0; ix<4; ix++) {
    for (iy=0; iy<4; iy++) {
      for (jx=0; jx<4; jx++) {
	for (jy=0; jy<4; jy++) {
	  if (abs(ix-jx) > 1 || abs(iy-jy) > 1) {
	    double dx = (ix - jx) / 4.;
	    double dy = (iy - jy) / 4.;
	    double r = sqrt(dx * dx + dy * dy);
	    L[ix][iy] += M[jx][jy] / r;
	  }
	}
      }
    }
  }
  tic = get_time();
  printf("%f\n",tic-toc);
  // L2P
#pragma omp parallel for private(ix,iy)
  for (i=0; i<N; i++) {
    ix = x[i] * 4;
    iy = y[i] * 4;
    u[i] += L[ix][iy];
  }
  toc = get_time();
  printf("%f\n",toc-tic);
  // P2P
#pragma omp parallel for private(j,ix,iy,jx,jy)
  for (i=0; i<N; i++) {
    ix = x[i] * 4;
    iy = y[i] * 4;
    for (j=0; j<N; j++) {
      jx = x[j] * 4;
      jy = y[j] * 4;
      if (abs(ix-jx) <= 1 && abs(iy-jy) <= 1) {
	double dx = x[i] - x[j];
	double dy = y[i] - y[j];
	double r = sqrt(dx *dx + dy * dy);
	if (r!=0) u[i] += q[j] / r;
      }
    }
  }
  toc = get_time();
  printf("%f\n",toc-tic);
  // Check answer
#pragma omp parallel for private(j)
  for (i=0; i<N; i++) {
    double ui = 0;
    for (j=0; j<N; j++) {
      double dx = x[i] - x[j];
      double dy = y[i] - y[j];
      double r = sqrt(dx * dx + dy * dy);
      if (r != 0) ui += q[j] / r;
    }
    printf("%d %lf %lf\n", i, u[i], ui);
  }  
  tic = get_time();
  printf("%f\n",tic-toc);
}
