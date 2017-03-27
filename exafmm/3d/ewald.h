#ifndef ewald_h
#define ewald_h
#include "types.h"

namespace exafmm {
  //! Wave structure for Ewald summation
  struct Wave {
    real_t K[3];                                                //!< 3-D wave number vector
    real_t REAL;                                                //!< real part of wave
    real_t IMAG;                                                //!< imaginary part of wave
  };
  typedef std::vector<Wave> Waves;                              //!< Vector of Wave types
  typedef typename Waves::iterator W_iter;                      //!< Iterator of Wave types

  int ksize;                                                    //!< Number of waves in Ewald summation
  real_t alpha;                                                 //!< Scaling parameter for Ewald summation
  real_t sigma;                                                 //!< Scaling parameter for Ewald summation
  real_t cutoff;                                                //!< Cutoff distance
  real_t cycle;                                                 //!< Periodic cycle
  real_t K[3];                                                  //!< Wave number vector
  real_t dX[3];                                                 //!< Distance vector
  real_t scale[3];                                              //!< Scale vector

  //! Forward DFT
  void dft(Waves & waves, Bodies & bodies) {
    for (int d=0; d<3; d++) scale[d]= 2 * M_PI / cycle;         // Scale conversion
#pragma omp parallel for
    for (int w=0; w<int(waves.size()); w++) {                   // Loop over waves
      waves[w].REAL = waves[w].IMAG = 0;                        //  Initialize waves
      for (int b=0; b<int(bodies.size()); b++) {                //  Loop over bodies
        real_t th = 0;                                          //   Initialize phase
        for (int d=0; d<3; d++) th += waves[w].K[d] * bodies[b].X[d] * scale[d];//  Determine phase
        waves[w].REAL += bodies[b].SRC * std::cos(th);          //   Accumulate real component
        waves[w].IMAG += bodies[b].SRC * std::sin(th);          //   Accumulate imaginary component
      }                                                         //  End loop over bodies
    }                                                           // End loop over waves
  }

  //! Inverse DFT
  void idft(Waves & waves, Bodies & bodies) {
    for (int d=0; d<3; d++) scale[d] = 2 * M_PI / cycle;        // Scale conversion
#pragma omp parallel for
    for (int b=0; b<int(bodies.size()); b++) {                  // Loop over bodies
      real_t TRG[4];                                            //  Temporary target values
      for (int d=0; d<4; d++) TRG[d] = 0;                       //  Initialize target values
      for (int w=0; w<int(waves.size()); w++) {                 //   Loop over waves
        real_t th = 0;                                          //    Initialzie phase
        for (int d=0; d<3; d++) th += waves[w].K[d] * bodies[b].X[d] * scale[d];// Determine phase
        real_t dtmp = waves[w].REAL * std::sin(th) - waves[w].IMAG * std::cos(th);// Temporary value
        TRG[0]     += waves[w].REAL * std::cos(th) + waves[w].IMAG * std::sin(th);// Accumulate potential
        for (int d=0; d<3; d++) TRG[d+1] -= dtmp * waves[w].K[d];//   Accumulate force
      }                                                         //   End loop over waves
      for (int d=0; d<3; d++) TRG[d+1] *= scale[d];             //   Scale forces
      for (int d=0; d<4; d++) bodies[b].TRG[d] += TRG[d];       //  Copy results to bodies
    }                                                           // End loop over bodies
  }

  //! Initialize wave vector
  Waves initWaves() {
    Waves waves;                                                // Initialzie wave vector
    int kmaxsq = ksize * ksize;                                 // kmax squared
    int kmax = ksize;                                           // kmax as integer
    for (int l=0; l<=kmax; l++) {                               // Loop over x component
      int mmin = -kmax;                                         //  Determine minimum y component
      if (l==0) mmin = 0;                                       //  Exception for minimum y component
      for (int m=mmin; m<=kmax; m++) {                          //  Loop over y component
        int nmin = -kmax;                                       //   Determine minimum z component
        if (l==0 && m==0) nmin=1;                               //   Exception for minimum z component
        for (int n=nmin; n<=kmax; n++) {                        //   Loop over z component
          real_t ksq = l * l + m * m + n * n;                   //    Wave number squared
          if (ksq <= kmaxsq) {                                  //    If wave number is below kmax
            Wave wave;                                          //     Initialzie wave structure
            wave.K[0] = l;                                      //     x component of k
            wave.K[1] = m;                                      //     y component of k
            wave.K[2] = n;                                      //     z component of k
            wave.REAL = wave.IMAG = 0;                          //     Initialize amplitude
            waves.push_back(wave);                              //     Push wave to vector
          }                                                     //    End if for wave number
        }                                                       //   End loop over z component
      }                                                         //  End loop over y component
    }                                                           // End loop over x component
    return waves;                                               // Return wave vector
  }

