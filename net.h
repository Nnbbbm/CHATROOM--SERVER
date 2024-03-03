#pragma once
#include"MSG.pb.h"
int ini_sock_listen();
int recvData_fixedSize(int socket,char* buffer,int length_need);
int sendData_fixedSize(int fd,const void* buf,size_t size);
void send_MSG(int fd,MSG* msg);
MSG* recvMSG(int fd,int& ifSocket);