#include <rte_ethdev.h>
#include <rte_ip4.h>
#include <rte_ring.h>


extern struct rte_ring* parse_ring;
extern struct rte_ring* tx_ring;


void print_ipv4_address(uint32_t addr){

  printf("%u.%u.%u.%u\n", (addr>>24)&(0xFF),(addr>>16)&(0xFF),(addr>>8)&(0xFF),(addr)&(0xFF));

}

void parse_TCP(struct rte_tcp_hdr* tcp_hdr){
  printf("the source port is : %u\n", rte_be_to_cpu_16(tcp_hdr->src_port));
  printf("the destination port is : %u\n", rte_be_to_cpu_16(tcp_hdr->dst_port));
  printf("TCP sequence number: %u\n", rte_be_to_cpu_32(tcp_hdr->sent_seq));

  return;
}

void parse_UDP(struct rte_udp_hdr* udp_hdr){
  printf("the source port is %u\n", rte_be_to_cpu_16(udp_hdr->src_port));
  printf("the destination port is %u\n", rte_be_to_cpu_16(udp_hdr->dst_port));
  return;
}

void ipv4_parsing(struct rte_ipv4_hdr* ipv4_hdr,struct rte_mbuf* packet, int offset){
    printf("source address : ");
    print_ipv4_address(rte_be_to_cpu_32(ipv4_hdr->src_addr));
    printf("destination address :");
    print_ipv4_address(rte_be_to_cpu_32(ipv4_hdr->dst_addr));


    //offset+= sizeof(struct rte_ipv4_hdr);
    uint8_t ihl = (ipv4_hdr->version_ihl & 0x0F) * 4;
    offset += ihl;

    printf("the next header is for protocol: ");

    if(ipv4_hdr->next_proto_id == IPPROTO_TCP){
      printf("TCP\n");
      struct rte_tcp_hdr* tcp_hdr = rte_pktmbuf_mtod_offset(packet, struct rte_tcp_hdr* , offset);
      parse_TCP(tcp_hdr);
    }
    else if(ipv4_hdr->next_proto_id == IPPROTO_UDP){
      printf("UDP\n");
      struct rte_udp_hdr* udp_hdr = rte_pktmbuf_mtod_offset(packet, struct rte_udp_hdr* , offset);
      parse_UDP(udp_hdr);
    }
    else{
      printf("unknown\n");
    }
}

void ipv6_parsing(struct rte_ipv6_hdr* ipv6_hdr, struct rte_mbuf* packet, int offset){
  //printf("ipv6 source address: \n");

    char src[RTE_IPV6_ADDR_SIZE];
    inet_ntop(AF_INET6, ipv6_hdr->src_addr.a, src, sizeof(src));
    printf("source IP address is : %s\n", src);
    
    //printf("ipv6 destination address: \n");
    char dst[RTE_IPV6_ADDR_SIZE];
    inet_ntop(AF_INET6, ipv6_hdr->dst_addr.a, dst, sizeof(dst));
    printf("Destination IP address is : %s\n", dst);
    
    //offset+= sizeof(struct rte_ipv6_hdr);
    uint8_t ihl = (ipv6_hdr->version & 0x0F) * 4;
    offset += ihl;

    printf("the next header is for protocol: ");

    if(ipv6_hdr->proto == IPPROTO_TCP){
      printf("TCP\n");
      struct rte_tcp_hdr* tcp_hdr = rte_pktmbuf_mtod_offset(packet, struct rte_tcp_hdr* , offset);
      parse_TCP(tcp_hdr);
    }
    else if(ipv6_hdr->proto == IPPROTO_UDP){
      printf("UDP\n");
      struct rte_udp_hdr* udp_hdr = rte_pktmbuf_mtod_offset(packet, struct rte_udp_hdr* , offset);
      parse_UDP(udp_hdr);
    }
    else{
      printf("unknown\n");
    }
}

int parsing_logic(struct rte_mbuf* packet, bool type_finder){
  int ip_type=0;
  
  int offset=0;
  struct rte_ether_hdr* eth_hdr = rte_pktmbuf_mtod(packet, struct rte_ether_hdr*);

  if(eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)){
    ip_type=4;
  }
  else if(eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV6)){
    ip_type=6;
  }


  if(type_finder==1){
    return ip_type;
  }


  printf("Source MAC address:\n");
  for(int i=0; i<RTE_ETHER_ADDR_LEN; i++){
    printf("%02x", eth_hdr->src_addr.addr_bytes[i]);
    if(i<RTE_ETHER_ADDR_LEN-1){
      printf(":");
    }
  }
  printf("\n");

  printf("Destination MAC address:\n");
  for(int i=0; i<RTE_ETHER_ADDR_LEN; i++){
    printf("%02x", eth_hdr->dst_addr.addr_bytes[i]);
    if(i<RTE_ETHER_ADDR_LEN-1){
      printf(":");
    }
  }
  printf("\n");

  if(ip_type==4){
    printf("IP : IPV4\n");
  }
  else{
    printf("IP: IPV6\n");
  }

  offset+=RTE_ETHER_HDR_LEN;

  if(ip_type==4){
    struct rte_ipv4_hdr* ipv4_hdr = rte_pktmbuf_mtod_offset(packet, struct rte_ipv4_hdr*, offset);
    ipv4_parsing(ipv4_hdr,packet,offset);
  }
  else if(ip_type==6){
    struct rte_ipv6_hdr* ipv6_hdr = rte_pktmbuf_mtod_offset(packet, struct rte_ipv6_hdr*, offset);
    ipv6_parsing(ipv6_hdr, packet, offset);
  } 
  else {
    printf("Unknown EtherType\n");
  }

  return ip_type;

}

