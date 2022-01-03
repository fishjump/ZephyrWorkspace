#ifndef APP_UDP_H
#define APP_UDP_H

#include <net/net_context.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>

void listen_udp(uint16_t port, net_context_recv_cb_t cb);

void init_ipsp(struct in6_addr addr);

#endif // APP_UDP_H
