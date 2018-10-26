#include "funcs.h"
#include "timing.h"
using namespace std;

extern vector <Node *> nv;

// Node class functions
Node::Node(int id) {
  ID = id;

  credit.push_back(1);
  degree = 0;
}

void Node::print()
{

  printf("ID: %d, degree: %d, credit: %f\n", ID, degree, credit.back());
  //printf("Neighbors:\n");
  //for (int i = 0; i < neighbors.size(); i++) {
  //  printf("  ID: %d\n", neighbors[i]->ID);
  //}
  //printf("\n");

}

void Node::add_neighbor(Node *N) 
{
  // check if node is already marked as neighbor
  for (int i = 0; i < neighbors.size(); i++) {
    if (N->ID == neighbors[i]->ID) {
      printf("REPEAT\n");
      return;
    }
  }

  // add the new neighbor and update degree
  neighbors.push_back(N);
  degree++;

  return;
}

// Update algorithm for a single node
void Node::update(int round) {

  double new_credit = 0;
  for (int i = 0; i < neighbors.size(); i++) {

    new_credit += neighbors[i]->credit[round - 1] / neighbors[i]->degree;
  }

  credit.push_back(new_credit);

  return;
}


// Independant functions
Node *get_node(int ID) {

  Node *rv;

  if (nv[ID] == NULL) {
    rv = new Node(ID);
    nv[ID] = rv;
  }
  else {
    rv = nv[ID];
  }

  return rv;
}

void add_edge(Node *A, Node *B) {

  //A->add_neighbor(B);
  //B->add_neighbor(A);

  A->neighbors.push_back(B);
  B->neighbors.push_back(A);

  A->degree++;
  B->degree++;

  return;
}

void round_update(int round) {

  printf("ROUND:%d\n", round);

  // calculate new credit values
  for (int i = 0; i < nv.size(); i++) {
   
    if (nv[i] != NULL) 
      nv[i]->update(round);
  }

}

void read_edges_from_file(string file_name) 
{
  reset_and_start_timer();
  printf("Reading from file");

  // Read from file
#if 0
  ifstream ifs(file_name);
  vector <int> id_vec(istream_iterator<int>(ifs), {});
  ifs.close();

  printf("... %f secs\n", get_elapsed_msec() / 1000 );

  reset_and_start_timer();
  printf("Resizing vector");
  // find largest node ID
  int l_ID ;
  l_ID = 0;

  for (int i = 0; i < id_vec.size(); i++) {
    l_ID = id_vec[i] > l_ID ? id_vec[i] : l_ID; 
  }

  // resize vector
  nv.resize(l_ID + 1, NULL);

  printf("... %f secs\n", get_elapsed_msec() / 1000 );

  reset_and_start_timer();
  printf("Adding edges");
  // loop over edges
  for (int i = 0; i < id_vec.size(); i += 2) {
    Node *A = get_node(id_vec[i]);
    Node *B = get_node(id_vec[i + 1]);

    add_edge(A, B);
  }
#endif

#if 1
  // Uses boost library to parse file into data_t data
  parse(file_name);

  printf("... %f secs\n", get_elapsed_msec() / 1000 );

  reset_and_start_timer();
  printf("Resizing vector");
  // find largest node ID
  int l_ID ;
  l_ID = 0;

  for (int i = 0; i < data.size(); i++) {
    l_ID = data[i].x > l_ID ? data[i].x : l_ID; 
    l_ID = data[i].y > l_ID ? data[i].y : l_ID; 
  }

  // resize vector
  nv.resize(l_ID + 1, NULL);

  printf("... %f secs\n", get_elapsed_msec() / 1000 );

  reset_and_start_timer();
  printf("Adding edges");

  // loop over edges
  for (int i = 0; i < data.size(); i++) {
    Node *A = get_node(data[i].x);
    Node *B = get_node(data[i].y);

    add_edge(A, B);
  }

#endif
  printf("... %f secs\n", get_elapsed_msec() / 1000 );
  return;
}

void write_credits_to_file(string file_name) 
{
  FILE *output_fp = fopen(file_name.c_str(), "w");

  for (int i = 0; i < nv.size(); i++) {
    if (nv[i] == NULL) continue;

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

void cleanup() 
{
  printf("nv.size(): %lu\n", nv.size());
  for (int i = 0; i < nv.size(); i++) {
    if (nv[i] != NULL) delete nv[i];
  }
}

void verify() 
{
  double credit_sum = 0;
  double node_sum = 0;
  
  for (int i = 0; i < nv.size(); i++) {
    if (nv[i] != NULL) {
      credit_sum += nv[i]->credit.back();
      node_sum ++;
    }
  }

  if ( fabs(credit_sum - node_sum ) > 1) 
    printf("Verification Failed!\n");
  else 
    printf("Verification Successful!\n");

  printf("credit_sum: %f, nodes: %f\n", credit_sum, node_sum); 

  return;
}
