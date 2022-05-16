# EventQueue - a stupid easy FD multiplexer

EventQueue (eq) is a highly abstracted wrapper for either [kqueue](https://www.freebsd.org/cgi/man.cgi?query=kqueue&sektion=2) (MacOS) or [epoll](https://man7.org/linux/man-pages/man7/epoll.7.html) (Linux).

It only allows listening for when file descriptors are available for reading.

<br/>

## API

- `Eq::Eq()` - creates the EventQueue object which just initialises the underlying FD.

- `void Eq::add(int fd)` - adds a new event to the queue to listen for.

- `void Eq::add_all(std::initializer_list<int> fds` - shorthand for multiple adds.

- `void Eq::remove(int fd)` - removes existing event from queue. The underlying FD is automatically closed.

- `void Eq::remove_all(std::initializer_list<int> fds` - shorthand for multiple removes.

- `std::vector<Event> Eq::get_events()` - returns a vector of all registered events.

- `std::vector<Event> Eq::listen()` - starts to listen for any read activity amongst the registered events. Any events ready to read from are returned.

The `Event` class should not be instantiated directly - only through the eq's add and remove functions. Regardless, the class contains flags which depend on the underlying library being used. You can read more about the flags on the kqueue and epoll man pages.

- `uint16_t Event::get_flags()` - returns the flags set to this event

- `void Event::set_flags(uint16_t flags)` - sets the flag for this event. This is used by `Eq` and should not be used by the user.

<br/>

## Example - TCP socket server
This is a minimal example with no error checking on the creation of sockets. Any errors that may occur in the underlying event libraries are thrown from the `Eq::listen()` and `Eq::add(int fd)` and `Eq::remove(int fd)` functions.

```c++
#include "eventqueue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

using eq::Eq;

int main() {
    int fd;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(42523);
    address.sin_addr.s_addr = INADDR_ANY;

    // starting the server
    fd = socket(AF_INET, SOCK_STREAM, 0);
    bind(fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address));
    listen(fd, 0);

    // starting event queue and registering socket fd
    auto eq = Eq();
    eq.add(fd);

    // event listen loop
    while (true) {
        auto new_events = eq.listen();
        for (auto ev : new_events) {
            if (ev.get_fd() == fd) {
                // new socket to be accepted
                auto client_fd = accept(fd, nullptr, nullptr);
            } else {
                char buf[80];
                memset(buf, 0, 80);
                if (recv(ev.get_fd(), buf, 80, 0) > 0) {
                    // client sent message.
                } else {
                    // client disconnected.
                }
            }
        }
    }
}
```