/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(ipsp);

/* Preventing log module registration in net_core.h */
#define NET_LOG_ENABLED 0

#include <errno.h>
#include <linker/sections.h>
#include <stdio.h>
#include <zephyr.h>

#include <net/net_context.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/udp.h>

#include <net.h>

/* Define my IP address where to expect messages */
#define MY_IP6ADDR                                                             \
  {                                                                            \
    {                                                                          \
      { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 }         \
    }                                                                          \
  }
#define MY_PREFIX_LEN 64

static struct in6_addr in6addr_my = MY_IP6ADDR;

#define MY_PORT 4242

static uint8_t buf_tx[NET_IPV6_MTU];

#define MAX_DBG_PRINT 64

static int build_reply(const char *name, struct net_pkt *pkt, uint8_t *buf) {
  int reply_len = net_pkt_remaining_data(pkt);
  int ret;

  LOG_DBG("%s received %d bytes", log_strdup(name), reply_len);

  ret = net_pkt_read(pkt, buf, reply_len);
  if (ret < 0) {
    LOG_ERR("cannot read packet: %d", ret);
    return ret;
  }

  LOG_DBG("sending %d bytes", reply_len);

  return reply_len;
}

static inline void pkt_sent(struct net_context *context, int status,
                            void *user_data) {
  if (status >= 0) {
    LOG_DBG("Sent %d bytes", status);
  }
}

static inline void set_dst_addr(sa_family_t family, struct net_pkt *pkt,
                                struct net_ipv6_hdr *ipv6_hdr,
                                struct net_udp_hdr *udp_hdr,
                                struct sockaddr *dst_addr) {
  net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr, &ipv6_hdr->src);
  net_sin6(dst_addr)->sin6_family = AF_INET6;
  net_sin6(dst_addr)->sin6_port = udp_hdr->src_port;
}

static void udp_received(struct net_context *context, struct net_pkt *pkt,
                         union net_ip_header *ip_hdr,
                         union net_proto_header *proto_hdr, int status,
                         void *user_data) {
  struct sockaddr dst_addr;
  sa_family_t family = net_pkt_family(pkt);
  static char dbg[MAX_DBG_PRINT + 1];
  int ret;

  snprintf(dbg, MAX_DBG_PRINT, "UDP IPv%c", family == AF_INET6 ? '6' : '4');

  set_dst_addr(family, pkt, ip_hdr->ipv6, proto_hdr->udp, &dst_addr);

  ret = build_reply(dbg, pkt, buf_tx);
  if (ret < 0) {
    LOG_ERR("Cannot send data to peer (%d)", ret);
    return;
  }

  net_pkt_unref(pkt);

  ret = net_context_sendto(context, buf_tx, ret, &dst_addr,
                           family == AF_INET6 ? sizeof(struct sockaddr_in6)
                                              : sizeof(struct sockaddr_in),
                           pkt_sent, K_NO_WAIT, user_data);
  if (ret < 0) {
    LOG_ERR("Cannot send data to peer (%d)", ret);
  }
}

void main(void) {
  init_ipsp(in6addr_my);
  listen_udp(MY_PORT, udp_received);
}
