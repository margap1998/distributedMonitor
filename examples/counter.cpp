
#include "../monitor/app_server.h"
#include "../monitor/monitor_message.h"
#include <unistd.h>
#include <thread>
void message_handler(monitor_message<int>*, zmq_monitor::app_server*);
struct counter{
    char buf[10]{};
    int n{0};
    void add(int a){ n+=a;}
    void sub(int a){ n-=a;}
};
int main(int argc, const char *argv[] ) {
    auto addr = std::string(argv[2]);
    zmq_monitor::app_server appServer(addr);
    counter q;
    zmq_monitor::monitor_distributed<counter> abcd(q, argv[1],"abcd");
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
    while (appServer.nonstop){
        auto cnt = abcd.get_object_for_change();
        cnt.buf[9] = 1;
        printf("Otrzymany stan %s : %d\n",abcd.get_identifier(), cnt.n);
        cnt.add(1);

        if(strcmp(abcd.get_identifier(),"abcd1")==0){
            printf("abcd1 sleeps\n");
            cnt = abcd.wait();
        }

        printf("Wysyłany stan %s : %d\n",abcd.get_identifier(), cnt.n);
        abcd.set_object_after_change(cnt,1);
        sleep(1);
    }
    appServer.nonstop = false;
    return 0;
}

void message_handler(monitor_message<int>*, zmq_monitor::app_server*){
    printf("XD\n");
}