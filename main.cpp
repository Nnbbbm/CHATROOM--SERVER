#include <iostream>

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h> // 添加pthread头文件
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <unordered_set>
#include <unordered_map>
#include <sqlite3.h>
#include <string>

#include "MSG.pb.h"
#include "net.h"
#include "room.h"
#include "pipe.h"

using namespace std; //

const int num_pipe = 4;//how much pipe(how much room(how much process))

int pids[num_pipe] = {0};//the pid of each process
int pipes[num_pipe] = {0};//the num of each pipe is the num of each room
std::unordered_map<int,int> map_pipes;//first is room num(pipe num),secend is how much poeple in this room

int sock_listen = 0;//this socket will listen to connect

fd_set sets;//this sets are interested: 1:sock_listen 2:all pipe 3:all client that have not enter room
int sets_max = 0;


std::unordered_set<std::string> set_login;//this set collect all client's name that had log in(name is id,every client have it's own name) 
std::unordered_map<int,std::string> map_login;//first: client's socket, second:client's name

void update_room_num(){//send to every client: 1:room number 2:how much people in this room 
    for(int i=0;i<1024;i++){
        if(!FD_ISSET(i,&sets)||i==sock_listen||map_pipes.find(i)!=map_pipes.end()){
            continue;
        }

        string s;
        for(pair<int,int> p:map_pipes){
            s+=to_string(p.first)+"|"+to_string(p.second)+"|";
        }

        MSG *msg = new MSG;
        msg->set_type(TYPE::ROOM_NUM);
        msg->set_data(s);
        send_MSG(i, msg);
        delete msg;
    }
}

void signalHandler(int signal)//when you enter:ctrl+c,will kill all son process and exit
{
    if (signal == SIGINT)
    {
        // 发送终止信号给子进程
        for (int i = 0; i < num_pipe; i++)
        {
            kill(pids[i], SIGTERM);
        }
        // 等待子进程退出
        for (int i = 0; i < num_pipe; i++)
        {
            waitpid(pids[i], NULL, 0);
            close(pipes[i]);
        }
        // 主进程退出
        exit(0);
    }
}

