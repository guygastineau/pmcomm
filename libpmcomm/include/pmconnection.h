#ifndef PMCONNECTION_H
#define PMCONNECTION_H

#include "pmdefs.h"

#include <stdint.h>
#include <stdbool.h>

struct PMConnection;

int sendBytes(struct PMConnection *conn, int len, void *buf);
int receiveBytes(struct PMConnection *conn, int len, void *buf);

bool IsConnectionInet(struct PMConnection *conn);

#endif
