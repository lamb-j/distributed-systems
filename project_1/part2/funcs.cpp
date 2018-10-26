#include "funcs.h"
#include "timing.h"
using namespace std;

extern vector <Node *> nv;
extern vector <exNode *> env;

exNode::exNode(int ID, int degree, int part) {
  this->ID = ID;
  this->degree = degree;
  this->part = part;

  // intalize credit to 1
  credit = 1.0;
}

// Node class functions
Node::Node(int ID, int degree) {
  this->ID = ID;
  this->degree = degree;

  // intalize credit to 1
  credit.push_back(1);
}

void Node::print(FILE *of)
{

  fprintf(of, "ID: %d, degree: %d, credit: %f\n", ID, degree, credit.back());
  fprintf(of, "  Internals:\n");
  for (int i = 0; i < internals.size(); i++) {
    fprintf(of, "    ID: %d\n", internals[i]->ID);
  }
  fprintf(of, "  Externals:\n");
  for (int i = 0; i < externals.size(); i++) {
    fprintf(of, "    ID: %d\n", externals[i]->ID);
  }
}

// Update algorithm for a single node
void Node::update_internals(int round) {

  double new_credit = 0;

  // credit from internal neighbors
  for (int i = 0; i < internals.size(); i++) {
    new_credit += internals[i]->credit[round - 1] / internals[i]->degree;
  }

  // credit from external neighbors
  for (int i = 0; i < externals.size(); i++) {
    new_credit += externals[i]->credit / externals[i]->degree;
  }

  credit.push_back(new_credit);

  return;
}

// Independant functions
Node *get_internal_node(int ID, int degree) {

  Node *rv;

  if (nv[ID] == NULL) {
    rv = new Node(ID, degree);
    nv[ID] = rv;
  }
  else {
    rv = nv[ID];
  }

  return rv;
}

exNode *get_external_node(int ID, int degree, int part) {

  exNode *rv;

  if (env[ID] == NULL) {
    rv = new exNode(ID, degree, part);
    env[ID] = rv;
  }
  else {
    rv = env[ID];
  }

  return rv;
}

