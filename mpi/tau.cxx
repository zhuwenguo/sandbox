#include <mpi.h>
#include <unistd.h>
#include <iostream>

class Sleep {
public:
  void sleep() {
    usleep(100);
  }
};

int main(int argc, char **argv) {
  char hostname[256];
  int size,rank;
  Sleep S;
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  gethostname(hostname,sizeof(hostname));
  for( int irank=0; irank!=size; ++irank ) {
    MPI_Barrier(MPI_COMM_WORLD);
    if( rank == irank ) {
      std::cout << hostname << " " << rank << " / " << size << std::endl;
    }
    S.sleep();
  }
  MPI_Finalize();
}
