#ifndef types_h
#define types_h
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace exafmm {
  // Basic type definitions
  typedef float real_t;                                         //!< Floating point type is single precision
  typedef std::complex<real_t> complex_t;                       //!< Complex type

  //! Structure of bodies
  struct Body {
    real_t X[2];                                                //!< Position
    real_t q;                                                   //!< Charge
    real_t p;                                                   //!< Potential
    real_t F[2];                                                //!< Force
  };

  //! Structure of cells
  struct Cell {
    int NCHILD;                                                 //!< Number of child cells
    int NBODY;                                                  //!< Number of descendant bodies
    Cell * CHILD[4];                                            //!< Pointer of child cells
    Body * BODY;                                                //!< Pointer of first body
    real_t X[2];                                                //!< Cell center
    real_t R;                                                   //!< Cell radius
    std::vector<complex_t> M;                                   //!< Multipole expansion coefficients
    std::vector<complex_t> L;                                   //!< Local expansion coefficients
  };
}

#endif
