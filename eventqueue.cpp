#include "eventqueue.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <unistd.h>

namespace eq {
#if defined(__KQUEUE__)
    Eq::Eq() {
        qfd = kqueue();
    }
#elif defined(__EPOLL__)
    Eq::Eq() {
        qfd = epoll_create1(0);
    }
#endif

    std::vector<Event> eq::Eq::get_events() const {
        return events;
    }

#if defined(__KQUEUE__)
    void Eq::add(int fd) {
        if (contains(fd)) {
            char s[80];
            sprintf(s, "FD %d already registered", fd);
            throw std::invalid_argument(s);
        }
        events.emplace_back(fd);

        EV_SET(change_event, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (kevent(qfd, change_event, 1, nullptr, 0, nullptr) < 0) {
            char s[80];
            sprintf(s, "kevent error when adding new event %d", fd);
            throw std::invalid_argument(s);
        }
    }
#elif defined(__EPOLL__)
    void Eq::add(int fd) {
        if (contains(fd)) {
            char s[80];
            sprintf(s, "FD %d already registered", fd);
            throw std::invalid_argument(s);
        }
        events.emplace_back(fd);

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(qfd, EPOLL_CTL_ADD, fd, &ev);
    }
#endif

    void Eq::add_all(std::initializer_list<int> fds) {
        for (int fd : fds) {
            add(fd);
        }
    }

    void Eq::remove(int fd) {
        if (!contains(fd)) {
            char s[80];
            sprintf(s, "FD %d not registered", fd);
            throw std::invalid_argument(s);
        }

        auto pred = [fd](const Event &ev) {return ev.get_fd() == fd;};
        events.erase(std::remove_if(events.begin(), events.end(), pred), events.end());
        close(fd);
    }

    void Eq::remove_all(std::initializer_list<int> fds) {
        for (int fd : fds) {
            remove(fd);
        }
    }

#if defined(__KQUEUE__)
    std::vector<Event> Eq::listen() {
        auto new_events = kevent(qfd, nullptr, 0, event, events.size(), nullptr);
        if (new_events < 0) {
            throw std::invalid_argument("kevent error on listen");
        }

        std::vector<Event> new_evs;

        for (int i = 0; i < new_events; i++) {
            int event_fd = event[i].ident;
            auto ev = get_event_by_fd(event_fd);
            ev.set_flags(event[i].flags);
            new_evs.push_back(ev);
        }

        return new_evs;
    }
#elif defined(__EPOLL__)
    std::vector<Event> Eq::listen() {
        std::vector<Event> new_evs;

        auto num_ready = epoll_wait(qfd, event, MAX_EVENTS, -1);
        for (int i = 0; i < num_ready; i++) {
            if (event[i].events & EPOLLIN) {
                int event_fd = event[i].data.fd;
                auto ev = get_event_by_fd(event_fd);
                ev.set_flags(event[i].events);
                new_evs.push_back(ev);
            }
        }

        return new_evs;
    }
#endif

    bool Eq::contains(int fd) const {
        auto pred = [fd](const Event &ev){return ev.get_fd() == fd;};
        return std::any_of(events.begin(), events.end(), pred);
    }

    const Event &Eq::get_event_by_fd(int fd) const {
        auto pred = [fd](const Event &ev){return ev.get_fd() == fd;};
        auto it = std::find_if(events.begin(), events.end(), pred);
        if (it == events.end()) {
            char s[80];
            sprintf(s, "Event with FD %d not registered", fd);
            throw std::invalid_argument(s);
        }
        return *it;
    }

    int Event::get_fd() const {
        return fd;
    }

    uint16_t Event::get_flags() const {
        return flags;
    }

    Event::Event(int fd) {
        this->fd = fd;
    }

    void Event::set_flags(uint16_t flags) {
        this->flags = flags;
    }
}
