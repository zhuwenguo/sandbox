#include "serialfmm.h"

int main() {
  double tic, toc;
  int numBodies = 1000;
  THETA = 0.6;
  Bodies bodies;
  SerialFMM FMM;
  for( int it=0; it<25; ++it ) {
    numBodies = int(pow(10,(it+24)/8.0));
    std::cout << "N                    : " << numBodies << std::endl;
    bodies.resize(numBodies);
    FMM.dataset(bodies,numBodies);

    tic = FMM.getTime();
    FMM.bottomup(bodies);
    FMM.evaluate();
    toc = FMM.getTime();
    if( FMM.printNow ) printf("FMM                  : %lf\n",toc-tic);

    tic = FMM.getTime();
    FMM.direct(bodies);
    toc = FMM.getTime();
    if( FMM.printNow ) printf("Direct               : %lf\n",toc-tic);
  }
}
