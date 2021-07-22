//
// Created by mg on 18.06.2021.
//

#ifndef UNTITLED3_MONITOR_MESSAGE_H
#define UNTITLED3_MONITOR_MESSAGE_H

template<typename T_payload>struct monitor_message{
public:
    int msg_type;
    unsigned long sender_clock;
    char hostname[60]{};
    char monitor_name[80]{};
    size_t payload_size;
    T_payload payload;
    monitor_message(const char *host, size_t host_size, const char *monitor,size_t monitor_size, T_payload p,int type = 0, unsigned long clc = 0);
};

template<typename T_payload>
monitor_message<T_payload>::monitor_message(
        const char *host,
        size_t host_size,
        const char *monitor,
        size_t monitor_size,
        T_payload p,
        int type,
        unsigned long clc) {
    msg_type = type;
    sender_clock = clc;
    strncpy(hostname,host,host_size);
    strncpy(monitor_name,monitor,monitor_size);
    payload_size = sizeof(T_payload);
    payload = p;
}

#endif //UNTITLED3_MONITOR_MESSAGE_H
