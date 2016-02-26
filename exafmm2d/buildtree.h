#ifndef buildtree_h
#define buildtree_h
#include "types.h"

//! Build cells of tree adaptively using a top-down approach based on recursion (uses task based thread parallelism)
Cell * buildTree(Bodies& bodies, Bodies& buffer, B_iter B0, int begin, int end,
		 vec2 X, real_t R0, int ncrit, int level=0, bool direction=false) {
  if (begin == end) return NULL;                              // If no bodies left, return null pointer
  //! Create a tree cell
  Cell * cell = new Cell();                                   // Allocate memory for single cell
  cell->BODY = B0 + begin;                                    // Iterator of first body in cell
  cell->NBODY = end - begin;                                  // Number of bodies in cell
  cell->NNODE = 1;                                            // Initialize counter for decendant cells
  cell->X = X;                                                // Center position of cell
  cell->R = R0 / (1 << level);                                // Cell radius
  for (int i=0; i<4; i++) cell->CHILD[i] = NULL;              //  Initialize pointers to children
  //! If cell is a leaf
  if (end - begin <= ncrit) {                                 // If number of bodies is less than threshold
    if (direction)                                            //  If direction of data is from bodies to buffer
      for (int i=begin; i<end; i++) buffer[i] = bodies[i];    //   Copy bodies to buffer
    return cell;                                              //  Return cell pointer
  }                                                           // End if for number of bodies
  //! Count number of bodies in each quadrant 
  ivec4 size = 0;
  for (int i=begin; i<end; i++) {                             //  Loop over bodies in cell
    vec2 x = bodies[i].X;                                     //   Position of body
    int quadrant = (x[0] > X[0]) + ((x[1] > X[1]) << 1);      //   Which quadrant body belongs to
    size[quadrant]++;                                         //   Increment body count in quadrant
  }                                                           //  End loop over bodies in cell
  //! Exclusive scan to get offsets
  int offset = begin;                                         // Offset of first quadrant
  ivec4 offsets;                                              // Output vector
  for (int i=0; i<4; i++) {                                   // Loop over elements
    offsets[i] = offset;                                      //  Set value
    offset += size[i];                                        //  Increment offset
  }                                                           // End loop over elements
  //! Sort bodies by quadrant
  ivec4 counter = offsets;
  for (int i=begin; i<end; i++) {                             //  Loop over bodies
    vec2 x = bodies[i].X;                                     //   Position of body
    int quadrant = (x[0] > X[0]) + ((x[1] > X[1]) << 1);      //   Which quadrant body belongs to`
    buffer[counter[quadrant]] = bodies[i];                    //   Permute bodies out-of-place according to quadrant
    counter[quadrant]++;                                      //   Increment body count in quadrant
  }                                                           //  End loop over bodies
  //! Loop over children and recurse
  for (int i=0; i<4; i++) {                                   // Loop over children
    vec2 Xchild = X;                                          //   Initialize center position of child cell
    real_t r = R0 / (1 << (level + 1));                       //   Radius of cells for child's level
    for (int d=0; d<2; d++) {                                 //   Loop over dimensions
      Xchild[d] += r * (((i & 1 << d) >> d) * 2 - 1);         //    Shift center position to that of child cell
    }                                                         //   End loop over dimensions
    cell->CHILD[i] = buildTree(buffer, bodies, B0,            //   Recursive call for each child
			       offsets[i], offsets[i] + size[i],//   Range of bodies is calcuated from quadrant offset
			       Xchild, R0, ncrit, level+1, !direction);//   Alternate copy direction bodies <-> buffer
  }                                                           // End loop over children
  //! Accumulate number of decendant cells
  for (int i=0; i<4; i++) {                                   // Loop over children
    if (cell->CHILD[i]) {                                     //  If child exists
      cell->NNODE += cell->CHILD[i]->NNODE;                   //   Increment child cell counter
    }                                                         //  End if for child
  }                                                           // End loop over chlidren
  return cell;                                                // Return quadtree cell
}

#endif
