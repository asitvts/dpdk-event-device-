#include <rte_ethdev.h>
#include <rte_ether.h>
#include <stdint.h>

int configure_port(uint16_t portid, uint16_t num_rx_qs, uint16_t num_tx_qs) {
  printf("inside port_config>c\n");
  struct rte_eth_conf port_conf = {0};
  int ret;
  
  port_conf.rxmode.mtu = RTE_ETHER_MTU;
  port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;
  
  struct rte_mempool *mbuf_pool =
  rte_pktmbuf_pool_create(
      "MBUF_POOL",
      8192,
      250,
      0,
      RTE_MBUF_DEFAULT_BUF_SIZE,
      rte_socket_id()
    );

    if(mbuf_pool==NULL){
    printf("error while creating the mbuf pool\n");
    return 1;
    }
  
  
  ret = rte_eth_dev_configure(portid, num_rx_qs, num_tx_qs, &port_conf);
  
  if (ret < 0) {
    printf("error configuring the port %d\n", (int)portid);
    return ret;
  }
  printf("eth_dev_configure done\n");

  for(int q=0; q<num_rx_qs; q++){
    ret = rte_eth_rx_queue_setup(
      portid, 
      q, 
      1024, 
      rte_eth_dev_socket_id(portid),
      NULL,
      mbuf_pool
    );
    
    if(ret!=0){
      printf("failed while creating the rx queue\n");
      return 2;
    }
    printf("rx queue setup done\n");
  }
  
  for(int q=0; q<num_tx_qs; q++){

    ret = rte_eth_tx_queue_setup(
      portid, 
      q, 
      1024, 
      rte_eth_dev_socket_id(portid), 
      NULL
    );
    
    if(ret!=0){
      printf("failed while creating the tx queue\n");
      return 2;
    }
    printf("tx queue setup done\n");
  }
  
  ret=rte_eth_dev_start(portid);
  if (ret < 0) {
    printf("Failed to start port %d\n", portid);
    return ret;
  }
  printf("eth_dev_start done\n");
  printf("exiting port_config.c\n");
  return ret;
}