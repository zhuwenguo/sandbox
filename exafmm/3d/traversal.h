#ifndef traversal_h
#define traversal_h
#include "types.h"

namespace exafmm {
  int images;                                                   //!< Number of periodic image sublevels
  real_t theta;                                                 //!< Multipole acceptance criteria

  //! Post-order traversal for upward pass
  void upwardPass(C_iter Ci) {
    for (C_iter Cj=Ci->CHILD; Cj!=Ci->CHILD+Ci->NCHILD; Cj++) { // Loop over child cells
      upwardPass(Cj);                                           //  Recursive call for child cell
    }                                                           // End loop over child cells
    Ci->M.resize(NTERM, 0.0);                                   // Allocate and initialize multipole coefs
    Ci->L.resize(NTERM, 0.0);                                   // Allocate and initialize local coefs
    if(Ci->NCHILD==0) P2M(Ci);                                  // P2M kernel
    M2M(Ci);                                                    // M2M kernel
  }

  //! Dual tree traversal for a single pair of cells
  void traversal(C_iter Ci, C_iter Cj) {
    vec3 dX = Ci->X - Cj->X - Xperiodic;                        // Distance vector from source to target
    real_t R2 = norm(dX) * theta * theta;                       // Scalar distance squared
    if (R2 > (Ci->R + Cj->R) * (Ci->R + Cj->R)) {               // If distance is far enough
      M2L(Ci, Cj);                                              //  M2L kernel
    } else if (Ci->NCHILD == 0 && Cj->NCHILD == 0) {            // Else if both cells are leafs
      P2P(Ci, Cj);                                              //  P2P kernel
    } else if (Cj->NCHILD == 0 || Ci->R >= Cj->R) {             // If Cj is leaf or Ci is larger
      for (C_iter ci=Ci->CHILD; ci!=Ci->CHILD+Ci->NCHILD; ci++) {// Loop over Ci's children
        traversal(ci, Cj);                                      //   Traverse a single pair of cells
      }                                                         //  End loop over Ci's children
    } else {                                                    // Else if Ci is leaf or Cj is larger
      for (C_iter cj=Cj->CHILD; cj!=Cj->CHILD+Cj->NCHILD; cj++) {// Loop over Cj's children
        traversal(Ci, cj);                                      //   Traverse a single pair of cells
      }                                                         //  End loop over Cj's children
    }                                                           // End if for leafs and Ci Cj size
  }

  //! Tree traversal of periodic cells
  void traversePeriodic(C_iter Ci0, C_iter Cj0, vec3 cycle) {
    Cells pcells(27);                                           // Create cells
    for (C_iter C=pcells.begin(); C!=pcells.end(); C++) {       // Loop over periodic cells
      C->M.resize(NTERM, 0.0);                                  //  Allocate & initialize M coefs
      C->L.resize(NTERM, 0.0);                                  //  Allocate & initialize L coefs
    }                                                           // End loop over periodic cells
    C_iter Ci = pcells.end()-1;                                 // Last cell is periodic parent cell
    *Ci = *Cj0;                                                 // Copy values from source root
    Ci->CHILD = pcells.begin();                                 // Pointer of first periodic child cell
    Ci->NCHILD = 26;                                            // Number of periodic child cells
    for (int level=0; level<images-1; level++) {                // Loop over sublevels of tree
      for (int ix=-1; ix<=1; ix++) {                            //  Loop over x periodic direction
        for (int iy=-1; iy<=1; iy++) {                          //   Loop over y periodic direction
          for (int iz=-1; iz<=1; iz++) {                        //    Loop over z periodic direction
            if (ix != 0 || iy != 0 || iz != 0) {                //     If periodic cell is not at center
              for (int cx=-1; cx<=1; cx++) {                    //      Loop over x periodic direction (child)
                for (int cy=-1; cy<=1; cy++) {                  //       Loop over y periodic direction (child)
                  for (int cz=-1; cz<=1; cz++) {                //        Loop over z periodic direction (child)
                    Xperiodic[0] = (ix * 3 + cx) * cycle[0];    //   Coordinate offset for x periodic direction
                    Xperiodic[1] = (iy * 3 + cy) * cycle[1];    //   Coordinate offset for y periodic direction
                    Xperiodic[2] = (iz * 3 + cz) * cycle[2];    //   Coordinate offset for z periodic direction
                    M2L(Ci0, Ci);                               //         M2L kernel
                  }                                             //        End loop over z periodic direction (child)
                }                                               //       End loop over y periodic direction (child)
              }                                                 //      End loop over x periodic direction (child)
            }                                                   //     Endif for periodic center cell
          }                                                     //    End loop over z periodic direction
        }                                                       //   End loop over y periodic direction
      }                                                         //  End loop over x periodic direction
      C_iter Cj = pcells.begin();                               //  Iterator of periodic neighbor cells
      for (int ix=-1; ix<=1; ix++) {                            //  Loop over x periodic direction
        for (int iy=-1; iy<=1; iy++) {                          //   Loop over y periodic direction
          for (int iz=-1; iz<=1; iz++) {                        //    Loop over z periodic direction
            if (ix != 0 || iy != 0 || iz != 0) {                //     If periodic cell is not at center
              Cj->X[0] = Ci->X[0] + ix * cycle[0];              //      Set new x coordinate for periodic image
              Cj->X[1] = Ci->X[1] + iy * cycle[1];              //      Set new y cooridnate for periodic image
              Cj->X[2] = Ci->X[2] + iz * cycle[2];              //      Set new z coordinate for periodic image
              Cj->M = Ci->M;                                    //      Copy multipoles to new periodic image
              Cj++;                                             //      Increment periodic cell iterator
            }                                                   //     Endif for periodic center cell
          }                                                     //    End loop over z periodic direction
        }                                                       //   End loop over y periodic direction
      }                                                         //  End loop over x periodic direction
      M2M(Ci);                                                  //  Evaluate periodic M2M kernels for this sublevel
      cycle *= 3;                                               //  Increase periodic cycle by number of neighbors
    }                                                           // End loop over sublevels of tree
  }

