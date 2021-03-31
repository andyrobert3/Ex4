#include "headsock.h"
#include <stdbool.h>

float str_client(FILE *file, int socket_descriptor, struct sockaddr *address, unsigned int address_len, long *total_packets_size);
float calc_time_interval(struct timeval *t1, struct timeval *t2);

int main(int argc, char **argv) {
  // Server host details
  struct hostent *host;
  // Internet address
  struct in_addr **address;
  // Socket descriptor that references client socket
  int socket_descriptor;
  // Socket address descriptor
  struct sockaddr_in server_address;
  // File to be sent
  FILE *file;

  char ** host_alias;
  long total_packets_size;

  char *file_name = "myfile.txt";
  
	if (argc != 2) {
		printf("Parameters do not match.");
	}

  host = gethostbyname(argv[1]);
  if (host == NULL) {
    printf("Error in parsing user second input: %s", host->h_name);
    exit(0);
  }

  // Server host information
  for (host_alias = host->h_aliases; *host_alias != NULL; host_alias++) 
		printf("Server host alias name is: %s", *host_alias);

  switch (host->h_addrtype) {
    // UDP/TCP 
    case AF_INET: 
      break;
    default: 
      printf("Unknown address type: %s\n", *host->h_addr_list);
      break;
  }

  address = (struct in_addr **)host->h_addr_list;
  socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);

  if (socket_descriptor == -1) {
    printf("Error in creating socket");
    exit(1);
  } 

  if ((file = fopen(file_name, "r+t")) == NULL) {
    printf("File: %s doesn't exist.\n", file_name);
    exit(1);
  }

  server_address.sin_family = AF_INET;
  // Convert network byte to host byte order
  server_address.sin_port = htons(MYUDP_PORT);

  // // Set server address to internet address
  memcpy(&(server_address.sin_addr.s_addr), *address, sizeof(struct in_addr));
  
  // Writes n zeroed bytes to string s
  bzero(&(server_address.sin_zero), 8); 

  float time_interval_ms = str_client(file, socket_descriptor, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in), &total_packets_size); 
  float data_rate = total_packets_size / time_interval_ms;

  printf("Transfer time: %.3f ms\nData sent: %ld bytes\nData rate: %.0f KB/s\n", time_interval_ms, total_packets_size, data_rate);

  exit(0);
}

/**
 * Sends file in chunks of DATALEN via UDP
 * Returns time_interval between start sending and done in seconds 
 */ 
float str_client(FILE *file, int socket_descriptor, struct sockaddr *server_address, unsigned int address_length, long *total_packets_size) {
  int packet_sizes[NUMPACKETSIZES] = { 1 * DATALEN, 2 * DATALEN, 3 * DATALEN };
  int packet_to_send_idx = 0;
  int packet_to_send_size;
  int bytes_sent;
  int bytes_received;
  
  char *buffer;
  long file_size, sent_file_size = 0;
  bool is_last_packet = false;

  float time_interval = 0.0;
  
  // Calculate time interval
  struct timeval send_time, receive_time;
  
  // ACK 
  struct ack_so ack;

  // Sets file position indicator pointed by stream
  fseek(file, 0, SEEK_END);
  // Obtains current value of file position pointed by stream
  file_size = ftell(file);
  // File position indicator pointed by stream to beginning of file
  rewind(file);
  
  // printf("File length is %d bytes\n", (int) file_size);
  // printf("Packet length is %d bytes\n", DATALEN);

  // Allocate memory to contain file
  buffer = (char *) malloc(file_size);
  if (buffer == NULL) {
    exit(2);
  }

  // Copy file into buffer
  fread(buffer, 1, file_size, file);
  // Indicate end byte file
  buffer[file_size] = '\0';

  gettimeofday(&send_time, NULL);

  while (sent_file_size <= file_size) {
    int remaining_file_len = file_size + 1 - sent_file_size;

    // Last packet    
    if (remaining_file_len <= packet_sizes[packet_to_send_idx]) {
      is_last_packet = true;
      packet_to_send_size = remaining_file_len;
    } else {
      // Current packet size
      packet_to_send_size = packet_sizes[packet_to_send_idx];
    }
    char packet_to_send[packet_to_send_size];

    // Copy remaining_file_len bytes from buffer to packet_sent
    memcpy(packet_to_send, buffer + sent_file_size, packet_to_send_size);
    bytes_sent = sendto(socket_descriptor, &packet_to_send, packet_to_send_size, 0, server_address, address_length);
    
    if (bytes_sent == -1) {
      printf("Error when sending packet\n");
      exit(1);
    } else {
      // printf("%d bytes sent successfuly!\n", bytes_sent);
    }

    if ((bytes_received = recvfrom(socket_descriptor, &ack, 2, 0, server_address, &address_length)) == -1) {
      printf("Error when receiving packet\n");
      exit(1);
    } else {
      // printf("ACK received\n");
    }

    // ack.num & ack.len defined in server
    if (ack.num != 0 || ack.len != 0) {
      printf("Error in transmission\n");
      exit(1);
    }

    remaining_file_len += packet_to_send_size;

    // multiple of DATALEN: 1 -> 2 -> 3 -> 1 -> ...
    packet_to_send_idx++;
    packet_to_send_idx %= NUMPACKETSIZES;
    
    sent_file_size += packet_to_send_size;
  }

  gettimeofday(&receive_time, NULL);
  *total_packets_size = file_size;

  time_interval = calc_time_interval(&receive_time, &send_time);

  return time_interval;
}

// Calculate time interval between timeval t1, t2 (provided t1 >= t2) in miliseconds
float calc_time_interval(struct timeval *t1, struct timeval *t2) {
  long seconds = t1->tv_sec - t2->tv_sec;
  long micro_seconds = t1->tv_usec - t2->tv_usec;

  return (seconds * 1000.0) + ( (float) micro_seconds / 1000.0 );
}

