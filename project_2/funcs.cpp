#include "funcs.h"
extern int start_time;
extern int random_token_id;

int Configure::messages_remaining() 
{
  int rv = 0;

  for (int i = 0; i < message_sent.size(); i++) {
    rv += 1 - message_sent[i];
  }

  return rv;
}

int Configure::print() 
{
	fprintf(stderr, "config file:\t%s\n", config_file.c_str());
	fprintf(stderr, "input file:\t%s\n", input_file.c_str());
	fprintf(stderr, "output file:\t%s\n", output_file.c_str());
	fprintf(stderr, "\n");

	fprintf(stderr, "min port:\t%d\n", min_port);
	fprintf(stderr, "max port:\t%d\n", max_port);
	fprintf(stderr, "my port:\t%d\n", my_port);
	fprintf(stderr, "\n");

	fprintf(stderr, "join time:\t%d\n", join_time);
	fprintf(stderr, "leave time:\t%d\n", leave_time);
	fprintf(stderr, "\n");

	fprintf(stderr, "Messages:\n");
	for (int i = 0; i < messages.size(); i++) {
		fprintf (stderr, "  [%02d:%02d] \t\"%s\"\n", 
				message_times[i] / 60, message_times[i] % 60, messages[i].c_str());
	}
	fprintf(stderr, "\n");

	return 0;
}

int Configure::parse_args(int argc, char **argv) 
{
	if (argc < 3) {
		fprintf(stderr,  "usage: ring -c config_file -i input_file -o output_file\n");
		return 0;
	}

	// get file names
	int opt, c = 0, i = 0, o = 0;
	while ((opt = getopt(argc, argv, "c:i:o:")) != EOF)
	{
		switch(opt)
		{
			case 'c': c = 1; config_file = std::string(optarg); break;
			case 'i': i = 1; input_file = std::string(optarg); break; 
			case 'o': o = 1; output_file = std::string(optarg); break; 
			case '?': return 0;
			default: abort();
		}
	}

  if (!c) config_file = "";
  if (!i) input_file = "";
  if (!o) output_file = "";

	return 1;
}

// read input and configure files
int Configure::read_files() 
{
	// read from config file
	std::ifstream cs;
	cs.open(config_file);
	if (!cs.is_open()) {
		perror("Config file error"); 
		return 0;
	}

	std::string tmp;
	char separator;	
	int min, sec;

	cs >> tmp >> min_port >> separator >> max_port;
	cs >> tmp >> my_port;
	cs >> tmp >> min >> separator >> sec;
	join_time = min*60 + sec;

	cs >> tmp >> min >> separator >> sec;
	leave_time = min*60 + sec;

	cs.close();

	// read from input file
	std::ifstream is;
	is.open(input_file);
	if (!is.is_open()) {
		perror("Input file error"); 
		return 0;
	}

	std::string line;

	// get line from file
	while (getline(is, line)) {

		// turn into stringstream
		std::stringstream ss(line);

		// parse time from line
		ss >> min >> separator >> sec;
		message_times.push_back(min*60 + sec);

		// parse message from rest of line
		std::string m;
		getline(ss, m);
		m.erase(0, 1);
		messages.push_back(m);
		message_sent.push_back(0);
	}

	is.close();

	num_messages = messages.size();
  
  // set up output file stream
  if (output_file != "")
    output_stream = fopen(output_file.c_str(), "w+");
  else
    output_stream = stdout;


	return 1;
}

int udp_send(int my_port, int rem_port, struct udp_message udp_m) 
{
	struct sockaddr_in myaddr, remaddr;
	int fd, i, slen=sizeof(remaddr);
	const char *server = "127.0.0.1";

	// create a UDP socket
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	// bind it to all local addresses and pick any port number
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY); // should try to make this local 
	myaddr.sin_port = my_port;

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	// now define remaddr, the address to whom we want to send messages
	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(rem_port);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		return 0;
	}

	// send message
	if (sendto(fd, &udp_m, sizeof(udp_m), 0, (struct sockaddr *)&remaddr, slen)==-1) {
		perror("sendto");
		return 0;
	}

	close(fd);
	return 1;
}

