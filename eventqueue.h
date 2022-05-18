#pragma once
#include <vector>
#include <cstdint>

#define MAX_EVENTS 64

#if defined(__linux__)
#   define __EPOLL__
#elif defined(__APPLE__) || defined(__unix__) || defined(BSD) || defined(unix) || defined(__unix)
#   define __KQUEUE__
#endif

#if defined(__KQUEUE__)
#   include <sys/event.h>
#elif defined(__EPOLL__)
#   include <sys/epoll.h>
#endif

namespace eq {

    /**
     * A wrapper for the file descriptor and possible flags.
     */
    class Event {
    private:
        int fd{};
        uint16_t flags{};
    public:
        Event() = default;
        explicit Event(int fd);
        /**
         * @return the file descriptor of this Event
         */
        int get_fd() const;
        /**
         * Both epoll and kqueue have flags that are set to an event. You can read more about
         * these flags on their respective man pages. Event flags are set but not used in Eq, so
         * it is up to the application developer to use.
         * @return the flags to get
         */
        uint16_t get_flags() const;
        /**
         * Both epoll and kqueue have flags that are set to an event. You can read more about
         * these flags on their respective man pages. Event flags are set but not used in Eq, so
         * it is up to the application developer to use.
         * @param flags the flags to set
         */
        void set_flags(uint16_t flags);
    };

    /**
     * The EventQueue object
     */
    class Eq {
    private:
        std::vector<Event> events{};
#       if defined(__KQUEUE__)
            struct kevent change_event[MAX_EVENTS]{}, event[MAX_EVENTS]{};
#       elif defined(__EPOLL__)
            struct epoll_event event[MAX_EVENTS];
#       endif
        int qfd{};

        bool contains(int fd) const;
        const Event &get_event_by_fd(int fd) const;

    public:
        /**
         * Starts the underlying FD with either kqueue() or epoll_create1(0);
         */
        Eq();
        /**
         * @return all events added to this Eq via Eq::add(int fd).
         */
        std::vector<Event> get_events() const;
        /**
         * Creates an Event object which wraps the input fd and adds the result
         * to the Eq's events vector.
         * @param fd the file descriptor to listen for
         * @throws std::invalid_exception if an event in this Eq already has fd
         * or if the underlying library fails a system call
         */
        void add(int fd);
        /**
         * Shorthand for multiple adds.
         * @param fds an initializer_list of valid file descriptors.
         * @example eq.add_all({ fd1, fd2, fd3 });
         */
        void add_all(std::initializer_list<int> fds);
        /**
         * Shorthand for multiple removes.
         * @param fds an initializer_list of valid file descriptors.
         * @example eq.remove_all({ fd1, fd2, fd3 });
         */
        void remove_all(std::initializer_list<int> fds);
        /**
         * Removes an event from this queue and automatically closes the fd.
         * @param fd the file descriptor of the event to remove
         * @throws std::invalid_exception if an event in this Eq with this fd
         * doesn't exist or if the underlying library fails a system call
         */
        void remove(int fd);
        /**
         * Blocks on the underlying poll until one or more events are available
         * to receive data from.
         * Once one or more events are available, they are added to a std::vector<Event>
         * which is returned.
         * @return a vector of Events ready to read from
         * @throws std::invalid_exception if the underlying library fails
         * a system call
         */
        std::vector<Event> listen();
    };
}
