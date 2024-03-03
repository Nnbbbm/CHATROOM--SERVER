#include"net.h"
#include<netinet/in.h>
#include<string>
#include"MSG.pb.h"
int ini_sock_listen(){
    int sock_listen=socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in addr_bind;
    addr_bind.sin_family=AF_INET;
    addr_bind.sin_port=htons(12345);
    addr_bind.sin_addr.s_addr=htonl(INADDR_ANY);

    bind(sock_listen,(sockaddr*)&addr_bind,sizeof(addr_bind));
    listen(sock_listen,1024);
    return sock_listen;
}


void send_MSG(int fd,MSG* msg){
    std::string data;
    msg->SerializeToString(&data);

    int dataLength=data.size();

    int isSeng = send(fd,&dataLength,sizeof(int),0);//send length
    if(isSeng==-1){
        std::cout<<"send_MSG error!"<<std::endl;
        return;
    }
    isSeng = send(fd,data.data(),data.size(),0);
    if(isSeng==-1){
        std::cout<<"send_MSG error!"<<std::endl;
        return;
    }
}

MSG* recvMSG(int fd,int& ifSocket){
    int dataLength=0;
    int rec1 = recv(fd,(char*)&dataLength,sizeof(int),0);
    if(rec1==0){
        ifSocket=0;
        return nullptr;
    }
        
    std::string data;
    data.resize(dataLength);

    int rec2 = recv(fd,data.data(),dataLength,0);
    if(rec2==0){
        ifSocket=0;
        return nullptr;
    }
    MSG* msg=new MSG();
    msg->ParseFromString(data);
    return msg;
}