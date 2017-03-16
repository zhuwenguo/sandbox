#include "bound_box.h"
#include "build_tree.h"
#include "ewald.h"
#include "kernel.h"
#include "logger.h"
#include "namespace.h"
#include "traversal.h"
#include "up_down_pass.h"
#include "verify.h"
using namespace EXAFMM_NAMESPACE;

int main(int argc, char ** argv) {
  const int numBodies = 1000;
  const int P = 10;
  const int ncrit = 64;
  const int images = 4;
  const int ksize = 11;
  const real_t cycle = 2 * M_PI;
  const real_t alpha = ksize / cycle;
  const real_t sigma = .25 / M_PI;
  const real_t cutoff = cycle / 2;
  const real_t eps2 = 0.0;
  const real_t theta = 0.4;
  logger::verbose = true;

  // Initialize bodies
  Bodies bodies(numBodies);
  real_t average = 0;
  srand48(0);
  for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {
    for (int d=0; d<3; d++) {
      B->X[d] = drand48() * cycle - cycle * .5;
    }
  }
  for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {
    B->SRC = drand48() - .5;
    average += B->SRC;
  }
  average /= numBodies;
  for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {
    B->SRC -= average;
    B->TRG = 0;
  }

  // Build tree
  logger::printTitle("FMM Profiling");
  logger::startTimer("Total FMM");
  BoundBox boundBox;
  Bounds bounds = boundBox.getBounds(bodies);
  Bodies buffer(numBodies);
  BuildTree buildTree(ncrit);
  Cells cells = buildTree.buildTree(bodies, buffer, bounds);

  // FMM evaluation
  Kernel kernel(P, eps2);
  UpDownPass upDownPass(kernel);
  upDownPass.upwardPass(cells);
  Traversal traversal(kernel, theta, images);
  traversal.traverse(cells, cells, cycle);
  upDownPass.downwardPass(cells);

  // Dipole correction
  buffer = bodies;
  vec3 dipole = upDownPass.getDipole(bodies,0);
  upDownPass.dipoleCorrection(bodies, dipole, numBodies, cycle);

  // Ewald summation
  logger::printTitle("Ewald Profiling");
  logger::startTimer("Total Ewald");
  Bodies bodies2 = bodies;
  for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {
    B->TRG = 0;
  }
  Bodies jbodies = bodies;
  bounds = boundBox.getBounds(jbodies);
  Cells jcells = buildTree.buildTree(jbodies, buffer, bounds);
  Ewald ewald(ksize, alpha, sigma, cutoff, cycle);
  ewald.wavePart(bodies, jbodies);
  ewald.realPart(cells, jcells);
  ewald.selfTerm(bodies);
  logger::printTitle("Total runtime");
  logger::printTime("Total FMM");
  logger::stopTimer("Total Ewald");

  // Verify result
  Verify verify;
  double potSum = verify.getSumScalar(bodies);
  double potSum2 = verify.getSumScalar(bodies2);
  double accDif = verify.getDifVector(bodies, bodies2);
  double accNrm = verify.getNrmVector(bodies);
  double potDif = (potSum - potSum2) * (potSum - potSum2);
  double potNrm = potSum * potSum;
  double potRel = std::sqrt(potDif/potNrm);
  double accRel = std::sqrt(accDif/accNrm);
  logger::printTitle("FMM vs. Ewald");
  verify.print("Rel. L2 Error (pot)",potRel);
  verify.print("Rel. L2 Error (acc)",accRel);
  return 0;
}
