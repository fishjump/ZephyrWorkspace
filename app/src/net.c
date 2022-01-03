#include <logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(ipsp_udp);

#include <kernel.h>

#include <net.h>

#define STACKSIZE 2000
K_THREAD_STACK_DEFINE(net_thread_stack, STACKSIZE);
static struct k_thread thread_data;

static struct k_sem mutex;
static inline void init_lock(void) { k_sem_init(&mutex, 0, K_SEM_MAX_LIMIT); }
static inline void lock(void) { k_sem_take(&mutex, K_FOREVER); }
static inline void unlock(void) { k_sem_give(&mutex); }

static inline bool get_context(struct net_context **udp_recv6, uint16_t port);
static inline void setup_udp_recv(struct net_context *udp_recv6,
                                  net_context_recv_cb_t cb);

static void _listen_udp(void);
static uint16_t _port = 0;
static net_context_cb_t _cb = NULL;

void init_ipsp(struct in6_addr addr) {
  init_lock();
  if (net_addr_pton(AF_INET6, CONFIG_NET_CONFIG_MY_IPV6_ADDR, &addr) < 0) {
    LOG_ERR("Invalid IPv6 address %s", CONFIG_NET_CONFIG_MY_IPV6_ADDR);
  }

  struct net_if_addr *ifaddr;
  do {
    ifaddr =
        net_if_ipv6_addr_add(net_if_get_default(), &addr, NET_ADDR_MANUAL, 0);
  } while (ifaddr == NULL);
}

void listen_udp(uint16_t port, net_context_recv_cb_t cb) {
  _port = port;
  _cb = cb;
  k_thread_create(&thread_data, net_thread_stack, STACKSIZE,
                  (k_thread_entry_t)_listen_udp, NULL, NULL, NULL,
                  K_PRIO_COOP(7), 0, K_NO_WAIT);
}

static void _listen_udp(void) {
  struct net_context *udp_recv6 = NULL;

  if (!get_context(&udp_recv6, _port)) {
    LOG_ERR("Cannot get network contexts");
    return;
  }

  LOG_INF("Starting to wait");

  setup_udp_recv(udp_recv6, _cb);

  lock();

  LOG_INF("Stopping...");

  net_context_put(udp_recv6);
}

static inline bool get_context(struct net_context **udp_recv6, uint16_t port) {
  int ret;
  struct sockaddr_in6 my_addr6 = {0};

  my_addr6.sin6_family = AF_INET6;
  my_addr6.sin6_port = htons(port);

  ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, udp_recv6);
  if (ret < 0) {
    LOG_ERR("Cannot get network context for IPv6 UDP (%d)", ret);
    return false;
  }

  ret = net_context_bind(*udp_recv6, (struct sockaddr *)&my_addr6,
                         sizeof(struct sockaddr_in6));
  if (ret < 0) {
    LOG_ERR("Cannot bind IPv6 UDP port %d (%d)", ntohs(my_addr6.sin6_port),
            ret);
    return false;
  }

  return true;
}

static inline void setup_udp_recv(struct net_context *udp_recv6,
                                  net_context_recv_cb_t cb) {
  int ret;

  ret = net_context_recv(udp_recv6, cb, K_NO_WAIT, NULL);
  if (ret < 0) {
    LOG_ERR("Cannot receive IPv6 UDP packets");
  }
}