void request_externals(int target) {

  for (int i = 0; i < env.size(); i++) {

    // skip unused or internal nodes
    if (env[i] == NULL) continue;
    if (env[i]->part != target) continue; 

    // ask for env[i]'s credit, update with response
    MPI_Send(&env[i]->ID, 1, MPI_INT, target, 0, MPI_COMM_WORLD); 

    // MPI RECV env[i]'s credit from round-1 (comes from partitions nv vector)
    MPI_Recv(&env[i]->credit, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  // send done when finished
  int done = -1;
  MPI_Send(&done, 1, MPI_INT, target, 0, MPI_COMM_WORLD); 

  return;
}

// provide credit from previous round
void provide_internals(int target, int round) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  while (1) {
    int ex_ID;
    MPI_Recv(&ex_ID, 1, MPI_INT, target, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // done condition
    if (ex_ID == -1) {
      //printf("done recieved from target: %d\n", target);
      break;
    }

    if (nv[ex_ID] == NULL) {
      printf("BAD CREDIT REQUEST: Target Rank: %d, Handler Rank:%d, ID:%d\n",
          target, rank, ex_ID);
    }

    //printf("HANDLE CREDIT REQUEST: Round: %d, Target Rank: %d, Handler Rank:%d, ID:%d\n", \ round, target, rank, ex_ID);

    double reply_credit;
    reply_credit = nv[ex_ID]->credit[round-1];

    MPI_Send(&reply_credit, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD); 
  }

  return;
}

void round_update(int round) {
  int rank, numtasks;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  // ----
  // exchange externals (SLAVE -> MASTER, MASTER -> SLAVE)

  if (numtasks == 1) {
    // no exchanges needed
  }

  // 2 MPI tasks
  else if (numtasks == 2) {
    if (rank == MASTER && round > 1) {
      request_externals(SLAVE);
      provide_internals(SLAVE, round);
    }

    if (rank == SLAVE && round > 1) {
      provide_internals(MASTER, round);
      request_externals(MASTER);
    }
  }

  // 4 MPI tasks
  // 0 <-> 1, 0 <-> 2, 0 <-> 3
  // 1 <-> 2, 1 <-> 3
  // 2 <-> 3
  else if (numtasks == 4) {

    if (rank == 0 && round > 1) {
      request_externals(1);
      provide_internals(1, round);

      request_externals(2);
      provide_internals(2, round);

      request_externals(3);
      provide_internals(3, round);

    }

    if (rank == 1 && round > 1) {
      provide_internals(0, round);
      request_externals(0);

      request_externals(2);
      provide_internals(2, round);

      request_externals(3);
      provide_internals(3, round);

    }

    if (rank == 2 && round > 1) {
      provide_internals(0, round);
      request_externals(0);

      provide_internals(1, round);
      request_externals(1);

      request_externals(3);
      provide_internals(3, round);

    }

    if (rank == 3 && round > 1) {
      provide_internals(0, round);
      request_externals(0);

      provide_internals(1, round);
      request_externals(1);

      provide_internals(2, round);
      request_externals(2);
    }

  }
  else {
    printf("ERROR: INVALID NUMBER OF MPI PROCESSES: %d\n", numtasks);
    exit(0);
  }

  // maybe don't need
  MPI_Barrier(MPI_COMM_WORLD);

  // ----
  // calculate new credit values
  for (int i = 0; i < nv.size(); i++) {
    if (nv[i] != NULL) 
      nv[i]->update_internals(round);
  }

  return;
}

void read_from_files(string edge_file, string part_file) 
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  reset_and_start_timer();
  //printf("Rank %d: Reading from file", rank);

  // Read from file
  // Uses boost library to parse file into data_t data
  data_i2 edges;
  parse_i2(edge_file, edges);

  // parts[i] should refernce ID:i+1
  // might need to sort this by ID if not sorted...
  data_i3 parts;
  parse_i3(part_file, parts);

  //printf("... %f secs\n", get_elapsed_msec() / 1000 );

  reset_and_start_timer();
  //printf("Rank %d: Resizing vector", rank);

  // find largest node ID
  int l_ID ;
  l_ID = 0;

  for (int i = 0; i < parts.size(); i++) {
    l_ID = parts[i].ID > l_ID ? parts[i].ID : l_ID; 
  }


  // resize vector
  nv.resize(l_ID + 1, NULL);
  env.resize(l_ID + 1, NULL);

  //printf("... %f secs\n", get_elapsed_msec() / 1000 );

  reset_and_start_timer();
  //printf("Rank %d: Adding edges", rank);

  // loop over edges
  for (int i = 0; i < edges.size(); i++) {
    int x = edges[i].x;
    int y = edges[i].y;

    // If x, y both external, skip
    if ( parts[x-1].part != rank && parts[y-1].part != rank) 
      continue;

    // If x internal, y external
    if (parts[x-1].part == rank && parts[y-1].part != rank) {
      Node *A = get_internal_node(x, parts[x-1].degree);
      exNode *B = get_external_node(y, parts[y-1].degree, parts[y-1].part);

      A->externals.push_back(B);
    }

    // If x internal, y external
    if (parts[x-1].part != rank && parts[y-1].part == rank) {
      Node *B = get_internal_node(y, parts[y-1].degree);
      exNode *A = get_external_node(x, parts[x-1].degree, parts[x-1].part);

      B->externals.push_back(A);
    }

    // If x, y both internal 
    if ( parts[x-1].part == rank && parts[y-1].part == rank) { 
      Node *A = get_internal_node(x, parts[x-1].degree);
      Node *B = get_internal_node(y, parts[y-1].degree);

      A->internals.push_back(B);
      B->internals.push_back(A);
    }
  }

  //printf("... %f secs\n", get_elapsed_msec() / 1000 );
  return;
}

void write_credits_to_file(string file_name) 
{
  FILE *output_fp = fopen(file_name.c_str(), "w");

  // skip node 0 
  for (int i = 1; i < nv.size(); i++) {
    if (nv[i] == NULL) {
      continue;
    }

    fprintf(output_fp, "%d\t%d", nv[i]->ID, nv[i]->degree);

    // skip round 0
    for (int j = 1; j < nv[i]->credit.size(); j++) {
      fprintf(output_fp, "\t%.6f", nv[i]->credit[j]);
    }
    fprintf(output_fp, "\n");

  }
  fclose(output_fp);

  return;
}

// Need a better way to do this, seg faults on ix-dev
void cleanup() 
{
  for (int i = 0; i < nv.size(); i++) {
    if (nv[i] != NULL) delete nv[i];
  }

  for (int i = 0; i < env.size(); i++) {
    if (env[i] != NULL) delete env[i];
  }
}

void verify() 
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  //printf("Rank %d: Verifying!\n", rank);

  double credit_sum = 0;
  int node_sum = 0;

  for (int i = 0; i < nv.size(); i++) {
    if (nv[i] != NULL) {
      credit_sum += nv[i]->credit.back();
      node_sum++;
    }
  }

  //printf("Rank %d: local_credit_sum: %f, local_node_sum: %d\n", 
  //    rank, credit_sum, node_sum);

  // Master adds up
  double master_credit_sum;
  MPI_Reduce(&credit_sum, &master_credit_sum, 1, 
      MPI_DOUBLE, MPI_SUM, 
      MASTER, MPI_COMM_WORLD);
  int master_node_sum;
  MPI_Reduce(&node_sum, &master_node_sum, 1, 
      MPI_INT, MPI_SUM, 
      MASTER, MPI_COMM_WORLD);


  if (rank == MASTER) {
    if ( fabs(master_credit_sum - master_node_sum ) > 1) {
      printf("MASTER: Verification Failed!\n");
      printf("MASTER: master_credit_sum: %f, master_nodes: %d\n", 
          master_credit_sum, master_node_sum); 
    }
    else 
      printf("MASTER: Verification Successful!\n");
  }

  return;
}
