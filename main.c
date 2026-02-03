#include <rte_launch.h>
#include <rte_mbuf.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_eventdev.h>
#include <rte_ethdev.h>

#include "port_config.c"
#include "parse.c"

#define DEV_ID 0

#define V4_Q_ID 0
#define V6_Q_ID 1

#define PROD_PORT_ID 0
#define IPV4_CONS_PORT_ID 1
#define IPV6_CONS_PORT_ID 2


#define FIND_IP_TYPE 1
#define PARSE_PACKET 0

int receive(void __rte_unused *arg){
    int num_rx=0;

    struct rte_mbuf* buf[32];

    while(1){
        int received = rte_eth_rx_burst(0, 0, buf, 32);

        if(received==0)continue;

        num_rx+=received;

        for(int i=0; i<received; i++){
            struct rte_event event={0};
            event.event_type =  RTE_EVENT_TYPE_ETHDEV;
            event.sched_type = RTE_SCHED_TYPE_PARALLEL;
            event.mbuf = buf[i]; 
            event.op = RTE_EVENT_OP_NEW; 
            
            int ip = parsing_logic(buf[i],FIND_IP_TYPE);
            if(ip==4){
                event.queue_id = V4_Q_ID;
            }
            else{
                event.queue_id = V6_Q_ID;
            }

            
            if(rte_event_enqueue_burst(DEV_ID, PROD_PORT_ID, &event, 1)<=0){
                //printf("some error while sending mbuf no %d as event\n", i);
                //continue
                rte_pause();
            }
            printf("event enqueued\n");
        }
        
    }


    return num_rx;
}


int transmit_ipv4(void *arg){
    struct rte_event out;

    while (1) {
        int ret = rte_event_dequeue_burst(
            DEV_ID, IPV4_CONS_PORT_ID, &out, 1, 0);

        if (ret < 1)
            continue;

        struct rte_mbuf *packet = out.mbuf;

        printf("received mbuf from ipv4 queue\n");
        parsing_logic(packet, PARSE_PACKET);
        
        // release and enqueue it back (not required in parallel)
        out.op = RTE_EVENT_OP_RELEASE;
        ret = rte_event_enqueue_burst(
            DEV_ID, IPV4_CONS_PORT_ID, &out, 1);
        if (ret < 1)
            printf("failed to release ipv4 event\n");


        rte_pktmbuf_free(packet);
    }
}


int drop_ipv6(void __rte_unused *arg){

    int num_drops=0;

    struct rte_event out; 
    while(1){

        int ret=rte_event_dequeue_burst(DEV_ID, IPV6_CONS_PORT_ID, &out, 1, 0);

        if(ret<1){
            continue;
        }

        printf("received mbuf from ipv6 queue, now dropping\n");

        // release and queue it back (not required in parallel)
        struct rte_mbuf* packet = out.mbuf;
        out.op=RTE_EVENT_OP_RELEASE;
        ret=rte_event_enqueue_burst(DEV_ID, IPV6_CONS_PORT_ID, &out, 1);
        if(ret<1){
            printf("some problem while trying to enqueue the released event in ipv6 drop\n");
        }


        rte_pktmbuf_free(packet);
        num_drops++;
    }


    return num_drops;
}

int main(int argc, char** argv){

    int ret;
    ret = rte_eal_init(argc, argv);

    if(ret<0){
        printf("rte_eal_init failed\n");
        return 1;
    }

    printf("initialization done\n");


    configure_port(0,1,1);
    


    // event device config
    struct rte_event_dev_config econfig={0};
    econfig.nb_event_ports=3;
    econfig.nb_event_queues=2;
    econfig.nb_event_port_dequeue_depth=32;
    econfig.nb_event_port_enqueue_depth=32;
    econfig.nb_event_queue_flows=1024;     // each packet will have sched type as parallel anyway
    econfig.nb_events_limit=4096;

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
    struct rte_event_port_conf receive_port = {
        .new_event_threshold = 1024,
        .dequeue_depth = 32,
        .enqueue_depth = 32,
        .event_port_cfg = RTE_EVENT_PORT_CFG_HINT_PRODUCER
    };

    struct rte_event_port_conf ipv4_port = {
        .new_event_threshold = 1024,
        .dequeue_depth = 32,
        .enqueue_depth = 32,
        .event_port_cfg = RTE_EVENT_PORT_CFG_HINT_CONSUMER
    };

    struct rte_event_port_conf ipv6_port = {
        .new_event_threshold = 1024,
        .dequeue_depth = 32,
        .enqueue_depth = 32,
        .event_port_cfg = RTE_EVENT_PORT_CFG_HINT_CONSUMER
    };



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


    


    // launch on each core
    rte_eal_remote_launch(receive, NULL, 4);
    //receive(NULL);
    rte_eal_remote_launch(transmit_ipv4, NULL, 2);
    rte_eal_remote_launch(drop_ipv6, NULL, 3);



    // wait for each core to finish
    unsigned lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_wait_lcore(lcore_id);
    }




    // stop and clean 
    rte_event_dev_stop(DEV_ID);
    rte_event_dev_close(DEV_ID);
    printf("closed\n");


    rte_eal_cleanup();
    printf("cleanup");

    return 0;
}