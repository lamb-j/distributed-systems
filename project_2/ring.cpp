#include <string>
#include <cstdlib>

#include "funcs.h"

// global timer
int start_time;

// global token id
int random_token_id = -1;
int num_token_passes = 0;

int main(int argc, char **argv)
{
  // initialize random seed:
  srand (time(NULL));

  Configure config;
  config.parse_args(argc, argv);
  config.read_files();
  config.print();

  int my_port = config.my_port;

  int token = 0;

  int join_time = config.join_time;
  int leave_time = config.leave_time;

  start_timer();

  // wait to join the ring
  while (get_time() < join_time) {
    //fprintf(stderr, "[%d:%02d] waiting to join\n", get_time() / 60, get_time() % 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  int next_hop = -1;
  int prev_hop = -1;

  discovery(my_port, next_hop, prev_hop, config, token);

  // Random sleep [0-1]
  std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));

  // Initial Election
  start_election(my_port, next_hop, config);

  // In the Ring
  while (get_time() < leave_time || config.messages_remaining() > 0) {

    // if we don't have the token, wait for a message 
    if (!token) {
      fprintf(stderr, "[%d:%02d]: waiting for message\n", 
          get_time() / 60, get_time() % 60);

      // make struct to hold incomming messages
      struct udp_message udp_m;

      // check for message timeout
      // here we want a long timeout to make sure other procs can finish discovery
      // before declaring ring broken
      if (udp_recv(my_port, udp_m, 8000) == -1) {

        // ring broken, run discovery
        fprintf(config.output_stream, "[%d:%02d]: ring is broken\n", 
            get_time() / 60, get_time() % 60); 

        // reset hops
        next_hop = -1;
        prev_hop = -1;

        // random weight, so that discoveries are out of phase
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));
        discovery(my_port, next_hop, prev_hop, config, token);

        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));
        start_election(my_port, next_hop, config);

        continue;
      }
      std::string s;
      if (udp_m.type == BADTYPE) s = "BADTYPE";
      if (udp_m.type == TOKEN) s = "TOKEN";
      if (udp_m.type == DISCOVERY) s = "DISCOVERY";
      if (udp_m.type == RELAY) s = "RELAY";
      if (udp_m.type == ELECTION) s = "ELECTION";
      if (udp_m.type == LEADER) s = "LEADER";
      //fprintf(stderr, "Message recv\n  type: %s\n  sender_port:%d\n  content:%s\n", 
      //    s.c_str(), udp_m.sender_port, udp_m.message);

      // Discovery message
      // Accept from anyone
      if (udp_m.type == DISCOVERY) {
        handle_discovery_request(my_port, prev_hop, udp_m, config);
      }

      // Election message 
      // Accept only from prev_hop
      if (udp_m.type == ELECTION && udp_m.sender_port == prev_hop) {

        // check and update current max
        int max_ID;
        sscanf(udp_m.message, "%d", &max_ID);

        if (my_port == max_ID) {

          // construct message to continue
          struct udp_message udp_lead_m;
          udp_lead_m.sender_port = my_port;
          udp_lead_m.type = LEADER;
          sprintf(udp_lead_m.message, "%d", my_port);

          // send leader message
          udp_send(my_port, next_hop, udp_lead_m);
        }

        if (my_port != max_ID) {
          max_ID = std::max(my_port, max_ID);

          // construct message to continue
          struct udp_message udp_elect_m;
          udp_elect_m.sender_port = my_port;
          udp_elect_m.type = ELECTION;
          sprintf(udp_elect_m.message, "%d", max_ID);

          // send to my next hop
          udp_send(my_port, next_hop, udp_elect_m);

          // print appropriate message
          if (my_port < max_ID) {
            fprintf(config.output_stream, "[%d:%02d]: relayed election message, leader: %d\n",
                get_time() / 60, get_time() % 60, max_ID);

          }

          if (my_port > max_ID) { 
            // do nothing 
          //}
            fprintf(config.output_stream, "[%d:%02d]: relayed election message, replaced leader\n",
                get_time() / 60, get_time() % 60);
				  }
        }

      }

      // Leader message
      if (udp_m.type == LEADER) {
          
        // check leader ID 
        int leader_ID;
        sscanf(udp_m.message, "%d", &leader_ID);
        
        if (leader_ID == my_port) {
          fprintf(config.output_stream, "[%d:%02d]: leader selected\n", get_time() / 60, get_time() % 60);

          random_token_id = rand() % 10000;

          fprintf(config.output_stream, "[%d:%02d]: new token generated [%d]\n", 
              get_time() / 60, get_time() % 60, random_token_id);

          token = 1;
        }

        if (leader_ID > my_port) {
          // construct message to continue
          struct udp_message udp_lead_m;
          udp_lead_m.sender_port = my_port;
          udp_lead_m.type = LEADER;
          sprintf(udp_lead_m.message, "%d", leader_ID);

          udp_send(my_port, next_hop, udp_lead_m);
        }

        if (leader_ID < my_port) {
          // ignore
        }
      }

      // Relay message
      if (udp_m.type == RELAY) {

        // only relay from previous hop
        if (udp_m.sender_port == prev_hop) {
          relay_message(my_port, next_hop, udp_m.message);
          fprintf(config.output_stream, "[%d:%02d]: post \"%s\" from client [%d] was relayed\n",
              get_time() / 60, get_time() % 60, udp_m.message, prev_hop);
        }
      }

      // Token message
      if (udp_m.type == TOKEN) {

        // only accept token from previous hop
        if (udp_m.sender_port == prev_hop) {
          
          int new_random_token_id;
          sscanf(udp_m.message, "%d", &new_random_token_id);

          if (random_token_id != new_random_token_id) {
            random_token_id = new_random_token_id;

            fprintf(config.output_stream, "[%d:%02d]: token [%d] was received\n", 
                get_time() / 60, get_time() % 60, random_token_id);

            num_token_passes = 0;
            //printf("num_token_passes: %d\n", num_token_passes);
          }

          token = 1;
        }
      }

      // default
      if (udp_m.type == BADTYPE) {
        fprintf(config.output_stream, "[%d:%02d]: BAD TYPE\n", 
            get_time() / 60, get_time() % 60); 
      }

    } // exit if (!token)

    // if we have the token, post our messages
    if (token) {

      post_messages(my_port, next_hop, prev_hop, config);

      // wait a second before sending the token, so other process has a chance
      // to handle discovery requests
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      pass_token(my_port, next_hop);
    
      // only print the first time we pass this token
     if (num_token_passes == 0)
        fprintf(config.output_stream, "[%d:%02d]: token [%d] was sent to client [%d]\n", 
            get_time() / 60, get_time() % 60, random_token_id, next_hop);

      num_token_passes++;
     token = 0;
    }

  } // exit while

  // Exit the Ring
  fprintf(config.output_stream, "[%d:%02d] Depart the ring\n", get_time() / 60, get_time() % 60);

  // close the output stream
  fclose(config.output_stream);
  return 0;
}
