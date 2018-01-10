#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "../core/NstMachine.hpp"
#include "watches.h"

static const char *default_dir = ".";
using namespace Nes;
extern Api::Emulator emulator;

int sock = -1;
struct sockaddr_un addr;

void watch_cb(unsigned int address, unsigned int data)
{
	int ret;
	uint16_t addr;
	uint8_t d;
	unsigned char buf[4];
	addr = address;
	d = data;
	buf[0] = 'M';
	memcpy(buf + 1, &addr, 2);
	buf[3] = d;
	watches_connect();
	ret = send(sock, buf, 4, 0);
	if(ret == -1)
	{
		perror("send");
	}
	close(sock);
}

void watches_init(void)
{
	int ret, i;
	uint16_t *watch_list, *p;
	int watch_length;
	Nes::Core::Machine& machine = emulator.operator Nes::Core::Machine&();

	if(create_sockaddr())
	{
		return;
	}
	if(watches_connect())
	{
		return;
	}
	if(get_watchlist(&watch_list, &watch_length))
	{
		fprintf(stderr, "Unable to get watch list\n");
		return;
	}
	close(sock);
	p = watch_list;
	for(i = 0; i < watch_length; i++, p++)
	{
		machine.cpu.AddWatch(*p);
	}
	free(watch_list);
	machine.cpu.AddWatchCallback(watch_cb);
}

int get_watchlist(uint16_t **list, int *count)
{
	int ret, i;
	uint16_t address_len;
	uint16_t *p;
	unsigned char buf[1026];
	ret = send(sock, "A", 1, 0);
	if(ret == -1)
	{
		perror("send");
		return -1;
	}
	ret = recv(sock, buf, sizeof(buf), 0);
	if(ret == -1)
	{
		perror("recv");
		return -1;
	}
	memcpy(&address_len, buf, 2);
	if(address_len > 512)
	{
		fprintf(stderr, "Too many watches recieved. Truncating to first 512\n");
		address_len = 512;
	}
	if(ret != (address_len *2) + 2)
	{
		fprintf(stderr, "Address list length %d does not match promised length %d. Ignoring\n", ret, address_len);
		return -1;
	}
	*list = (uint16_t *)malloc(address_len * 2);
	if(*list == NULL)
	{
		perror("malloc");
		return -1;
	}
	memcpy(*list, (buf + 2), address_len * 2);
    *count = address_len;
	return 0;
}

int create_sockaddr(void)
{
	const char *socket_dir;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;

	socket_dir = getenv("XDG_RUNTIME_DIR");
	if(!socket_dir)
	{
		socket_dir = default_dir;
	}
	if(strlen(socket_dir) > sizeof(addr.sun_path))
	{
		fprintf(stderr, "XDG_RUNTIME_DIR %s is too long\n", socket_dir);
		return -1;
	}
	strncpy(addr.sun_path, socket_dir, sizeof(addr.sun_path) - 1);
	strncat(addr.sun_path, "/randomizer.sock", sizeof(addr.sun_path) - strlen(socket_dir) - 1);
	return 0;
}

int watches_connect(void)
{
	int ret;

	sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if(sock == -1)
	{
		perror("Could not create socket");
		return -1;
	}

	ret = connect(sock, (const struct sockaddr *) &addr, sizeof(addr));
	if( ret == -1 )
	{
		perror("Unable to connect to server");
		return -1;
	}
	return 0;
}
