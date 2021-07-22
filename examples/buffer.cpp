
#include "../monitor/app_server.h"
#include <thread>
void message_handler(monitor_message<int>*, zmq_monitor::app_server*);
class buffer{
public:
    int index_in;
    int index_out;
    int b[10];
    int in_buffer;
    buffer(){
        index_out = 0;
        index_in = 0;
        in_buffer = 0;
    }
    int *to_send(){
        auto res = new int[14];
        res[0] = 10;
        res[1] = in_buffer;
        res[2] = index_in;
        res[3] = index_out;
        for (int i = 4; i<14; i++){
            res[i] = b[i-4];
        }
        return  res;
    }
};
int main(int argc, const char *argv[] ) {
    auto addr = std::string(argv[2]);
    zmq_monitor::app_server appServer(addr);
    zmq_monitor::monitor_distributed<int> abcd(argv[1],"abcd");

    auto wpis = std::pair<std::string,zmq_monitor::monitor_distributed<int>*>("abcd",(zmq_monitor::monitor_distributed<int>*)&abcd);
    std::string node(argv[3]);
    abcd.nodes.push_back(node);
    if (argc>4){
        for(int i=4; i<argc; i++){
            node = std::string(argv[i]);
            abcd.nodes.push_back(node);
        }
    }
    auto map = appServer.lock_map();
    map->insert(wpis);
    appServer.unlock_map();
    appServer.init(message_handler);
    int a = 0;
    while (appServer.nonstop){
        a = abcd.get_object_for_change();/*
        if(strcmp(abcd.get_identifier(),"abcd1")==0) {
            if(cnt.in_buffer >9){abcd.wait();}
            cnt.b[cnt.index_in] = a++;
            cnt.in_buffer++;
            cnt.index_in = (cnt.index_in+1) % 10;
        }else{
            if(cnt.in_buffer ==0){abcd.wait();}
            printf("%d\n",cnt.b[cnt.index_out]);
            cnt.in_buffer++;
            cnt.index_out = (cnt.index_out+1) % 10;
        }*/
        if(strcmp(abcd.get_identifier(),"abcd1")==0){
            printf("abcd1 sleeps\n");
            abcd.wait();
        }
        a++;
        abcd.set_object_after_change(a, 1);
    }
    appServer.nonstop = false;
    return 0;
}

void message_handler(monitor_message<int>*, zmq_monitor::app_server*){
    printf("XD\n");
}