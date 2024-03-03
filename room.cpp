#include "room.h"
#include<pthread.h>
#include<unistd.h>
#include<iostream>
#include<sys/select.h>
#include<queue>
#include<unordered_map>
#include<vector>


#include"MSG.pb.h"
#include"net.h"
#include"pipe.h"


int pipe_fd=-1;


fd_set sets_users;
int sets_max_users=0;


std::unordered_map<int,int> map_sonFD_mainFD;

std::unordered_map<int,std::string> map_names;

void update_names(){
    std::string s;

    for(std::pair<int,std::string> p:map_names){
        s+=p.second+"|";
    }

    MSG *msg = new MSG;
    msg->set_type(TYPE::ROOM_NAMES);
    msg->set_data(s);

    for (int i = 0; i < sets_max_users + 1; i++){
        if (!FD_ISSET(i, &sets_users) || i == pipe_fd){
            continue;
        }
        send_MSG(i, msg);
    }

    delete msg;

}

void process(int pipe){//son process
    pipe_fd = pipe;
    FD_SET(pipe_fd,&sets_users);//listen to pipe
    sets_max_users=pipe_fd;

    while(true){

        fd_set sets_once(sets_users);
        int sets_max_users_once=sets_max_users;


        select(sets_max_users_once+1,&sets_once,nullptr,nullptr,nullptr);

        for(int i=0;i<sets_max_users_once+1;i++){
            if(!FD_ISSET(i,&sets_once)){
                continue;
            }
            
            if(i==pipe_fd){//if is pipe
                int int_from_pipe=0;
                read(pipe_fd,&int_from_pipe,sizeof(int));

                //int_from_pipe>=0 mean the fd that main process using
                //int_from_pipe<0 mean control signal 
                if(int_from_pipe>=0){//int_from_pipe is fd that main process using
                    int sock_client=recv_fd_from_pipe(pipe_fd);

                    FD_SET(sock_client,&sets_users);
                    sets_max_users=std::max(sets_max_users,sock_client);

                    map_sonFD_mainFD.insert(std::pair<int,int>(sock_client,int_from_pipe));

                    std::cout<<"client enter room!: "<<sock_client<<"  mainFD: "<<int_from_pipe<<std::endl;

                    std::string name=read_data_from_pipe(i);

                    map_names.insert(std::pair<int,std::string>(sock_client,name));

                    update_names();
                    
                }else if(int_from_pipe==-1){//int_from_pipe is control signal

                }
                continue;
            }

            //if is client
            int ifSocket=1;//can know client quit(client force quit,socket can't use)
            MSG* msg=recvMSG(i,ifSocket);//if force quit,recvMSG return nullptr

            if(!ifSocket||msg->type()==TYPE::CLIENT_QUIT){//1:client force quit,2:client tell server quit
                std::cout<<"room: client quit!: "<<i<<std::endl;





                int temp=-1;
                write(pipe_fd,&temp,sizeof(temp));

                auto it= map_sonFD_mainFD.find(i);
                int who_quit=it->second;
                write(pipe_fd,&who_quit,sizeof(who_quit));

                
                close(i);

                FD_CLR(i,&sets_users);
                map_sonFD_mainFD.erase(i);

                map_names.erase(i);
                update_names();
                continue;
            }

            if(msg->type()==TYPE::CLIENT_LEAVE_ROOM){
                auto it= map_sonFD_mainFD.find(i);
                int who_leave=it->second;
                write(pipe_fd,&who_leave,sizeof(who_leave));

                FD_CLR(i,&sets_users);
                map_sonFD_mainFD.erase(i);

                map_names.erase(i);
                update_names();
                continue;
            }

            if(msg->type()==TYPE::TEXT_DATA){
                for(int i=0;i<sets_max_users+1;i++){
                    if(!FD_ISSET(i,&sets_users)||i==pipe_fd){
                        continue;
                    }
                    send_MSG(i,msg);
                }
                continue;
            }

            if(msg->type()==TYPE::CLIENT_ENTER_ROOM){
                std::cout<<"in room already!"<<std::endl;
            }

            if(msg)
                delete msg;
        }
    }
    return;
}

