#include "pmconnection.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Includes for TCP/IP and serial */
#include <unistd.h>
#include <sys/time.h>

#ifdef _WIN32
// Windows
#ifndef WINVER
#define WINVER 0x0502 // XP SP2
#endif

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#define ERROR_CODE WSAGetLastError()
#define CONNECT_IN_PROGRESS WSAEWOULDBLOCK

#else
// Unix-like
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <termios.h>
#include <fcntl.h>
#include <errno.h>

#define ERROR_CODE errno
#define CONNECT_IN_PROGRESS EINPROGRESS
#endif

struct PMConnection {
  int fd;
  bool inet;
  int version; // Set to zero for serial connections

  // For serial ports on windows, we need a different type of descriptor
#ifdef _WIN32
  HANDLE winserial;
#endif
};

#define SERIALTIMEOUT 4 // sec
#define INETTIMEOUT 10 // sec

int sendBytes(struct PMConnection *conn, int len, void *buf) {
  while(len > 0) {
    int bytes = 0;
    // GG: I guess this is serial instead of ip/tcp
    if(!conn->inet) {
#ifdef _WIN32
      // GG: Is this still true for Windows?
      DWORD serBytes;
      // For windows, serial ports need special handling
      if(!WriteFile(conn->winserial, buf, len, &serBytes, NULL)) {
        return PM_ERROR_COMMUNICATION;
      }
      bytes = serBytes;
#else
      bytes = write(conn->fd, buf, len);
#endif
    } else {
#ifdef __linux__
      // Avoid SIGPIPE
      bytes = send(conn->fd, buf, len, MSG_NOSIGNAL);
#else
      bytes = send(conn->fd, buf, len, 0);
#endif
    }
    if(bytes <= 0)
      return PM_ERROR_COMMUNICATION;
    len -= bytes;
    buf = (char *) buf + bytes;
  }
  return 0;
}

int receiveBytes(struct PMConnection *conn, int len, void *buf) {
  while(len > 0) {
    int bytes = 0;
    if(!conn->inet) {
#ifdef _WIN32
      DWORD serBytes;
      // For windows, serial ports need special handling
      if(!ReadFile(conn->winserial, buf, len, &serBytes, NULL)) {
        return PM_ERROR_COMMUNICATION;
      }
      bytes = serBytes;
#else
      bytes = read(conn->fd, buf, len);
#endif
    } else {
      bytes = recv(conn->fd, buf, len, 0);
    }
    if(bytes <= 0)
      return PM_ERROR_COMMUNICATION;
    len -= bytes;
    buf = (char *) buf + bytes;
  }
  return 0;
}

PMCOMM_API struct PMConnection * PM_CALLCONV PMOpenConnectionSerial(const char *serialport) {
  struct PMConnection *res = malloc(sizeof(struct PMConnection));
  if(!res)
    return NULL;

  memset(res, 0, sizeof(*res));

#ifdef _WIN32
  res->winserial = CreateFile(serialport, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if(res->winserial == INVALID_HANDLE_VALUE) {
    free(res);
    return NULL;
  }

  DCB dcbSerial;
  memset(&dcbSerial, 0, sizeof(dcbSerial));

  dcbSerial.DCBlength = sizeof(dcbSerial);
  if(!GetCommState(res->winserial, &dcbSerial)) {
    CloseHandle(res->winserial);
    free(res);
    return NULL;
  }

  dcbSerial.BaudRate = CBR_2400;
  dcbSerial.ByteSize = 8;
  dcbSerial.Parity = NOPARITY;
  dcbSerial.StopBits = ONESTOPBIT;

  if(!SetCommState(res->winserial, &dcbSerial)) {
    CloseHandle(res->winserial);
    free(res);
    return NULL;
  }

  COMMTIMEOUTS timeouts;
  memset(&timeouts, 0, sizeof(timeouts));

  timeouts.ReadTotalTimeoutConstant = SERIALTIMEOUT * 1000;
  timeouts.WriteTotalTimeoutConstant = SERIALTIMEOUT * 1000;

  if(!SetCommTimeouts(res->winserial, &timeouts)) {
    CloseHandle(res->winserial);
    free(res);
    return NULL;
  }

#else
  res->fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY);
  if(res->fd < 0) {
    free(res);
    return NULL;
  }

  int error = fcntl(res->fd, F_SETFL, 0);
  if(error < 0) {
    close(res->fd);
    free(res);
    return NULL;
  }

  struct termios options;
  error = tcgetattr(res->fd, &options);
  if(error < 0) {
    close(res->fd);
    free(res);
    return NULL;
  }

  cfsetispeed(&options, B2400);
  cfsetospeed(&options, B2400);

  // Control options (8 bits, no parity, no flow control)
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_cflag &= ~CRTSCTS;

  // Line options (raw mode)
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  // Disable input parity and flow control
  options.c_iflag &= ~INPCK;
  options.c_iflag &= ~(IXON | IXOFF | IXANY);

  // Make output raw
  options.c_oflag &= ~OPOST;

  // Set timeout
  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = SERIALTIMEOUT * 10;

  error = tcsetattr(res->fd, TCSANOW, &options);
  if(error < 0) {
    close(res->fd);
    free(res);
    return NULL;
  }
#endif

  return res;
}


