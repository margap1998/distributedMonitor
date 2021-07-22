//
// Created by mg on 16.06.2021.
//

#include "app_server.h"
#include "monitor_message.h"
#include "monitor_distributed.h"
namespace zmq_monitor{
    app_server::app_server(const std::string& addr, void(message_handler)(monitor_message<int>*, app_server*), size_t buf_s){
        buffer_size = buf_s;
        buffer = new char[buf_s];
        server_socket = std::make_shared<zmqpp::socket>(ctx,zmqpp::socket_type::reply);
        server_socket->bind(addr);
        nonstop = true;
        th = std::thread();
        init(message_handler);
    }
    app_server::app_server(const std::string& addr, size_t buf_s){
        th = std::thread();
        buffer_size = buf_s;
        buffer = new char[buf_s];
        server_socket = std::make_shared<zmqpp::socket>(ctx,zmqpp::socket_type::reply);
        server_socket->bind(addr);
        nonstop = true;
    }
app_server::~app_server(){
    if (server_socket != nullptr) server_socket->close();
    server_socket.reset();

    delete [] buffer;
}
void app_server::init(void(message_handler)(monitor_message<int>*, app_server*)){
    th = std::thread(procedure,this,message_handler);
    th.detach();
}
    void app_server::procedure(app_server *server, void (message_handler)(monitor_message<int> *, app_server*)) {
        while(server->nonstop){
            memset(server->buffer,0,server->buffer_size);
            size_t recv = 0;
            server->server_socket->receive_raw(server->buffer,recv);
            auto *msg = (monitor_message<int> *) server->buffer;
            server->lock_map();
            auto monitor = server->monitor_map.at(std::string(msg->monitor_name));
            auto lamport_clock = monitor->get_Lamport_clock();
            printf("%s -> %s\n",msg->hostname,monitor->get_identifier());
            server->unlock_map();
            if (msg->msg_type == MSG_ASK){
                auto msg_body = msg->payload;
                if (msg_body == TYPE_ZERO_MSG_ASK) {
                    bool answer = ( lamport_clock > msg->sender_clock) ||
                                   (lamport_clock == msg->sender_clock &&
                                    strcmp(msg->monitor_name, monitor->get_identifier()) < 0) ||monitor->wait_flag;
                    if (answer) {
                        lamport_clock = monitor->set_Lamport_clock(msg->sender_clock);
                        auto reply_msg = monitor_message<int>(monitor->get_identifier(),
                                                              strlen(monitor->get_identifier()),
                                                              msg->monitor_name,
                                                              strlen(msg->hostname),
                                                              TYPE_ZERO_MSG_REPLY,
                                                              MSG_ASK,
                                                              lamport_clock);
                        server->server_socket->send_raw(reinterpret_cast<const char *>(&reply_msg), sizeof(reply_msg));
                    } else{

                        lamport_clock = monitor->set_Lamport_clock(msg->sender_clock);
                        auto reply_msg = monitor_message<int>(monitor->get_identifier(),
                                                              strlen(monitor->get_identifier()),
                                                              msg->monitor_name,
                                                              strlen(msg->hostname),
                                                              TYPE_ZERO_MSG_ASK,
                                                              MSG_ASK,
                                                              lamport_clock);
                        server->server_socket->send_raw(reinterpret_cast<const char *>(&reply_msg), sizeof(reply_msg));
                    }
                }else if (msg_body == TYPE_ZERO_MSG_NOTIFY){
                    monitor->unlock();
                    monitor->notify_all();
                }
            }else if (msg->msg_type == MSG_BROADCAST){
                memcpy(reinterpret_cast<void*>(monitor->get_very_unsafe_ptr()),&msg->payload,msg->payload_size);
                monitor->unlock();
                monitor->notify_all();
                monitor->addAnswer();
                lamport_clock = monitor->set_Lamport_clock(msg->sender_clock);
                auto reply_msg = (*msg);
                strcpy(reply_msg.hostname,monitor->get_identifier());
                reply_msg.sender_clock = lamport_clock;
                auto len = (sizeof(reply_msg)-sizeof(int)+reply_msg.payload_size);
                server->server_socket->send_raw(reinterpret_cast<const char *>(&reply_msg), len);
            }else{
                (message_handler)(msg,server);
            }
        }
    }

    monitor_map_t *app_server::lock_map() {
        map_mtx.lock();
        return &monitor_map;
    }

    void app_server::unlock_map() {
        map_mtx.unlock();
    }
};