int udp_recv(int my_port, struct udp_message &udp_m, int ms_timeout) 
{
	struct sockaddr_in myaddr;	
	struct sockaddr_in remaddr;	
	socklen_t addrlen = sizeof(remaddr);		
	int recvlen;
	int fd;

	// create a UDP socket
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	// set a timeout for recv a message
	struct timeval read_timeout;
	read_timeout.tv_sec = ms_timeout / 1000;
	read_timeout.tv_usec = (ms_timeout % 1000) * 1000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	// bind the socket to any valid IP address and a specific port
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(my_port);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

	// recieve message
	recvlen = recvfrom(fd, &udp_m, sizeof(udp_m), 0, (struct sockaddr *)&remaddr, &addrlen);

	close(fd);

  return recvlen;
}

int post_messages(int my_port, int next_hop, int &prev_hop, struct Configure &config)
{
  // keep track of Turn Around Time
  int TAT = 1000;
  int s_time, r_time;

  if (config.messages_remaining() == 0) return 0;

	for (int i = 0; i < config.messages.size(); i++) {
		std::string send_message = config.messages[i];

		// skip if message already sent, or if
		// timestamp for the message is ahead of the clock
		if (config.message_sent[i]) continue;
		if (config.message_times[i] > get_time()) continue;

		// check message size
		if (send_message.size() > BUFSIZE) {
			fprintf(stderr, "message %d too large!\n", i);
			exit(0);
		}

		// construct message to post 
		struct udp_message udp_post_m;
		udp_post_m.sender_port = my_port;
		udp_post_m.type = RELAY;
		strcpy(udp_post_m.message, send_message.c_str());

		// send message to next hop
		udp_send(my_port, next_hop, udp_post_m);
    s_time = get_time();

		fprintf(config.output_stream, "[%d:%02d]: post \"%s\" was sent\n",
				get_time() / 60, get_time() % 60, send_message.c_str());

		// construct message for return
		struct udp_message udp_return_m;

		// wait to get message from prev hop
		int rv = udp_recv(my_port, udp_return_m, 2*TAT);
    r_time = get_time();

    TAT = (r_time - s_time) * 1000;

		// check return message data
    if (rv == -1) {
      fprintf(config.output_stream, "[%d:%02d]: reply timeout\n",
          get_time() / 60, get_time() % 60);

      // stop posting, no one listening :(
      return 0;
    }

    // deal with discovery real quick
    if (udp_return_m.type == DISCOVERY) {
      handle_discovery_request(my_port, prev_hop, udp_return_m, config);
      continue;
    }


    if (udp_return_m.type != RELAY) {
      // ignore
      continue;
    }
    

    if (udp_return_m.sender_port != prev_hop) {
			fprintf(stderr, "Bad message port:\n  prev_hop: %d, sender_port: %d\n",
					prev_hop, udp_return_m.sender_port);
		}

		if ( !strcmp(send_message.c_str(), udp_return_m.message))  {
			fprintf(config.output_stream, 
          "[%d:%02d]: post \"%s\" was delivered to all successfully\n",
					get_time() / 60, get_time() % 60, udp_return_m.message);
			config.message_sent[i] = 1;
		}
		else {
			fprintf(stderr, "Message corrupted!\n  send:\"%s\"\n  recv:\"%s\"\n",
					send_message.c_str(), udp_return_m.message);
		}
	}

  return 0;
}

int pass_token(int my_port, int next_hop) {

	// once messages are done sending, pass on token
	struct udp_message udp_token_m;
	udp_token_m.sender_port = my_port;
	udp_token_m.type = TOKEN;
  sprintf (udp_token_m.message, "%d", random_token_id);

	udp_send(my_port, next_hop, udp_token_m);
  
  return 0;
}

int relay_message(int my_port, int next_hop, char *message)
{
	// construct message to relay
	struct udp_message udp_relay_m;
	udp_relay_m.sender_port = my_port;
	udp_relay_m.type = RELAY;
	strcpy(udp_relay_m.message, message);

	udp_send(my_port, next_hop, udp_relay_m);

	return 0;
}

void start_timer() 
{
  start_time = time(0);
}
  
int get_time() 
{
  return time(0) - start_time;
}

int calc_prev_hop(int my_port, int x, int y) {

  if (x == my_port || y == my_port) {
    fprintf(stderr, "ERROR, prev_hop matches my_port)\n");
    return -1;
  }

  // same side
  if ( (x > my_port && y > my_port) || (x < my_port && y < my_port) )
    return std::max(x, y);

  // opposite sides
  else
    return std::min(x, y);
}

