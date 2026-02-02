#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_eventdev.h>

#define DEV_ID 0
#define CONS_PORT_ID 1
#define PROD_PORT_ID 0

int main(int argc, char** argv){
    int ret = rte_eal_init(argc,argv);

    if(ret<0){
        printf("rte_eal_init failed \n");
        return 1;
    }

    printf("eal initialised\n");

    struct rte_event_dev_config event_config={0};
    event_config.nb_event_ports=2;
    event_config.nb_event_queues=1;
    event_config.nb_event_port_dequeue_depth=32;
    event_config.nb_event_port_enqueue_depth=32;
    event_config.nb_event_queue_flows=1024;
    event_config.nb_events_limit=4096;

    ret = rte_event_dev_configure(DEV_ID, &event_config);
    if(ret<0){
        printf("event dev configure failed\n");
        return 2;
    }

    printf("event device configured\n");

    struct rte_event_queue_conf queue_config={0};
    queue_config.schedule_type=RTE_SCHED_TYPE_ATOMIC;
    queue_config.nb_atomic_flows=1024;
    queue_config.nb_atomic_order_sequences=1024;
    queue_config.priority=RTE_EVENT_DEV_PRIORITY_NORMAL;

    ret = rte_event_queue_setup(DEV_ID, 0, &queue_config);
    if(ret<0){
        printf("queue setup failed\n");
        return 3;
    }

    printf("queue created\n");


    struct rte_event_port_conf port_config;

    rte_event_port_default_conf_get(DEV_ID, 0, &port_config);

    port_config.event_port_cfg=RTE_EVENT_PORT_CFG_HINT_PRODUCER;
    ret= rte_event_port_setup(DEV_ID, PROD_PORT_ID, &port_config);
    if(ret<0){
        printf("port setup failed for port 0 which is producer\n");
        return 4;
    }

    port_config.event_port_cfg=RTE_EVENT_PORT_CFG_HINT_CONSUMER;
    ret= rte_event_port_setup(DEV_ID, CONS_PORT_ID, &port_config);
    if(ret<0){
        printf("port setup failed for port 1 which is consumer\n");
        return 4;
    }

    printf("ports created\n");


    // link consumer port to queue
    uint8_t queues[]={0};
    uint8_t priorities[]={RTE_EVENT_DEV_PRIORITY_NORMAL};
    ret=rte_event_port_link(DEV_ID, CONS_PORT_ID, queues, priorities, 1);
    if(ret<1){
        printf("number of links established is less than 1\n");
    }

    ret=rte_event_dev_start(DEV_ID);
    if(ret<0){
        printf("device has not started properly\n");
        return 5;
    }

    printf("event device started\n");


    struct rte_event event={0};
    // how to schedule event
    event.queue_id=0;
    event.op=RTE_EVENT_OP_NEW;
    event.sched_type=RTE_SCHED_TYPE_ATOMIC;
    event.event_type=RTE_EVENT_TYPE_CPU;
    event.flow_id=1;
    // payload
    event.u64=12345;

    printf("enqueue event\n");

    while(rte_event_enqueue_burst(DEV_ID, PROD_PORT_ID, &event, 1)!=1){
        printf("number of events actually enqueued are less than 1\n");
        rte_pause();
    }


    struct rte_event out={0};

    printf("waiting for event\n");

    while(1){
        ret=rte_event_dequeue_burst(DEV_ID, CONS_PORT_ID, &out, 1, 0);
        if(ret){
            printf("received event data: %ld\n", out.u64);
            break;
        }
    }

    rte_event_dev_stop(DEV_ID);
    rte_event_dev_close(DEV_ID);

    printf("event device stopped and closed\n");


    return 0;
}