//
// Created by mg on 16.06.2021.
//

#ifndef UNTITLED3_MONITOR_DISTRIBUTED_H
#define UNTITLED3_MONITOR_DISTRIBUTED_H
#include <zmqpp/zmqpp.hpp>
#include <string>
#include <thread>
#include <map>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include "monitor_message.h"
#define TYPE_ZERO_MSG_REPLY 2
#define TYPE_ZERO_MSG_ASK 0
#define MSG_ASK 0
#define MSG_BROADCAST -1
#define TYPE_ZERO_MSG_NOTIFY 4
namespace zmq_monitor {
    template<typename T> class monitor_distributed {
    private:
        zmqpp::context ctx;
        std::shared_ptr<zmqpp::socket> client;
        mutable std::mutex mtx, mtx_local, mtx_lamport, mtx_answer;
        std::condition_variable_any cv;
        T guarded_object;
        unsigned int answers;
        std::string hostname;
        char *buffer;
        size_t buffer_size;
        void queueing();
        std::atomic<bool> wait_flag;
        void unqueueing();
        unsigned long lamport_clock;
        std::string monitor_name;
    public:
        friend class app_server;
        std::vector<std::string> nodes;
        T *get_very_unsafe_ptr();
        explicit monitor_distributed(T any, std::string unique_name, std::string common_name, size_t buf_size = 16384);
        explicit monitor_distributed(std::string unique_name, std::string common_name, size_t buf_size = 16384);

        ~monitor_distributed();
        bool try_lock();
        void unlock();
        T get_object_for_change();

        T wait(bool local_release = false);

        void notify_one();

        void notify_all();
        const char *get_identifier();
        void set_object_after_change(T obj, unsigned long lampChange = 0);

        unsigned long set_Lamport_clock(unsigned long newCL, unsigned long td = 1);

        unsigned long get_Lamport_clock();

        void addAnswer();
    };

    /**
    *
    * @tparam T - type of object to guard
    * @param any - initial object state
    * @param unique_name - name to identify host in network
    * @param common_name - name to identify monitor in network
    */
    template<typename T>
    monitor_distributed<T>::monitor_distributed(T any, std::string unique_name, std::string common_name, const size_t buf_size){
        client = std::make_shared<zmqpp::socket>(ctx,zmqpp::socket_type::request);
        hostname = std::move(unique_name);
        monitor_name = std::move(common_name);
        guarded_object = T(any);
        lamport_clock = 1;
        buffer_size = buf_size;
        wait_flag.store(false);
        buffer = new char[buf_size];
    }
    template<typename T>
    monitor_distributed<T>::monitor_distributed( std::string unique_name, std::string common_name, const size_t buf_size){
        client = std::make_shared<zmqpp::socket>(ctx,zmqpp::socket_type::request);
        hostname = std::move(unique_name);
        monitor_name = std::move(common_name);
        guarded_object = T();
        lamport_clock = 1;
        buffer_size = buf_size;
        wait_flag.store(false);
        buffer = new char[buf_size];
    }

