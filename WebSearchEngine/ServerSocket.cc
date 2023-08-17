/*
 * Copyright ©2023 Chris Thachuk.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

using std::string;

namespace hw4 {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int* const listen_fd) {
    // Use "getaddrinfo," "socket," "bind," and "listen" to
    // create a listening socket on port port_.  Return the
    // listening socket through the output parameter "listen_fd"
    // and set the ServerSocket data member "listen_sock_fd_"

    // Populate the "hints" addrinfo structure for getaddrinfo().
    // ("man addrinfo")
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = ai_family;       // IPv6 (also handles IPv4 clients)
    hints.ai_socktype = SOCK_STREAM;  // stream
    hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
    hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
    hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;
    char portbuf[10];
    sprintf(portbuf, "%hu", port_);
    // Use argv[1] as the string representation of our portnumber to
    // pass in to getaddrinfo().  getaddrinfo() returns a list of
    // address structures via the output parameter "result".
    struct addrinfo* result;
    int res = getaddrinfo(nullptr, portbuf, &hints, &result);

    // Did addrinfo() fail?
    if (res != 0) {
      std::cerr << "getaddrinfo() failed: ";
      std::cerr << gai_strerror(res) << std::endl;
      return false;
    }

    // Loop through the returned address structures until we are able
    // to create a socket and bind to one.  The address structures are
    // linked in a list through the "ai_next" field of result.
    int lsn_fd = -1;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
      lsn_fd = socket(rp->ai_family,
                        rp->ai_socktype,
                        rp->ai_protocol);
      if (lsn_fd == -1) {
        // Creating this socket failed.  So, loop to the next returned
        // result and try again.
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        lsn_fd = -1;
        continue;
      }

      // Configure the socket; we're setting a socket "option."  In
      // particular, we set "SO_REUSEADDR", which tells the TCP stack
      // so make the port we bind to available again as soon as we
      // exit, rather than waiting for a few tens of seconds to recycle it.
      int optval = 1;
      setsockopt(lsn_fd, SOL_SOCKET, SO_REUSEADDR,
                &optval, sizeof(optval));

      // Try binding the socket to the address and port number returned
      // by getaddrinfo().
      if (bind(lsn_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
        // Bind worked!  Print out the information about what
        // we bound to.

        // Return to the caller the address family.
        break;
      }

      // The bind failed.  Close the socket, then loop back around and
      // try the next address/port returned by getaddrinfo().
      close(lsn_fd);
      lsn_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (lsn_fd == -1) {
    close(lsn_fd);
    return false;
  }

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(lsn_fd, SOMAXCONN) != 0) {
    std::cerr << "Failed to mark socket as listening: ";
    std::cerr << strerror(errno) << std::endl;
    close(lsn_fd);
    return false;
  }

  // Return to the client the listening file descriptor.
  *listen_fd = lsn_fd;
  listen_sock_fd_ = lsn_fd;
  return true;
}
// Handles the client return infos
bool ClientAddr(int fd, struct sockaddr* addr, size_t addrlen,
                int* const accepted_fd, std::string* const client_addr,
                uint16_t* const client_port,
                std::string* const client_dns_name);

// handles the client dns name lookup.
bool ReverseDNS(struct sockaddr* addr, size_t addrlen,
                std::string* const client_dns_name);

// handles server info.
bool ServerSide(int client_fd, std::string* const server_addr,
                          std::string* const server_dns_name);

bool ServerSocket::Accept(int* const accepted_fd,
                          std::string* const client_addr,
                          uint16_t* const client_port,
                          std::string* const client_dns_name,
                          std::string* const server_addr,
                          std::string* const server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // STEP 2:
  int client_fd;
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    client_fd = accept(listen_sock_fd_,
                           reinterpret_cast<struct sockaddr*>(&caddr),
                           &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
      return false;
    }
    if (!ClientAddr(client_fd,
          reinterpret_cast<struct sockaddr*>(&caddr), caddr_len,
          accepted_fd, client_addr, client_port, client_dns_name))
          return false;
    break;
  }
  // handles server info return
  if (!ServerSide(client_fd, server_addr, server_dns_name))
    return false;

  return true;
}


bool ClientAddr(int fd, struct sockaddr* addr, size_t addrlen,
                int* const accepted_fd, std::string* const client_addr,
                uint16_t* const client_port,
                std::string* const client_dns_name) {
  *accepted_fd = fd;
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in* in4 = reinterpret_cast<struct sockaddr_in*>(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    string castring(astring);
    *client_addr = castring;
    *client_port = ntohs(in4->sin_port);

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6* in6 = reinterpret_cast<struct sockaddr_in6*>(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    string castring(astring);
    *client_addr = castring;
    *client_port = ntohs(in6->sin6_port);
  } else {
    std::cout << "client ???? address and port ????" << std::endl;
    return false;
  }

  if (!ReverseDNS(addr, addrlen, client_dns_name)) {
    return false;
  }
  return true;
}

bool ReverseDNS(struct sockaddr* addr, size_t addrlen,
                std::string* const client_dns_name) {
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, addrlen, hostname, 1024, nullptr, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
    return false;
  }
  string hname(hostname);
  *client_dns_name = hname;
  return true;
}

bool ServerSide(int client_fd, std::string* const server_addr,
                          std::string* const server_dns_name) {
  char hname[1024];
  hname[0] = '\0';
  struct sockaddr_storage addr;
  socklen_t addr_len = sizeof(addr);
  if (getsockname(client_fd,
      reinterpret_cast<struct sockaddr*>(&addr), &addr_len) == -1) {
    return false;
  }
  // Determine the address family (IPv4 or IPv6)
  int family = addr.ss_family;
  char ip_address[INET6_ADDRSTRLEN];
  if (family == AF_INET) {
    struct sockaddr_in *ipv4 = reinterpret_cast<struct sockaddr_in*>(&addr);
    inet_ntop(AF_INET, &(ipv4->sin_addr), ip_address, INET_ADDRSTRLEN);
  } else {
    struct sockaddr_in6 *ipv6 = reinterpret_cast<struct sockaddr_in6*>(&addr);

    inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_address, INET6_ADDRSTRLEN);
  }
  getnameinfo(reinterpret_cast<struct sockaddr*>(&addr),
              addr_len, hname, 1024, nullptr, 0, 0);

  string servAddr(ip_address);
  string servDNS(hname);
  *server_addr = servAddr;
  *server_dns_name = servDNS;
  return true;
}
}  // namespace hw4
