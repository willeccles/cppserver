cppserver

DESCRIPTION

This is a simple webserver made in C++. It's not really meant for use, since
it's pretty naive. At the moment, it has a very, very limited feature set.

USAGE

server <port> [root]
 - port (required): the port to listen on
 - root (optional): the root directory to host from (defaults to ".")

CONFIGURATION

See config.h. Configuration changes require a rebuild by running `make`. 

BUILDING

Building this program requires a C++2a capable compiler. It also is compatible
only with Unix-based systems, such as Linux and macOS.

TODO

 - Support HTTPS/SSL
 - Replace fork() with a configurable-size threadpool
