#ifndef build_tree_tbb_h
#define build_tree_tbb_h
#include "types.h"

namespace exafmm {
  typedef vec<8,int> ivec8;                                     //!< Vector of 8 integer types

  //! Octree is used for building the FMM tree structure as "nodes", then transformed to "cells" data structure
  struct OctreeNode {
    int          IBODY;                                         //!< Index offset for first body in node
    int          NBODY;                                         //!< Number of descendant bodies
    int          NNODE;                                         //!< Number of descendant nodes
    OctreeNode * CHILD[8];                                      //!< Pointer to child node
    real_t       X[3];                                          //!< Coordinate at center
  };

  int ncrit;                                                    //!< Number of bodies per leaf cell
  real_t x[3];                                                  //!< Coordinate vector
  Body * B0;                                                    //!< Pointer of first body
  OctreeNode * N0;                                              //!< Pointer to octree root node

  //! Counting bodies in each octant
  void countBodies(Bodies & bodies, int begin, int end, real_t * X, ivec8 & NBODY) {
    for (int i=0; i<8; i++) NBODY[i] = 0;                       // Initialize number of bodies in octant
    for (int i=begin; i<end; i++) {                             // Loop over bodies in node
      for (int d=0; d<3; d++) x[d] = bodies[i].X[d];            //  Coordinates of body
      int octant = (x[0] > X[0]) + ((x[1] > X[1]) << 1) + ((x[2] > X[2]) << 2);// Which octant body belongs to
      NBODY[octant]++;                                          //  Increment body count in octant
    }                                                           // End loop over bodies in node
  }

  //! Sorting bodies according to octant (Morton order)
  void moveBodies(Bodies & bodies, Bodies & buffer, int begin, int end,
                  ivec8 octantOffset, real_t * X) {
    for (int i=begin; i<end; i++) {                             // Loop over bodies
      for (int d=0; d<3; d++) x[d] = bodies[i].X[d];            //  Coordinates of body
      int octant = (x[0] > X[0]) + ((x[1] > X[1]) << 1) + ((x[2] > X[2]) << 2);// Which octant body belongs to`
      buffer[octantOffset[octant]] = bodies[i];                 //   Permute bodies out-of-place according to octant
      octantOffset[octant]++;                                   //  Increment body count in octant
    }                                                           // End loop over bodies
  }

  //! Create an octree node
  OctreeNode * makeOctNode(OctreeNode *& octNode, int begin, int end, real_t * X, bool nochild) {
    octNode = new OctreeNode();                                 // Allocate memory for single node
    octNode->IBODY = begin;                                     // Index of first body in node
    octNode->NBODY = end - begin;                               // Number of bodies in node
    octNode->NNODE = 1;                                         // Initialize counter for decendant nodes
    for (int d=0; d<3; d++) octNode->X[d] = X[d];               // Center coordinates of node
    if (nochild) {                                              // If node has no children
      for (int i=0; i<8; i++) octNode->CHILD[i] = NULL;         //  Initialize pointers to children
    }                                                           // End if for node children
    return octNode;                                             // Return node
  }

  //! Exclusive scan with offset
  inline ivec8 exclusiveScan(ivec8 input, int offset) {
    ivec8 output;                                               // Output vector
    for (int i=0; i<8; i++) {                                   // Loop over elements
      output[i] = offset;                                       //  Set value
      offset += input[i];                                       //  Increment offset
    }                                                           // End loop over elements
    return output;                                              // Return output vector
  }

  //! Recursive functor for building nodes of an octree adaptively using a top-down approach
  void buildNodes(OctreeNode *& octNode, Bodies & bodies, Bodies & buffer,
                  int begin, int end, real_t * X, real_t R0,
                  int level=0, bool direction=false) {
    if (begin == end) {                                         // If no bodies are left
      octNode = NULL;                                           //  Assign null pointer
      return;                                                   //  End buildNodes()
    }                                                           // End if for no bodies
    if (end - begin <= ncrit) {                                 // If number of bodies is less than threshold
      if (direction)                                            //  If direction of data is from bodies to buffer
        for (int i=begin; i<end; i++) buffer[i] = bodies[i];    //   Copy bodies to buffer
      octNode = makeOctNode(octNode, begin, end, X, true);      //  Create an octree node and assign it's pointer
      return;                                                   //  End buildNodes()
    }                                                           // End if for number of bodies
    octNode = makeOctNode(octNode, begin, end, X, false);       // Create an octree node with child nodes
    ivec8 NBODY;                                                // Number of bodies in node
    countBodies(bodies, begin, end, X, NBODY);                  // Count bodies in each octant
    ivec8 octantOffset = exclusiveScan(NBODY, begin);           // Exclusive scan to obtain offset from octant count
    moveBodies(bodies, buffer, begin, end, octantOffset, X);    // Sort bodies according to octant
    real_t Xchild[3];                                           // Coordinates of child node
    for (int i=0; i<8; i++) {                                   // Loop over children
      for (int d=0; d<3; d++) Xchild[d] = X[d];                 //  Initialize center coordinates of child node
      real_t r = R0 / (1 << (level + 1));                       //  Radius of cells for child's level
      for (int d=0; d<3; d++) {                                 //  Loop over dimensions
        Xchild[d] += r * (((i & 1 << d) >> d) * 2 - 1);         //   Shift center coordinates to that of child node
      }                                                         //  End loop over dimensions
      buildNodes(octNode->CHILD[i], buffer, bodies,             //  Recursive call for children
                 octantOffset[i], octantOffset[i] + NBODY[i],
                 Xchild, R0, level+1, !direction);
    }                                                           // End loop over children
    for (int i=0; i<8; i++) {                                   // Loop over children
      if (octNode->CHILD[i]) octNode->NNODE += octNode->CHILD[i]->NNODE;// If child exists increment child node count
    }                                                           // End loop over chlidren
  }

