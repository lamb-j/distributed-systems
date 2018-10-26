#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <ctime>

#include <thread>
#include <chrono>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 256

#define BADTYPE   0
#define TOKEN     1
#define DISCOVERY 2
#define RELAY     3
#define ELECTION  4
#define LEADER    5

class Configure {
	public:
		int min_port;
		int max_port;
		int my_port;

		int join_time;
		int leave_time;

    int num_messages;

    std::vector <std::string> messages;
    std::vector <int> message_times;
    std::vector <int> message_sent;

    std::string config_file;
    std::string input_file;
    std::string output_file;

    FILE *output_stream;

		int parse_args(int argc, char **argv);
    int read_files();
    int print();
    int messages_remaining();
};

struct udp_message
{
  int sender_port;
  int type;
  char message[BUFSIZE];
};

int udp_send(int my_port, int rem_port, struct udp_message udp_m);
int udp_recv(int my_port, struct udp_message &udp_m, int ms_timeout); 

int post_messages(int my_port, int next_hop, int &prev_hop, struct Configure &config);
int relay_message(int my_port, int next_hop, char *message);
int pass_token(int my_port, int next_hop);

int calc_prev_hop(int my_port, int x, int y);
int calc_next_hop(int my_port, int x, int y);

int discovery(int my_port, int &next_hop, int &prev_hop, 
    struct Configure config, int &token);
int start_election(int my_port, int next_hop, struct Configure config);

int handle_discovery_request(int my_port, int &prev_hop, struct udp_message udp_m,
    struct Configure config);
//int handle_election_message(int my_port, int next_hop, struct udp_message udp_m);
//int handle_leader_message(int my_port, int next_hop, struct udp_message udp_m);

void start_timer();
int get_time();
