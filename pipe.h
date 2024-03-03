#pragma once
#include<sys/socket.h>
#include<unordered_map>

#include"MSG.pb.h"
static const int CONTROL_LEN = CMSG_LEN( sizeof(int) );

extern std::unordered_map<int,int> map_sonFD_mainFD;


void send_fd_to_pipe( int fd, int fd_to_send );
int recv_fd_from_pipe( int fd );

void write_MSG_to_pipe(int pipe,MSG* msg);
MSG* read_MSG_from_pipe(int pipe);

std::string read_data_from_pipe(int pipe_fd);
void write_string_to_pipe(int pipe_fd,std::string s);