  //! Evaluate P2P and M2L using list based traversal
  void traversal(C_iter Ci0, C_iter Cj0, vec3 cycle) {
    if (images == 0) {                                          // If non-periodic boundary condition
      traversal(Ci0, Cj0);                                      //  Traverse the tree
    } else {                                                    // If periodic boundary condition
      for (int ix=-1; ix<=1; ix++) {                            //  Loop over x periodic direction
        for (int iy=-1; iy<=1; iy++) {                          //   Loop over y periodic direction
          for (int iz=-1; iz<=1; iz++) {                        //    Loop over z periodic direction
            Xperiodic[0] = ix * cycle[0];                       //     Coordinate shift for x periodic direction
            Xperiodic[1] = iy * cycle[1];                       //     Coordinate shift for y periodic direction
            Xperiodic[2] = iz * cycle[2];                       //     Coordinate shift for z periodic direction
            traversal(Ci0, Cj0);                                //     Traverse the tree for this periodic image
          }                                                     //    End loop over z periodic direction
        }                                                       //   End loop over y periodic direction
      }                                                         //  End loop over x periodic direction
      traversePeriodic(Ci0, Cj0, cycle);                        //  Traverse tree for periodic images
    }                                                           // End if for periodic boundary condition
  }

  //! Pre-order traversal for downward pass
  void downwardPass(C_iter Cj) {
    L2L(Cj);                                                    // L2L kernel
    if (Cj->NCHILD==0) L2P(Cj);                                 // L2P kernel
    for (C_iter Ci=Cj->CHILD; Ci!=Cj->CHILD+Cj->NCHILD; Ci++) { // Loop over child cells
      downwardPass(Ci);                                         //  Recursive call for child cell
    }                                                           // End loop over chlid cells
  }

  //! Direct summation
  void direct(Bodies & bodies, Bodies & jbodies, vec3 cycle) {
    Cells cells(2);                                             // Define a pair of cells to pass to P2P kernel
    C_iter Ci = cells.begin();                                  // Allocate single target
    C_iter Cj = cells.begin()+1;                                // Allocate single source
    for (int ix=-1; ix<=1; ix++) {                              //  Loop over x periodic direction
      for (int iy=-1; iy<=1; iy++) {                            //   Loop over y periodic direction
        for (int iz=-1; iz<=1; iz++) {                          //    Loop over z periodic direction
          Xperiodic[0] = ix * cycle[0];                         //     Coordinate shift for x periodic direction
          Xperiodic[1] = iy * cycle[1];                         //     Coordinate shift for y periodic direction
          Xperiodic[2] = iz * cycle[2];                         //     Coordinate shift for z periodic direction
          Ci->BODY = &bodies[0];                                //     Iterator of first target body
          Ci->NBODY = bodies.size();                            //     Number of target bodies
          Cj->BODY = &jbodies[0];                               //     Iterator of first source body
          Cj->NBODY = jbodies.size();                           //     Number of source bodies
          P2P(Ci, Cj);                                          //     Evaluate P2P kenrel
        }                                                       //    End loop over z periodic direction
      }                                                         //   End loop over y periodic direction
    }                                                           //  End loop over x periodic direction
  }
}
#endif
