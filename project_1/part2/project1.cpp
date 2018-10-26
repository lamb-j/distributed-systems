#include "timing.h"
#include "funcs.h"

vector <Node *> nv;
vector <exNode *> env;

int main(int argc, char **argv) {

  // check args
  if (argc != 4) {
    printf("usage: project1 edge_file partition_file rounds\n");
    exit(0);
  }

  // parse args
  string edge_file = string (argv[1]);
  string part_file = string (argv[2]);
  int rounds = atoi(argv[3]);

  // total overall time
  double total_overall_time = rtc();

  // Set up MPI 
  int rank;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // open output log file
  string output_file = "rank_" + to_string(rank) + ".log";

  double total_rank_time = rtc();
  {
    // read from file
    reset_and_start_timer();
    read_from_files(edge_file, part_file);
    double read_time = get_elapsed_msec();
    fprintf(stdout, "rank: %d, read:\t%.3f sec\n", rank, read_time / 1000.0);
    fflush(stdout);

    MPI_Barrier(MPI_COMM_WORLD);

    // rounds
    // start from round 1 (round 0 has credit as 1 for all nodes by default)

    for (int r = 1; r <= rounds; r++) {
      if (rank == MASTER) printf("\n");
      MPI_Barrier(MPI_COMM_WORLD);

      //if (rank == MASTER) printf("--- ROUND %d ---\n", r);

      reset_and_start_timer();
      double total_rank_time = rtc();

      round_update(r);

      fprintf(stdout, "rank: %d, round %d: %.3f sec\n", rank, r, get_elapsed_msec() / 1000);
      fflush(stdout);

      MPI_Barrier(MPI_COMM_WORLD);
      total_rank_time = rtc() - total_rank_time;
      if (rank == MASTER) {
        printf("total time for round %d: %.3f sec\n", rank, total_rank_time);
        fflush(stdout);
      }

    }

    // verify
    //verify();

    // write to file
    reset_and_start_timer();
    write_credits_to_file("rank_" + to_string(rank) + ".txt");
    double write_time = get_elapsed_msec();
    fprintf(stdout, "rank: %d, write:\t%.3f sec\n", rank, write_time / 1000);
    fflush(stdout);

    // cleanup
    cleanup();

    
  }
  total_rank_time = rtc() - total_rank_time;

  MPI_Finalize();

  total_overall_time = rtc() - total_overall_time;
  if (rank == MASTER) printf("\nTotal overall time: %f sec\n", total_overall_time);
 
}
