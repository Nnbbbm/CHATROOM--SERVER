#include<sys/socket.h>
#include<iostream>
#include<unistd.h>
#include<unordered_map>


#include"pipe.h"
#include"MSG.pb.h"



void send_fd_to_pipe( int fd, int fd_to_send )
{
    struct iovec iov[1];
    struct msghdr msg;
    char buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_name    = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov     = iov;
    msg.msg_iovlen = 1;

    cmsghdr cm;
    cm.cmsg_len = CONTROL_LEN;
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA( &cm ) = fd_to_send;
    msg.msg_control = &cm;/*设置辅助数据*/
    msg.msg_controllen = CONTROL_LEN;

    sendmsg( fd, &msg, 0 );
}
/*接收目标文件描述符*/
int recv_fd_from_pipe( int fd )
{
    struct iovec iov[1];
    struct msghdr msg;
    char buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_name    = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov     = iov;
    msg.msg_iovlen = 1;

    cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg( fd, &msg, 0 );

    int fd_to_read = *(int *)CMSG_DATA( &cm );

    return fd_to_read;
}

void write_MSG_to_pipe(int pipe,MSG* msg){
    std::string s;
    msg->SerializeToString(&s);

    int datalength=s.size();
    write(pipe,&datalength,sizeof(int));
    write(pipe,s.data(),datalength);
}

MSG* read_MSG_from_pipe(int pipe){
    int datalength=0;
    read(pipe,&datalength,sizeof(int));

    std::string s;
    s.resize(datalength);
    read(pipe,s.data(),datalength);

    MSG* msg=new MSG();
    msg->ParseFromString(s);
    return msg;

}

std::string read_data_from_pipe(int pipe_fd){
    int length = 0;
    read(pipe_fd, &length, sizeof(int));


    std::string data;
    data.resize(length);

    read(pipe_fd, data.data(), length);

    return data;
}

void write_string_to_pipe(int pipe_fd,std::string s){
    int length=s.size();
    write(pipe_fd,&length,sizeof(int));

    write(pipe_fd,s.data(),length);

}