#pragma once
#include <vector>

#define MAX_EVENTS 64

#if defined(__APPLE__)
#   include <sys/event.h>
#elif defined(__linux__)
#   include <sys/epoll.h>
#endif

namespace eq {

    // todo: add more flags here and make them depend on epoll vs kqueue
    class Event {
    private:
        int fd{};
        uint16_t flags{};
    public:
        Event() = default;
        explicit Event(int fd);
        int get_fd() const;
        uint16_t get_flags() const;
        void set_flags(uint16_t flags);
    };

    class Eq {
    private:
        std::vector<Event> events{};
#       if defined(__APPLE__)
            struct kevent change_event[MAX_EVENTS]{}, event[MAX_EVENTS]{};
#       elif defined(__linux__)
            struct epoll_event event[MAX_EVENTS];
#       endif
        int qfd{};

        bool contains(int fd) const;
        const Event &get_event_by_fd(int fd) const;

    public:
        Eq();
        std::vector<Event> get_events() const;
        void add(int fd);
        void add_all(std::initializer_list<int> fds);
        void remove_all(std::initializer_list<int> fds);
        void remove(int fd);
        std::vector<Event> listen();
    };
}