// 1:create some thread
// 2:create some process
// 3:deal with pipes from processes(use select)
int main()
{
    signal(SIGINT, signalHandler);
    
    sock_listen = ini_sock_listen();

    FD_SET(sock_listen, &sets);
    sets_max = sock_listen;

    for (int i = 0; i < num_pipe; i++)
    {
        int fd[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
        int pid = fork();
        if (pid < -1)
        {
            cout << "fork err" << endl;
        }

        if (pid == 0)
        {
            close(fd[1]);
            process(fd[0]);
            close(fd[0]);
        }
        if (pid > 0)
        {
            pids[i] = pid;
            pipes[i] = fd[1];
            std::cout<<"Room: "<<fd[1]<<std::endl;
            map_pipes.insert(pair<int,int>(fd[1],0));
            close(fd[0]);
        }
    }
    for (int i = 0; i < num_pipe; i++)
    {
        sets_max = std::max(sets_max, pipes[i]);
        FD_SET(pipes[i], &sets);
    }

    while (true)
    {
        fd_set sets_once(sets);
        int sets_max_once = sets_max;

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        select(sets_max_once + 1, &sets_once, nullptr, nullptr, &timeout);
        for (int i = 0; i < sets_max_once + 1; i++)
        {
            if (!FD_ISSET(i, &sets_once))
            {
                continue;
            }

            if (i == sock_listen)
            { // new connect
                int sock_client = accept(sock_listen, nullptr, nullptr);
                cout << "client connect!: " << sock_client << endl;

                FD_SET(sock_client, &sets);
                sets_max = max(sets_max, sock_client);

                continue;
            }
            if (map_pipes.find(i) != map_pipes.end())
            {

                int int_from_pipe = 0;
                read(i, &int_from_pipe, sizeof(int_from_pipe));

                // int_from_pipe>=0 mean who leave room
                // int_from_pipe<0 mean control signal
                if (int_from_pipe >= 0)
                {                
                    // int_from_pipe is who leave room

                    std::cout << "client back main: " << int_from_pipe << std::endl; //main is hall
                    FD_SET(int_from_pipe, &sets);
                    sets_max = std::max(sets_max, int_from_pipe);
                    map_pipes[i]--;
                    update_room_num();

                    
                }else if (int_from_pipe == -1)//poeple--
                { // int_from_pipe is control signal
                  //...
                    // std::cout<<"client quit from room"<<std::endl;
                    map_pipes[i]--;
                    update_room_num();

                    int who_quit=0;
                    read(i,&who_quit,sizeof(int));

                    if (map_login.find(who_quit) != map_login.end()){
                        set_login.erase(map_login.find(who_quit)->second); // if map have,set mast have
                        map_login.erase(who_quit);
                    }



                }else if (int_from_pipe == -2)
                { // int_from_pipe is control signal
                  //...
                  std::cout<<"int_from_pipe == -2"<<std::endl;
                }else{
                    std::cout<<"int_from_pipe == -wrong"<<std::endl;
                }

                continue;
            }
            int ifSocket = 1;
            MSG *msg = recvMSG(i, ifSocket);
            if (!ifSocket || msg->type() == TYPE::CLIENT_QUIT)
            {
                std::cout << "main: client quit!: " << i << std::endl;

                if(map_login.find(i)!=map_login.end()){
                    set_login.erase(map_login.find(i)->second);//if map have,set mast have
                    map_login.erase(i);
                }


                close(i);
                FD_CLR(i, &sets);
            }
            else if (msg->type() == TYPE::TEXT_DATA)
            {
                cout << "main: " << msg->data() << endl;
            }
            else if (msg->type() == TYPE::CLIENT_ENTER_ROOM)
            {

                int room_num = std::stoi(msg->data());

                map_pipes[room_num]++;
                update_room_num();

                write(room_num, &i, sizeof(int)); // send main process's client fd(if want to send a control signal,please send <0)
                send_fd_to_pipe(room_num, i);     // let that process get a new fd that can use
                FD_CLR(i, &sets);

                write_string_to_pipe(room_num,msg->from());
            }
            else if (msg->type() == TYPE::CLIENT_LEAVE_ROOM)
            {
                cout << "you are not in room now!" << endl;
            }
            else if (msg->type() == TYPE::CLIENT_LOGIN)
            {
                // cout << "CLIENT_LOGIN" << endl;
                // cout << "login: " << msg->data() << endl;
                std::string s = msg->data();
                size_t pos = s.find('|');

                // 提取分割后的字符串
                std::string username = s.substr(0, pos);
                std::string password = s.substr(pos + 1);

                // // 打印分割后的结果
                // std::cout << "First: " << username << std::endl;
                // std::cout << "Second: " << password << std::endl;

                sqlite3 *db;
                sqlite3_open("/home/pxl/my_data/sqlite/chatroom.db", &db);

                std::string query = "SELECT COUNT(*) FROM users WHERE username='" + username + "' AND password='" + password + "';";

                sqlite3_stmt *statement;
                sqlite3_prepare_v2(db, query.c_str(), -1, &statement, nullptr);
                sqlite3_step(statement);

                int count = sqlite3_column_int(statement, 0);
                if (count > 0)
                {
                    if(set_login.find(username)!=set_login.end()){
                        MSG *msg_temp = new MSG();
                        msg_temp->set_type(TYPE::CLIENT_LOGIN_NO);
                        msg_temp->set_data("You have already log in！");
                        send_MSG(i, msg_temp);
                        delete msg_temp;
                    }else{

                        set_login.insert(username);

                        map_login.insert(pair<int,std::string>(i,username));

                        MSG *msg_temp = new MSG();
                        msg_temp->set_type(TYPE::CLIENT_LOGIN_YES);
                        send_MSG(i, msg_temp);
                        delete msg_temp;
                    }

                }
                else
                {
                    MSG *msg_temp = new MSG();
                    msg_temp->set_type(TYPE::CLIENT_LOGIN_NO);
                    msg_temp->set_data("Wrong username of password！");
                    send_MSG(i, msg_temp);
                    delete msg_temp;
                }

                sqlite3_finalize(statement);
                sqlite3_close(db);
            }else if(msg->type() == TYPE::ROOM_NUM){
                update_room_num();
            }

            delete msg;
        }
    }

    pthread_exit(nullptr);
    return 0;
}
