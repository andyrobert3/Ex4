#include "headsock.h"

void str_server(int socket_descriptor);
void compare_files(FILE *file1, FILE *file2);

const char * RECEIVED_FILE_NAME = "myUDPreceive.txt";
const char * ORIGINAL_FILE_NAME = "myfile.txt";

int main(void) {
  int socket_descriptor;
  struct sockaddr_in server_address;

  socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0); 
  if (socket_descriptor == -1) {
    printf("Error in creating socket\n");
    exit(1);
  }

  // Server address setup
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(MYUDP_PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_address.sin_zero), 8);

  // Socket bind()
  if (bind(socket_descriptor, (struct sockaddr *) &server_address, sizeof(struct sockaddr)) == -1) {
    printf("Error in binding socket\n");
    exit(1);
  }

  printf("Start receiving data!\n");
  while (1) {
    str_server(socket_descriptor);

    FILE *original = fopen(ORIGINAL_FILE_NAME, "r");
    FILE *received = fopen(RECEIVED_FILE_NAME, "r");

    if (original == NULL || received == NULL) {
      printf("Error in opening files: %s and/or %s\n", ORIGINAL_FILE_NAME, RECEIVED_FILE_NAME);
      exit(0);
    }

    // compare_files(original, received);
    fclose(original);
    fclose(received);
  }

  close(socket_descriptor);
  exit(0);
}

void str_server(int socket_descriptor) {
  FILE *file;

  int packet_sizes[NUMPACKETSIZES] = { 1 * DATALEN, 2 * DATALEN, 3 * DATALEN };
  int curr_packet_idx = 0;

  // Buffer size is at least file size limit
  int buffer[BUFSIZ * 100];
  int is_eof = 0; 
  
  int bytes_received = 0;
  int bytes_sent = 0;
  long total_bytes_received = 0;

  struct sockaddr_in client_address;
  unsigned int address_len = sizeof (struct sockaddr_in);

  struct ack_so ack; 

  // Agreed definition with client
  ack.num = 0;
  ack.len = 0;
  
  while (!is_eof) {
    int curr_packet_size = packet_sizes[curr_packet_idx];
    char recvs[curr_packet_size];

    bytes_received = recvfrom(socket_descriptor, &recvs, DATALEN * (curr_packet_idx + 1), 0, (struct sockaddr *)&client_address, &address_len);

    if (bytes_received == -1) {
      printf("Error when receiving from client\n");
      exit(1);
    } else {
      printf("Received %d bytes of data!\n", bytes_received);
    }
    
    // EOF
    if (recvs[bytes_received - 1] == '\0') {
      is_eof = 1;
      bytes_received--;
      printf("End of file reached!\n");
    } 

    memcpy(buffer + total_bytes_received, recvs, bytes_received);
    total_bytes_received += bytes_received; 

    bytes_sent = sendto(socket_descriptor, &ack, 2, 0, (struct sockaddr *)&client_address, address_len);
    if (bytes_sent == -1) {
      printf("Error when sending ACK\n");
      exit(1);
    } else {
      printf("ACK sent: %d bytes!\n", bytes_sent);
    }

    curr_packet_idx++;
    curr_packet_idx %= NUMPACKETSIZES;
    printf("size: %d\n", curr_packet_idx);
  }
 
  
  file = fopen(RECEIVED_FILE_NAME, "w");
  // Will never run as if file doesn't exist, it will create new file
  if (file == NULL) {
    printf("File doesn't exist\n");
    exit(0);
  }

  fwrite(buffer, 1, total_bytes_received, file);
  fclose(file);

  printf("File has ben received!\nTotal data received is %ld bytes.\n", total_bytes_received);
}

void compare_files(FILE *file1, FILE *file2){
   char ch1 = getc(file1);
   char ch2 = getc(file2);
   int error = 0, pos = 0, line = 1;

   while (ch1 != EOF && ch2 != EOF){
      pos++;
      if (ch1 == '\n' && ch2 == '\n'){
         line++;
         pos = 0;
      }
      if (ch1 != ch2) {
         error++;
        //  printf("Line Number : %d \tError"
        //  " Position : %d \n", line, pos);
      }
      ch1 = getc(file1);
      ch2 = getc(file2);
   }
   printf("Total Errors : %d\t", error);
}