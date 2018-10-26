#include "timing.h"
#include "funcs.h"

vector <Node *> nv;

int main(int argc, char **argv) {

  // check args
  if (argc != 3) {
    printf("usage: project1 file_name rounds\n");
    exit(0);
  }

  string input_file = string (argv[1]);
  int rounds = atoi(argv[2]);

  double total_time = rtc();
  {
    // read from file
    reset_and_start_timer();
    read_edges_from_file(input_file);
    double read_time = get_elapsed_msec();


    // rounds
    vector <double> round_times;
    round_times.push_back(0);

    // start from round 1 (round 0 has credit as 1 for all nodes by default)
    for (int r = 1; r <= rounds; r++) {
      reset_and_start_timer();
      round_update(r);
      round_times.push_back(get_elapsed_msec());
    }

    // verify 
    verify();

    // write to file
    reset_and_start_timer();
    write_credits_to_file("out.txt");
    double write_time = get_elapsed_msec();

    // cleanup
    cleanup();

    // timing info
    printf("Timing (ms):\n");
    printf("  read:\t%f\n", read_time);
    //printf("  graph:\t%f\n", graph_time);
    printf("  rounds:\n");

    for (int i = 0; i < round_times.size(); i++) 
      printf("    round %d: %f\n", i, round_times[i]);

    printf("  write:\t%f\n", write_time);
  }
  total_time = rtc() - total_time;
  printf("\nTotal time: %f sec\n", total_time);
 
}