static int PMRespondPassword(struct PMConnection *conn) {
  unsigned char challenge[9];
  int error = receiveBytes(conn, 9, challenge);
  if(error < 0)
    return error;

  conn->version = challenge[0];
  if(memcmp(challenge + 1, "\x52\x1a\xdd\x8c\x26\x97\xc7\x80", 8) != 0)
    return PM_ERROR_CONNECTION;

  error = sendBytes(conn, 8, "\xee\x28\xda\x94\x8b\x0f\x87\x3a");
  if(error < 0)
    return error;

  unsigned char correct;
  error = receiveBytes(conn, 1, &correct);
  if(error < 0)
    return error;

  if(correct == 0)
    return 0;
  else
    return PM_ERROR_CONNECTION;
}

static int SetNonblocking(int fd, bool nonblock) {
  int error;
#ifdef _WIN32
  u_long blockMode = nonblock;
  error = ioctlsocket(fd, FIONBIO, &blockMode);
#else
  int flags = fcntl(fd, F_GETFL, NULL);
  if(flags < 0)
    return flags;
  if(nonblock)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;
  error = fcntl(fd, F_SETFL, flags);
#endif
  return error;
}

bool IsConnectionInet(struct PMConnection *conn) {
  return conn->inet;
}

PMCOMM_API struct PMConnection * PM_CALLCONV PMOpenConnectionInet(const char *hostname, uint16_t port) {
  struct PMConnection *res = malloc(sizeof(struct PMConnection));
  if(!res)
    return NULL;

  memset(res, 0, sizeof(*res));
  // PMRequestListInit(&res->waiting);
  res->inet = true;

  struct addrinfo *addrlist, *addr, hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  int error = getaddrinfo(hostname, NULL, &hints, &addrlist);
  if (error) {
    freeaddrinfo(addrlist);
    free(res);
    return NULL;
  }

  for(addr = addrlist; addr != NULL; addr = addr->ai_next) {
    res->fd = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(res->fd < 0) {
      continue;
    }

    struct sockaddr_in inaddr;
    memcpy(&inaddr, addr->ai_addr, sizeof(inaddr));
    inaddr.sin_port = htons(port);

    error = SetNonblocking(res->fd, true);
    if(error < 0) {
      close(res->fd);
      continue;
    }

    error = connect(res->fd, (struct sockaddr *) &inaddr, addr->ai_addrlen);
    if(error < 0 && ERROR_CODE != CONNECT_IN_PROGRESS) {
      close(res->fd);
      continue;
    }

    /* Set up select() call */
    fd_set s;
    FD_ZERO(&s);
    FD_SET(res->fd, &s);
    struct timeval connTimeout;
    memset(&connTimeout, 0, sizeof(connTimeout));
    connTimeout.tv_sec = INETTIMEOUT;

    int numready = select(res->fd + 1, NULL, &s, NULL, &connTimeout);

    if(numready != 1) { /* Timed out */
      close(res->fd);
      continue;
    }

    int errorval; /* Check the status of the socket */
    socklen_t optlen = sizeof(errorval);
#ifdef _WIN32
    error = getsockopt(res->fd, SOL_SOCKET, SO_ERROR, (char *) &errorval, &optlen);
#else
    error = getsockopt(res->fd, SOL_SOCKET, SO_ERROR, &errorval, &optlen);
#endif

    if(error < 0 || errorval != 0) {
      close(res->fd);
      continue;
    }

    error = SetNonblocking(res->fd, false);
    if(error < 0) {
      close(res->fd);
      continue;
    }

    break;
  }

  freeaddrinfo(addrlist);

  if(addr == NULL) {
    free(res);
    return NULL;
  }

  error = 0;

#ifdef _WIN32
  DWORD timeout = INETTIMEOUT * 1000;
#else
  struct timeval timeout;
  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = INETTIMEOUT;
#endif

  // Set receive timeout
  if(error == 0) {
    error = setsockopt(res->fd, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout));
  }

  // Set transmit timeout
  if(error == 0) {
    error = setsockopt(res->fd, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout));
  }

#ifdef __APPLE__
  // Avoid SIGPIPE
  if(error == 0) {
    int on = 1;
    error = setsockopt(res->fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
  }
#endif

  if(error == 0) {
    error = PMRespondPassword(res);
  }

  if(error != 0) {
    close(res->fd);
    free(res);
    res = NULL;
  }

  return res;
}

PMCOMM_API int PM_CALLCONV PMNetInitialize() {
#ifdef _WIN32
  WSADATA wsaData;

  int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if(err < 0)
    return PM_ERROR_OTHER;
#endif
  return 0;
}

PMCOMM_API int PM_CALLCONV PMNetUninitialize() {
#ifdef _WIN32
  int err = WSACleanup();
  if(err < 0)
    return PM_ERROR_OTHER;
#endif
  return 0;
}

PMCOMM_API void PM_CALLCONV PMCloseConnection(struct PMConnection *conn) {
#ifdef _WIN32
  // For windows, serial ports need special handling
  if(!conn->inet) {
    CloseHandle(conn->winserial);
  } else {
    closesocket(conn->fd);
  }
#else
  close(conn->fd);
#endif

  free(conn);
}

PMCOMM_API int PM_CALLCONV PMGetInterfaceVersion(struct PMConnection *conn) {
  return conn->version;
}
