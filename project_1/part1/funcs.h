#include <cstdio>
#include <cmath>
#include <cstdlib>

#include <vector>
#include <iostream>
#include <fstream>

#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

using namespace std;

struct int2
{
    int x,y;
};

BOOST_FUSION_ADAPT_STRUCT(int2, (int, x)(int, y))
  
  typedef std::vector<int2> data_t;
    
extern data_t data;

class Node {

  public:
    int ID;
    int degree;

    vector <double> credit;
    vector <Node *> neighbors;

    // functions
    Node(int id);
    void print();

    void add_neighbor(Node *N);
    void update(int round);
};

// Independant functions
Node *get_node(int ID);
void add_edge(Node *A, Node *B);
void round_update(int round);

void read_edges_from_file(string file_name);
void write_credits_to_file(string file_name);

void cleanup(); 
void verify(); 

int parse(string file_name);
