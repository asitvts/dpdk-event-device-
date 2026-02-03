#include <stdint.h>
#include <stdio.h>
#include<unistd.h>

#include <rte_eal.h>
#include <rte_eventdev.h>
#include <rte_ethdev.h>

#define DEV_ID 0

#define V4_Q_ID 0
#define V6_Q_ID 1

#define PROD_PORT_ID 0
#define IPV4_CONS_PORT_ID 1
#define IPV6_CONS_PORT_ID 2


int main(int argc, char** argv){

    int ret;
    ret = rte_eal_init(argc, argv);

    if(ret<0){
        printf("rte_eal_init failed\n");
        return 1;
    }

    printf("initialization done\n");



    // event device config
    struct rte_event_dev_config econfig={0};
    econfig.nb_event_ports=3;
    econfig.nb_event_queues=2;
    econfig.nb_event_port_dequeue_depth=32;
    econfig.nb_event_port_enqueue_depth=32;
    econfig.nb_event_queue_flows=1;     // each packet will have sche type as parallel anyway

    ret = rte_event_dev_configure(DEV_ID, &econfig);
    if(ret<0){
        printf("event device configuratopn failed\n");
        return 2;
    }

    printf("event device configuration done\n");





    // queue setup
    struct rte_event_queue_conf ipv4queue={0};
    ipv4queue.schedule_type = RTE_SCHED_TYPE_PARALLEL;
    ret=rte_event_queue_setup(DEV_ID, V4_Q_ID, &ipv4queue);
    if(ret<0){
        printf("error while queue setup for ipv4 queue\n");
    }

    struct rte_event_queue_conf ipv6queue={0};
    ipv6queue.schedule_type = RTE_SCHED_TYPE_PARALLEL;
    ret=rte_event_queue_setup(DEV_ID, V6_Q_ID, &ipv6queue);
    if(ret<0){
        printf("error while queue setup for ipv6 queue\n");
    }
    printf("queue setup done\n");






    // port setup
    struct rte_event_port_conf receive_port;
    ret=rte_event_port_default_conf_get(DEV_ID, PROD_PORT_ID, &receive_port);
    if(ret<0){
        printf("error fetching default config for port receive\n");
    }
    receive_port.event_port_cfg= RTE_EVENT_PORT_CFG_HINT_PRODUCER;

    struct rte_event_port_conf ipv4_port;
    ret=rte_event_port_default_conf_get(DEV_ID, IPV4_CONS_PORT_ID, &ipv4_port);
    if(ret<0){
        printf("error fetching default config for port ipv4\n");
    }
    ipv4_port.event_port_cfg= RTE_EVENT_PORT_CFG_HINT_CONSUMER;

    struct rte_event_port_conf ipv6_port;
    ret=rte_event_port_default_conf_get(DEV_ID, IPV6_CONS_PORT_ID, &ipv6_port);
    if(ret<0){
        printf("error fetching default config for port ipv6\n");
    }
    ipv6_port.event_port_cfg= RTE_EVENT_PORT_CFG_HINT_CONSUMER;

    ret=rte_event_port_setup(DEV_ID, PROD_PORT_ID, &receive_port);
    if(ret<0){
        printf("receive port setup failed\n");
        return 3;
    }

    ret=rte_event_port_setup(DEV_ID, IPV4_CONS_PORT_ID, &ipv4_port);
    if(ret<0){
        printf("ipv4 port setup failed\n");
        return 4;
    }

    ret=rte_event_port_setup(DEV_ID, IPV6_CONS_PORT_ID, &ipv6_port);
    if(ret<0){
        printf("ipv port setup failed\n");
        return 5;
    }
    printf("ports created\n");







    //port linking
    uint8_t ipv4queues[]={V4_Q_ID};
    uint8_t prio[]={RTE_EVENT_DEV_PRIORITY_NORMAL};
    ret=rte_event_port_link(DEV_ID, IPV4_CONS_PORT_ID, ipv4queues, prio, 1);
    if(ret<0){
        printf("ipv4 port linking failed\n");
    }

    uint8_t ipv6queues[]={V6_Q_ID};
    prio[0]=RTE_EVENT_DEV_PRIORITY_LOWEST;
    ret=rte_event_port_link(DEV_ID, IPV6_CONS_PORT_ID, ipv6queues, prio, 1);
    if(ret<0){
        printf("ipv6 port linking failed\n");
    }

    printf("both ports linked\n");





    // device start
    ret= rte_event_dev_start(DEV_ID);
    if(ret<0){
        printf("event device start failed\n");
        return 6;
    }

    printf("device started\n");


    



    return 0;
}