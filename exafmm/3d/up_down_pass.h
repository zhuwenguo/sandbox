#ifndef up_down_pass_h
#define up_down_pass_h
#include "types.h"

namespace exafmm {
  class UpDownPass {
  private:
    //! Post-order traversal for upward pass
    void postOrderTraversal(C_iter C, C_iter C0) {
      for (C_iter CC=C0+C->ICHILD; CC!=C0+C->ICHILD+C->NCHILD; CC++) { // Loop over child cells
        postOrderTraversal(CC, C0);                             //  Recursive call for child cell
      }                                                         // End loop over child cells
      if(C->NCHILD==0) P2M(C);                                  // P2M kernel
      else {                                                    // If not leaf cell
        M2M(C, C0);                                             //  M2M kernel
      }                                                         // End if for non leaf cell
    };

    //! Pre-order traversal for downward pass
    void preOrderTraversal(C_iter C, C_iter C0) {
      L2L(C, C0);                                               //  L2L kernel
      if (C->NCHILD==0) {                                       //  If leaf cell
        L2P(C);                                                 //  L2P kernel
      }                                                         // End if for leaf cell
      for (C_iter CC=C0+C->ICHILD; CC!=C0+C->ICHILD+C->NCHILD; CC++) {// Loop over child cells
        preOrderTraversal(CC, C0);                              //  Recursive call for child cell
      }                                                         // End loop over chlid cells
    };

  public:
    //! Upward pass (P2M, M2M)
    void upwardPass(Cells & cells) {
      if (!cells.empty()) {                                     // If cell vector is not empty
	C_iter C0 = cells.begin();                              //  Set iterator of target root cell
        for (C_iter C=cells.begin(); C!=cells.end(); C++) {     //  Loop over cells
          C->M.resize(NTERM, 0.0);                              //   Allocate & initialize M coefs
          C->L.resize(NTERM, 0.0);                              //   Allocate & initialize L coefs
        }                                                       //  End loop over cells
	postOrderTraversal(C0, C0);                             //  Start post-order traversal from root
      }                                                         // End if for empty cell vector
    }

    //! Downward pass (L2L, L2P)
    void downwardPass(Cells & cells) {
      if (!cells.empty()) {                                     // If cell vector is not empty
	C_iter C0 = cells.begin();                              //  Root cell
	if (C0->NCHILD == 0 ) {                                 //  If root is the only cell
          L2P(C0);                                              //   L2P kernel
        }                                                       //  End if root is the only cell
	for (C_iter CC=C0+C0->ICHILD; CC!=C0+C0->ICHILD+C0->NCHILD; CC++) {// Loop over child cells
	  preOrderTraversal(CC, C0);                            //   Start pre-order traversal from root
	}                                                       //  End loop over child cells
      }                                                         // End if for empty cell vector
    }
  };
}
#endif