  //! Creating cell data structure from nodes
  void nodes2cells(OctreeNode * octNode, C_iter C,
                   C_iter C0, C_iter CN, real_t * X0, real_t R0,
                   int level=0) {
    C->R = R0 / (1 << level);                                   //  Cell radius
    for (int d=0; d<3; d++) C->X[d] = octNode->X[d];            //  Cell center
    C->NBODY = octNode->NBODY;                                  //  Number of decendant bodies
    C->BODY = B0 + octNode->IBODY;                              //  Iterator of first body in cell
    if (octNode->NNODE == 1) {                                  //  If node has no children
      C->ICHILD = 0;                                            //   Set index of first child cell to zero
      C->NCHILD = 0;                                            //   Number of child cells
    } else {                                                    //  Else if node has children
      int nchild = 0;                                           //   Initialize number of child cells
      int octants[8];                                           //   Map of child index to octants
      for (int i=0; i<8; i++) {                                 //   Loop over octants
        if (octNode->CHILD[i]) {                                //    If child exists for that octant
          octants[nchild] = i;                                  //     Map octant to child index
          nchild++;                                             //     Increment child cell counter
        }                                                       //    End if for child existance
      }                                                         //   End loop over octants
      C_iter Ci = CN;                                           //   CN points to the next free memory address
      C->ICHILD = Ci - C0;                                      //   Set Index of first child cell
      C->NCHILD = nchild;                                       //   Number of child cells
      CN += nchild;                                             //   Increment next free memory address
      for (int i=0; i<nchild; i++) {                            //   Loop over children
        int octant = octants[i];                                //    Get octant from child index
        nodes2cells(octNode->CHILD[octant], Ci, C0, CN,         //    Recursive call for child cells
                    X0, R0, level+1);
        Ci++;                                                   //    Increment cell iterator
        CN += octNode->CHILD[octant]->NNODE - 1;                //    Increment next free memory address
      }                                                         //   End loop over children
      for (int i=0; i<nchild; i++) {                            //   Loop over children
        int octant = octants[i];                                //    Get octant from child index
        delete octNode->CHILD[octant];                          //    Free child pointer to avoid memory leak
      }                                                         //   End loop over children
    }                                                           //  End if for child existance
  };

  //! Transform Xmin & Xmax to X (center) & R (radius)
  Box getBox(Bodies & bodies) {
    real_t Xmin[3], Xmax[3];                                    // Set local Xmin
    for (int d=0; d<3; d++) Xmin[d] = Xmax[d] = bodies.front().X[d];// Initialize Xmin, Xmax
    for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {       // Loop over bodies
      for (int d=0; d<3; d++) Xmin[d] = fmin(B->X[d], Xmin[d]); //  Update Xmin
      for (int d=0; d<3; d++) Xmax[d] = fmax(B->X[d], Xmax[d]); //  Update Xmax
    }                                                           // End loop over bodies
    Box box;                                                    // Bounding box
    for (int d=0; d<3; d++) box.X[d] = (Xmax[d] + Xmin[d]) / 2; // Calculate center of domain
    box.R = 0;                                                  // Initialize localRadius
    for (int d=0; d<3; d++) {                                   // Loop over dimensions
      box.R = fmax(box.X[d] - Xmin[d], box.R);                  //  Calculate min distance from center
      box.R = fmax(Xmax[d] - box.X[d], box.R);                  //  Calculate max distance from center
    }                                                           // End loop over dimensions
    box.R *= 1.00001;                                           // Add some leeway to radius
    return box;                                                 // Return box.X and box.R
  }

  //! Build tree structure top down
  Cells buildTree(Bodies & bodies, Bodies & buffer) {
    Box box = getBox(bodies);                                   // Get bounding box
    if (bodies.empty()) {                                       // If bodies vector is empty
      N0 = NULL;                                                //  Reinitialize N0 with NULL
    } else {                                                    // If bodies vector is not empty
      if (bodies.size() > buffer.size()) buffer.resize(bodies.size());// Enlarge buffer if necessary
      B0 = &bodies[0];                                          // Pointer of first body
      buildNodes(N0, bodies, buffer, 0, bodies.size(),          // Build octree nodes
                 box.X, box.R);
    }                                                           // End if for empty root
    Cells cells;                                                // Initialize cell array
    if (N0 != NULL) {                                           // If the node tree is not empty
      cells.resize(N0->NNODE);                                  //  Allocate cells array
      C_iter C0 = cells.begin();                                //  Cell begin iterator
      nodes2cells(N0, C0, C0, C0+1, box.X, box.R);              // Instantiate recursive functor
      delete N0;                                                //  Deallocate nodes
    }                                                           // End if for empty node tree
    return cells;                                               // Return cells array
  }
}
#endif
