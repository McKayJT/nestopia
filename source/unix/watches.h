#ifndef _WATCHES_H_
#define _WATCHES_H_

#include <stdint.h>
#include "core/api/NstApiEmulator.hpp"

void watches_init(void);
int watches_connect(void);
int create_sockaddr(void);
void watches_cb(void);
int get_watchlist(uint16_t **, int *);

#endif
