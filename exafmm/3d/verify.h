#ifndef verify_h
#define verify_h
#include "timer.h"
#include "namespace.h"
#include "types.h"

namespace EXAFMM_NAMESPACE {
  //! Verify results
  class Verify {
  public:
    bool verbose;                                               //!< Print to screen
    double average, average2;                                   //!< Average for regression

    //! Constructor
    Verify() : average(0), average2(0) {}

    //! Get sum of scalar component of a vector of target bodies
    double getSumScalar(Bodies & bodies) {
      double v = 0;                                             // Initialize difference
      for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {     // Loop over bodies
#if EXAFMM_LAPLACE
	v += B->TRG[0] * B->SRC;                                //  Sum of scalar component for Laplace
#elif EXAFMM_HELMHOLTZ
	v += std::abs(B->TRG[0] * B->SRC);                      //  Sum of scalar component for Helmholtz
#elif EXAFMM_BIOTSAVART
	v += B->TRG[0];                                         //  Sum of x component for Biot-Savart
#endif
      }                                                         // End loop over bodies
      return v;                                                 // Return difference
    }

    //! Get norm of scalar component of a vector of target bodies
    double getNrmScalar(Bodies & bodies) {
      double v = 0;                                             // Initialize norm
      for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {     // Loop over bodies
	v += std::abs(B->TRG[0] * B->TRG[0]);                   //  Norm of scalar component
      }                                                         // End loop over bodies
      return v;                                                 // Return norm
    }

    //! Get difference between scalar component of two vectors of target bodies
    double getDifScalar(Bodies & bodies, Bodies & bodies2) {
      double v = 0;                                             // Initialize difference
      B_iter B2 = bodies2.begin();                              // Set iterator of bodies2
      for (B_iter B=bodies.begin(); B!=bodies.end(); B++, B2++) { // Loop over bodies & bodies2
	v += std::abs((B->TRG[0] - B2->TRG[0]) * (B->TRG[0] - B2->TRG[0])); //  Difference of scalar component
      }                                                         // End loop over bodies & bodies2
      return v;                                                 // Return difference
    }

    //! Get difference between scalar component of two vectors of target bodies
    double getRelScalar(Bodies & bodies, Bodies & bodies2) {
      double v = 0;                                             // Initialize difference
      B_iter B2 = bodies2.begin();                              // Set iterator of bodies2
      for (B_iter B=bodies.begin(); B!=bodies.end(); B++, B2++) { // Loop over bodies & bodies2
	v += std::abs(((B->TRG[0] - B2->TRG[0]) * (B->TRG[0] - B2->TRG[0]))
		      / (B2->TRG[0] * B2->TRG[0]));             //  Difference of scalar component
      }                                                         // End loop over bodies & bodies2
      return v;                                                 // Return difference
    }

    //! Get norm of scalar component of a vector of target bodies
    double getNrmVector(Bodies & bodies) {
      double v = 0;                                             // Initialize norm
      for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {     // Loop over bodies
	v += std::abs(B->TRG[1] * B->TRG[1] +                   //  Norm of vector x component
		      B->TRG[2] * B->TRG[2] +                   //  Norm of vector y component
		      B->TRG[3] * B->TRG[3]);                   //  Norm of vector z component
      }                                                         // End loop over bodies
      return v;                                                 // Return norm
    }

    //! Get difference between scalar component of two vectors of target bodies
    double getDifVector(Bodies & bodies, Bodies & bodies2) {
      double v = 0;                                             // Initialize difference
      B_iter B2 = bodies2.begin();                              // Set iterator of bodies2
      for (B_iter B=bodies.begin(); B!=bodies.end(); B++, B2++) { // Loop over bodies & bodies2
	v += std::abs((B->TRG[1] - B2->TRG[1]) * (B->TRG[1] - B2->TRG[1]) + //  Difference of vector x component
		      (B->TRG[2] - B2->TRG[2]) * (B->TRG[2] - B2->TRG[2]) + //  Difference of vector y component
		      (B->TRG[3] - B2->TRG[3]) * (B->TRG[3] - B2->TRG[3])); //  Difference of vector z component
      }                                                         // End loop over bodies & bodies2
      return v;                                                 // Return difference
    }

    //! Get difference between scalar component of two vectors of target bodies
    double getRelVector(Bodies & bodies, Bodies & bodies2) {
      double v = 0;                                             // Initialize difference
      B_iter B2 = bodies2.begin();                              // Set iterator of bodies2
      for (B_iter B=bodies.begin(); B!=bodies.end(); B++, B2++) { // Loop over bodies & bodies2
	v += std::abs(((B->TRG[1] - B2->TRG[1]) * (B->TRG[1] - B2->TRG[1]) +//  Difference of vector x component
		       (B->TRG[2] - B2->TRG[2]) * (B->TRG[2] - B2->TRG[2]) +//  Difference of vector y component
		       (B->TRG[3] - B2->TRG[3]) * (B->TRG[3] - B2->TRG[3]))//   Difference of vector z component
		      / (B2->TRG[1] * B2->TRG[1] +              //  Norm of vector x component
			 B2->TRG[2] * B2->TRG[2] +              //  Norm of vector y component
			 B2->TRG[3] * B2->TRG[3]));             //  Norm of vector z component
      }                                                         // End loop over bodies & bodies2
      return v;                                                 // Return difference
    }

    //! Print relative L2 norm scalar error
    void print(std::string title, double v) {
      std::cout << std::setw(timer::stringLength) << std::left  //  Set format
                << title << " : " << std::setprecision(timer::decimal) << std::scientific // Set title
                << v << std::endl;                              //  Print potential error
    }
  };
}
#endif