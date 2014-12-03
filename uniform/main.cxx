#include "fmm.h"
#include "logger.h"

int main() {
  Fmm FMM;
  const int numBodies = 10000;
  const int ncrit = 100;
  const int maxLevel = numBodies >= ncrit ? 1 + int(log(numBodies / ncrit)/M_LN2/3) : 0;
  const real cycle = 10 * M_PI;

  FMM.allocate(numBodies, maxLevel);
  logger::verbose = true;
  logger::printTitle("FMM Profiling");
  for( int it=0; it<1; it++ ) {
    int ix[3] = {0, 0, 0};
    FMM.R0 = 0.5 * cycle;
    for_3d FMM.X0[d] = FMM.R0;
    srand48(0);
    real average = 0;
    for( int i=0; i<numBodies; i++ ) {
      FMM.Jbodies[i][0] = 2 * FMM.R0 * (drand48() + ix[0]);
      FMM.Jbodies[i][1] = 2 * FMM.R0 * (drand48() + ix[1]);
      FMM.Jbodies[i][2] = 2 * FMM.R0 * (drand48() + ix[2]);
      FMM.Jbodies[i][3] = (drand48() - .5) / numBodies;
      average += FMM.Jbodies[i][3];
    }
    average /= numBodies;
    for( int i=0; i<numBodies; i++ ) {
      FMM.Jbodies[i][3] -= average;
    }
  
    logger::startTimer("Grow tree");
    FMM.sortBodies();
    FMM.buildTree();
    logger::stopTimer("Grow tree");
  
    logger::startTimer("Upward pass");
    for( int i=0; i<FMM.numCells; i++ ) {
      for_m FMM.Multipole[i][m] = 0;
      for_l FMM.Local[i][l] = 0;
    }
    FMM.P2M();
    FMM.M2M();
    logger::stopTimer("Upward pass");

    logger::startTimer("Traverse");
    FMM.M2L();
    logger::stopTimer("Traverse", 0);

    logger::startTimer("Downward pass");
    FMM.L2L();
    FMM.L2P();
    logger::stopTimer("Downward pass");

    logger::startTimer("Traverse");
    FMM.P2P();
    logger::stopTimer("Traverse");
  
    double potDif = 0, potNrm = 0, accDif = 0, accNrm = 0;
    for (int i=0; i<100; i++) {
      real Ibodies[4] = {0, 0, 0, 0};
      real Jbodies[4], dX[3];
      for_4d Jbodies[d] = FMM.Jbodies[i][d];
      for (int j=0; j<numBodies; j++) {
	for_3d dX[d] = Jbodies[d] - FMM.Jbodies[j][d];
	real R2 = dX[0] * dX[0] + dX[1] * dX[1] + dX[2] * dX[2];
	real invR2 = R2 == 0 ? 0 : 1.0 / R2;
	real invR = FMM.Jbodies[j][3] * sqrtf(invR2);
	for_3d dX[d] *= invR2 * invR;
	Ibodies[0] += invR;
	Ibodies[1] -= dX[0];
	Ibodies[2] -= dX[1];
	Ibodies[3] -= dX[2];
      }
      potDif += (FMM.Ibodies[i][0] - Ibodies[0]) * (FMM.Ibodies[i][0] - Ibodies[0]);
      potNrm += Ibodies[0] * Ibodies[0];
      for_3d accDif += (FMM.Ibodies[i][d+1] - Ibodies[d+1]) * (FMM.Ibodies[i][d+1] - Ibodies[d+1]);
      for_3d accNrm += (Ibodies[d+1] * Ibodies[d+1]);
    }
    logger::printTitle("FMM vs. direct");
    logger::printError("Rel. L2 Error (pot)",std::sqrt(potDif/potNrm));
    logger::printError("Rel. L2 Error (acc)",std::sqrt(accDif/accNrm));
  }
  FMM.deallocate();
}
