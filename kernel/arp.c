#include "arp.h"
#include "defs.h"
#include "ethernet.h"
#include "ip.h"
#include "mbuf.h"
#include "spinlock.h"

#define ARP_OR_REQUEST 1
#define ARP_OR_REPLY   2
#define ARP_TABLE_SIZE 2048

#define ETH_HTYPE  1
#define IPV4_PTYPE 0x800
#define ETH_HLEN   6
#define IPV4_PLEN  4

static struct spinlock lock;
static struct arp_entry arp_table[ARP_TABLE_SIZE];

struct arp_entry *
arp_table_select(uint32 ip_addr)
{
  for (struct arp_entry *entry = arp_table; entry < arp_table + ARP_TABLE_SIZE; entry++) {
    if (entry->used && entry->ip_addr == ip_addr)
      return entry;
  }
  return 0;
}

struct arp_entry *
arp_table_get_unused(void)
{
  for (struct arp_entry *entry = arp_table; entry < arp_table + ARP_TABLE_SIZE; entry++) {
    if (!entry->used)
      return entry;
  }
  return 0;
}

static int
arp_tx(uint16 op, uint8 hw_addr[6], const uint8 dst_mac[6], uint32 dst_ip)
{
  struct mbuf *m = mbuf_alloc(MBUF_DEFAULT_HEADROOM);
  if (!m)
    return -1;
  struct arp *arp_hdr = (struct arp*) mbuf_append(m, sizeof(struct arp));
  arp_hdr->htype = toggle_endian16(ETH_HTYPE);
  arp_hdr->ptype = toggle_endian16(IPV4_PTYPE);
  arp_hdr->hlen = ETH_HLEN;
  arp_hdr->plen = IPV4_PLEN;
  arp_hdr->oper = toggle_endian16(op);
  memmove(arp_hdr->sha, LOCAL_MAC_ADDR, 6);
  arp_hdr->spa = toggle_endian32(LOCAL_IP_ADDR);
  memmove(arp_hdr->tha, hw_addr, 6);
  arp_hdr->tpa = toggle_endian32(dst_ip);
  ethernet_tx(m, ETH_TYPE_ARP, dst_mac);
  return 0;
}

void
arp_rx(struct mbuf *m)
{
  struct arp *arp_hdr = (struct arp*) mbuf_pop(m, sizeof(struct arp));

  // TODO: validate packet

  uint8 sender_mac[6];
  memmove(sender_mac, arp_hdr->sha, 6);
  uint32 sender_ip = toggle_endian32(arp_hdr->spa);
  uint16 op = toggle_endian16(arp_hdr->oper);
  switch (op) {
  case ARP_OR_REQUEST:
    arp_tx(ARP_OR_REPLY, sender_mac, sender_mac, sender_ip);
    return;
  case ARP_OR_REPLY:
    // TODO: need this? delete or update!
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
      if (!arp_table[i].used) {
        arp_table[i].used = 1;
        arp_table[i].ip_addr = sender_ip;
        memmove(arp_table[i].mac_addr, sender_mac, 6);
        break;
      }
    }
    return;
  default:
    printf("arp_rx: invalid operation %d\n", op);
  }
}

uint
arp_resolve(uint32 ip_addr, uint8 mac_addr[6])
{
  acquire(&lock);
  struct arp_entry *entry = arp_table_select(ip_addr);
  if (entry) {
    if (memcmp(entry->mac_addr, ETHERNET_ADDR_ANY, 6) == 0) {
      arp_tx(ARP_OR_REQUEST, (uint8*) ETHERNET_ADDR_ANY, ETHERNET_ADDR_BROADCAST, ip_addr);
      release(&lock);
      return 0;
    }
    memmove(mac_addr, entry->mac_addr, 6);
    release(&lock);
    return 1;
  }

  entry = arp_table_get_unused();
  if (!entry) {
    release(&lock);
    printf("full arp table\n");
    return -1;
  }

  arp_tx(ARP_OR_REQUEST, (uint8*) GATEWAY_MAC_ADDR, ETHERNET_ADDR_BROADCAST, ip_addr);

  release(&lock);
  return 0;
}

void
arp_init(void)
{
  initlock(&lock, "arp");
}