  //! Ewald real part P2P kernel
  void realPart(C_iter Ci, C_iter Cj) {
    for (Body * Bi=Ci->BODY; Bi!=Ci->BODY+Ci->NBODY; Bi++) {    // Loop over target bodies
      for (Body * Bj=Cj->BODY; Bj!=Cj->BODY+Cj->NBODY; Bj++) {  //  Loop over source bodies
        for (int d=0; d<3; d++) dX[d] = Bi->X[d] - Bj->X[d] - Xperiodic[d];//   Distance vector from source to target
        real_t R2 = dX[0] * dX[0] + dX[1] * dX[1] + dX[2] * dX[2];//   R^2
        if (0 < R2 && R2 < cutoff * cutoff) {                   //   Exclude self interaction and cutoff
          real_t R2s = R2 * alpha * alpha;                      //    (R * alpha)^2
          real_t Rs = std::sqrt(R2s);                           //    R * alpha
          real_t invRs = 1 / Rs;                                //    1 / (R * alpha)
          real_t invR2s = invRs * invRs;                        //    1 / (R * alpha)^2
          real_t invR3s = invR2s * invRs;                       //    1 / (R * alpha)^3
          real_t dtmp = Bj->SRC * (M_2_SQRTPI * std::exp(-R2s) * invR2s + erfc(Rs) * invR3s);
          dtmp *= alpha * alpha * alpha;                        //    Scale temporary value
          Bi->TRG[0] += Bj->SRC * erfc(Rs) * invRs * alpha;     //    Ewald real potential
          Bi->TRG[1] -= dX[0] * dtmp;                           //    x component of Ewald real force
          Bi->TRG[2] -= dX[1] * dtmp;                           //    y component of Ewald real force
          Bi->TRG[3] -= dX[2] * dtmp;                           //    z component of Ewald real force
        }                                                       //   End if for self interaction
      }                                                         //  End loop over source bodies
    }                                                           // End loop over target bodies
  }

  void neighbor(C_iter Ci, C_iter Cj) {                         // Traverse tree to find neighbor
    for (int d=0; d<3; d++) dX[d] = Ci->X[d] - Cj->X[d];        //  Distance vector from source to target
    for (int d=0; d<3; d++) {                                   //  Loop over dimensions
      if(dX[d] < -cycle / 2) dX[d] += cycle;                    //   Wrap around positive
      if(dX[d] >  cycle / 2) dX[d] -= cycle;                    //   Wrap around negative
    }                                                           //  End loop over dimensions
    for (int d=0; d<3; d++) Xperiodic[d] = Ci->X[d] - Cj->X[d] - dX[d];//  Coordinate offset for periodic B.C.
    real_t R = std::sqrt(dX[0] * dX[0] + dX[1] * dX[1] + dX[2] * dX[2]);//  Scalar distance
    if (R - Ci->R - Cj->R < sqrtf(3) * cutoff) {                //  If cells are close
      if(Cj->NCHILD == 0) realPart(Ci, Cj);                     //   Ewald real part
      for (C_iter cj=Cj->CHILD; cj!=Cj->CHILD+Cj->NCHILD; cj++) {// Loop over cell's children
        neighbor(Ci, cj);                                       //    Instantiate recursive functor
      }                                                         //   End loop over cell's children
    }                                                           //  End if for far cells
  }                                                             // End overload operator()

  //! Ewald real part
  void realPart(Cells & cells, Cells & jcells) {
    C_iter Cj = jcells.begin();                                 // Set begin iterator of source cells
    for (C_iter Ci=cells.begin(); Ci!=cells.end(); Ci++) {      // Loop over target cells
      if (Ci->NCHILD == 0) neighbor(Ci, Cj);                    //  If target cell is leaf, find neighbors
    }                                                           // End loop over target cells
  }

  //! Ewald wave part
  void wavePart(Bodies & bodies, Bodies & jbodies) {
    Waves waves = initWaves();                                  // Initialize wave vector
    dft(waves,jbodies);                                         // Apply DFT to bodies to get waves
    for (int d=0; d<3; d++) scale[d] = 2 * M_PI / cycle;        // Scale conversion
    real_t coef = 2 / sigma / cycle / cycle / cycle;            // First constant
    real_t coef2 = 1 / (4 * alpha * alpha);                     // Second constant
    for (int w=0; w<int(waves.size()); w++) {                   // Loop over waves
      for (int d=0; d<3; d++) K[d] = waves[w].K[d] * scale[d];  //  Wave number scaled
      real_t K2 = K[0] * K[0] + K[1] * K[1] + K[2] * K[2];      //  Wave number squared
      real_t factor = coef * std::exp(-K2 * coef2) / K2;        //  Wave factor
      waves[w].REAL *= factor;                                  //  Apply wave factor to real part
      waves[w].IMAG *= factor;                                  //  Apply wave factor to imaginary part
    }                                                           // End loop over waves
    idft(waves,bodies);                                         // Inverse DFT
  }

  //! Subtract self term
  void selfTerm(Bodies & bodies) {
    for (int b=0; b<int(bodies.size()); b++) {                  // Loop over all bodies
      bodies[b].TRG[0] -= M_2_SQRTPI * bodies[b].SRC * alpha;   //  Self term of Ewald real part
    }                                                           // End loop over all bodies in cell
  }
}
#endif