int calc_next_hop(int my_port, int x, int y) 
{
  if (x == my_port || y == my_port) {
    fprintf(stderr, "ERROR, prev_hop matches my_port)\n");
    return -1;
  }

  // same side
  if ( (x > my_port && y > my_port) || (x < my_port && y < my_port) )
    return std::min(x, y);

  // opposite sides
  else
    return std::max(x, y);
}

int handle_discovery_request(int my_port, int &prev_hop, 
    struct udp_message udp_m, struct Configure config) 
{

  // update prev hop
  if (prev_hop == -1) {
    prev_hop = udp_m.sender_port;
    fprintf(config.output_stream, "[%d:%02d]: previous hop is changed to client [%d]\n",
        get_time() / 60, get_time() % 60, udp_m.sender_port);
  }
  else if (prev_hop == udp_m.sender_port) {
    // do nothing
  }
  else {
    prev_hop = calc_prev_hop(my_port, prev_hop, udp_m.sender_port);
    if (prev_hop == udp_m.sender_port) 
      fprintf(config.output_stream, "[%d:%02d]: previous hop is changed to client [%d]\n",
          get_time() / 60, get_time() % 60, udp_m.sender_port);
  }

  // respond to request
  struct udp_message udp_resp_m;
  udp_resp_m.sender_port = my_port;
  udp_resp_m.type = DISCOVERY;

  udp_send(my_port, udp_m.sender_port, udp_resp_m);
  
  return 0;
}

int discovery(int my_port, int &next_hop, int &prev_hop, 
    struct Configure config, int &token)
{

	// get a starting check point
	int check_port;

	if (my_port == config.max_port)
		check_port = config.min_port;
	else
		check_port = my_port + 1;

	// keep looking until we find a next hop
	while (next_hop == -1) {

    // check to see if we can quit
    if (get_time() > config.leave_time && config.messages_remaining() == 0) {
      fprintf(config.output_stream, "[%d:%02d] Depart the ring\n", get_time() / 60, get_time() % 60);
      exit(0);
    }

		// construct discovery message
		struct udp_message udp_disc_m;
		udp_disc_m.sender_port = my_port;
		udp_disc_m.type = DISCOVERY;

		fprintf(stderr, "dicovery! checking port %d\n", check_port);

		// send discovery message to check_port
		udp_send(my_port, check_port, udp_disc_m);

		// construct response message
		struct udp_message udp_resp_m;

		while (udp_recv(my_port, udp_resp_m, 200) != -1) { // keep looking if not timeout

			// unrelated request
			if (udp_resp_m.type != DISCOVERY) {
				// ignore
        if (udp_resp_m.type == TOKEN) { 
          fprintf(config.output_stream, "[%d:%02d]: token [%d] was received\n",
              get_time() / 60, get_time() % 60, random_token_id);

          token = 1;
        }
				continue;
			}
			// outside discovery request
			if (udp_resp_m.sender_port != check_port) {

        // respond and update prev hop
				handle_discovery_request(my_port, prev_hop, udp_resp_m, config);
			}

			// response to our recovery request
			if (udp_resp_m.sender_port == check_port) {

				// update next hop
				if (next_hop == -1) {
					next_hop = udp_resp_m.sender_port;
					fprintf(config.output_stream, "[%d:%02d]: next hop is changed to client [%d]\n",
							get_time() / 60, get_time() % 60, next_hop);
				}
				else if (next_hop == udp_resp_m.sender_port) {
					// do nothing
				}
				else {
					next_hop = calc_next_hop(my_port, next_hop, udp_resp_m.sender_port);
					if (next_hop == udp_resp_m.sender_port)
						fprintf(config.output_stream, "[%d:%02d]: next hop is changed to client [%d]\n",
								get_time() / 60, get_time() % 60, next_hop);
				}

			}
		}

		// update check port
		check_port++;

		if (check_port > config.max_port) check_port = config.min_port;
	}

  return 0;
}

int start_election(int my_port, int next_hop, struct Configure config) 
{
  // create election message
  struct udp_message udp_elect_m;
  udp_elect_m.sender_port = my_port;
  udp_elect_m.type = ELECTION;
  sprintf(udp_elect_m.message, "%d", my_port);

  // send to my next hop
  udp_send(my_port, next_hop, udp_elect_m);

  fprintf(config.output_stream, "[%d:%02d]: started election, send election message to client [%d]\n",
      get_time() / 60, get_time() % 60, next_hop);

  return 0;
}
