//
// Created by mg on 16.06.2021.
//
#include <map>
#include "monitor_distributed.h"
#include "monitor_message.h"


namespace zmq_monitor{
    typedef std::pair<std::string,zmq_monitor::monitor_distributed<int>*> app_server_map_pair;
    typedef std::map<std::string, monitor_distributed< int>*> monitor_map_t;
    class app_server{
    private:

        size_t buffer_size;
        char *buffer;
        static void procedure(app_server* server, void(message_handler)(monitor_message<int>*, app_server*));
        std::thread th;
        zmqpp::context ctx;
        mutable std::mutex map_mtx;
    public:
        monitor_map_t monitor_map{};
        explicit app_server(const std::string &addr, size_t buf_s = 16384);
        bool nonstop;
        explicit app_server(const std::string &addr, void (message_handler)(monitor_message<int> *, app_server*), size_t buf_s = 16384);
        std::shared_ptr<zmqpp::socket> server_socket;
        ~app_server();
        monitor_map_t *lock_map();
        void unlock_map();

        void init(void (message_handler)(monitor_message<int> *, app_server*));
    };
}


