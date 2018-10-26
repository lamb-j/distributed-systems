#include <cstdio>
#include <cmath>
#include <cstdlib>

#include <vector>
#include <iostream>
#include <fstream>

#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "mpi.h"
#define MASTER 0
#define SLAVE 1

using namespace std;

struct int2
{
    int x, y;
};

struct int3
{
    int ID;
    int degree;
    int part;
};

BOOST_FUSION_ADAPT_STRUCT(int2, (int, x)(int, y))
BOOST_FUSION_ADAPT_STRUCT(int3, (int, ID)(int, degree)(int, part))

typedef std::vector<int2> data_i2;
typedef std::vector<int3> data_i3;

class exNode {
  public:
    int ID;
    int degree;
    int part;

    double credit;

    exNode(int ID, int degree, int part);

};

class Node {
  public:
    int ID;
    int degree;

    vector <double> credit;
    vector <Node *> internals;
    vector <exNode *> externals;

    // functions
    Node(int id, int dg);
    void print(FILE *ofstream);

    void update_internals(int round);
    void request_externals(int round, int target);
};

// Independant functions
Node *get_internal_node(int ID);
exNode *get_external_node(int ID);
void round_update(int round);

void read_from_files(string edge_file, string part_file);
void write_credits_to_file(string file_name);

void cleanup(); 
void verify(); 

int parse_i2(string file_name, data_i2 &data);
int parse_i3(string file_name, data_i3 &data);