    template<typename T>
    monitor_distributed<T>::~monitor_distributed(){
        client->close();
        client.reset();
        ctx.terminate();
        delete [] buffer;
    }
    /**
     * Locks monitor and return guarded object
     * @tparam T
     * @return
     */
    template<typename T>
    T monitor_distributed<T>::get_object_for_change(){
        mtx_local.lock();
        mtx.lock();
        queueing();
        lamport_clock+=1;
        return guarded_object;
    }
    /**
     * Conditional release of monitor
     * @tparam T
     * @param local_release - flag for releasing monitor locally, default: false
     * @return
     */
    template<typename T>
    T monitor_distributed<T>::wait(bool local_release){
        wait_flag.store(true);
        lamport_clock+=1;
        if (local_release) mtx_local.unlock();
        unqueueing();
        cv.wait(mtx);
        wait_flag.store(false);
        if (local_release) mtx_local.lock();
        queueing();
        return guarded_object;
    }
    template<typename T>
    void monitor_distributed<T>::notify_one(){
        cv.notify_one();
    }
    template<typename T>
    void monitor_distributed<T>::notify_all(){
        cv.notify_all();
    }
    template<typename T>
    /**
     * Replace guarded object with new state, broadcasts to everyone and unlocks monitor
     * @tparam T type of obj
     * @param obj - new state of guarded object
     */
    void monitor_distributed<T>::set_object_after_change(T obj, unsigned long lampChange){
        guarded_object = obj;
        set_Lamport_clock(lamport_clock, lampChange);
        mtx.unlock();
        unqueueing();
        mtx_local.unlock();
    }
    /**
     * Procedure for queueing with Lamport's algorithm
     * @tparam T
     */
    template<typename T>
    void monitor_distributed<T>::queueing() {
        std::vector<std::string> nodes_COPY(nodes);
        auto lc = set_Lamport_clock(lamport_clock);
            for (std::string &node:nodes_COPY) if(!node.empty()){
                    memset(buffer,0,buffer_size);
                    client->connect(node);
                    auto host = hostname.c_str();
                    auto mon = monitor_name.c_str();
                    monitor_message<int> msg(host, strlen(host) + 1, mon, strlen(mon) + 1, TYPE_ZERO_MSG_ASK, MSG_ASK,lc);
                    client->send_raw(reinterpret_cast<const char *>(&msg), sizeof(msg));
                    size_t size;
                    client->receive_raw(buffer,size);
                    auto *reply = (monitor_message<int> *) buffer;
                    client->disconnect(node);
                    if(reply->payload == TYPE_ZERO_MSG_REPLY){
                        node = "";
                        addAnswer();
                    }
                    set_Lamport_clock(reply->sender_clock);
                }

        while (answers<nodes.size()) {
            cv.wait(mtx);
        }
    }
    /**
     * Broadcasting set object - it means also accepting the next
     * @tparam T
     */
    template<typename T>
    void monitor_distributed<T>::unqueueing() {
        std::vector<std::string> nodes_COPY(nodes);
        auto lc = set_Lamport_clock(lamport_clock);
        for (std::string &node:nodes_COPY) if(!node.empty()){
                    memset(buffer,0,buffer_size);
                    client->connect(node);
                    auto host = hostname.c_str();
                    auto mon = monitor_name.c_str();
                    monitor_message<T> msg(host, strlen(host) + 1, mon, strlen(mon) + 1, guarded_object, MSG_BROADCAST,lc);
                    client->send_raw(reinterpret_cast<const char *>(&msg), sizeof(msg));
                    size_t size;
                    client->receive_raw(buffer,size);
                    auto *reply = (monitor_message<int> *) buffer;
                    client->disconnect(node);
                    node = "";
                    set_Lamport_clock(reply->sender_clock);
        }
    }

    template<typename T>
    const char *monitor_distributed<T>::get_identifier() {
        return hostname.c_str();
    }

    template<typename T>
    bool monitor_distributed<T>::try_lock() {
        return mtx.try_lock();
    }

    template<typename T>
    void monitor_distributed<T>::unlock() {
        mtx.unlock();
    }

    template<typename T>
    T *monitor_distributed<T>::get_very_unsafe_ptr() {
        return &guarded_object;
    }

    template<typename T>
    unsigned long monitor_distributed<T>::get_Lamport_clock(){
        unsigned long a;
        mtx_lamport.lock();
        a = lamport_clock;
        mtx_lamport.unlock();
        return a;
    }

    template<typename T>
    unsigned long monitor_distributed<T>::set_Lamport_clock(unsigned long newCL, unsigned long td) {
        unsigned long a;
        mtx_lamport.lock();
        lamport_clock = std::max(lamport_clock,newCL) + td;
        a = lamport_clock;
        mtx_lamport.unlock();
        return a;
    }

    template<typename T>
    void monitor_distributed<T>::addAnswer() {
        mtx_answer.lock();
        monitor_distributed::answers = answers+1;
        mtx_answer.unlock();
    }

}


#endif //UNTITLED3_MONITOR_DISTRIBUTED_H
