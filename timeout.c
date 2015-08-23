#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<sys/time.h>
#include<errno.h>
#include<assert.h>
#include<signal.h>

typedef void(*handler_t)(int id);
typedef struct timeout_node{
    int fnode_id;
    uint64_t start_time;
    uint64_t timeout; 
    handler_t func; 
}tnode;


typedef struct timeout_event{
    tnode** event_array;
    int size;
    int used;
}tevent;

uint64_t get_current_time(){
    uint64_t cur_ms;
    struct timeval tv;
    struct timezone tz;
    if(-1 == gettimeofday(&tv, &tz)){
        perror("Get Current Time Error!");
    }
    cur_ms = tv.tv_sec * 1000 + tv.tv_usec;
    return cur_ms;
}


void timeout_func(int id){
    printf("the event %d is timeout\n", id);
}

tevent* event_init(int max_event_num){
    tevent* tv = (tevent*)malloc(sizeof(*tv));    
    tv->size = max_event_num;
    tv->used = 0;
    tv->event_array = calloc(max_event_num, sizeof(*tv->event_array));
    assert(tv->event_array != NULL);
    return tv;
}

void event_free(tevent *tv){
    int i;
    for(i = 0; i < tv->size; i++){
        if(tv->event_array[i]){
            free(tv->event_array[i]);
        }
    }
    free(tv->event_array);
    free(tv);
}

tnode* node_init(int id){
    tnode* nd = calloc(1, sizeof(*nd));
    nd->fnode_id = id;
    nd->start_time = get_current_time();
    nd->timeout = 0;
    nd->func = NULL;
}

void node_free(tnode* nd){
    free(nd);
}

void timeout_event_register(tevent*tv, int nid, uint64_t timeout, handler_t handler){
    tnode* tnd = node_init(nid);
    tnd->start_time = get_current_time();
    tnd->func = handler;
    tnd->timeout = timeout;
    tv->event_array[tv->used] = tnd;
    tv->used++;
}

int timeout_event_unregister(tevent*tv, int nid){
    int i;
    for(i = 0; i < tv->size; i++){
        if(tv->event_array[i] && tv->event_array[i]->fnode_id == nid){
            node_free(tv->event_array[i]);
            tv->event_array[i] = NULL;
            tv->used--;
            return 1;
        }
    }
    return 0;
}

void process_timeout(tevent *tv){
    int i; 
    for(i = 0; i < tv->size; i++){

        //printf("%ld, %ld, %ld\n", get_current_time(), tv->event_array[i]->start_time, tv->event_array[i]->timeout );
        if(tv->event_array[i]){
            if(get_current_time() > (tv->event_array[i]->start_time + tv->event_array[i]->timeout)){
                tv->event_array[i]->func(tv->event_array[i]->fnode_id);
                timeout_event_unregister(tv, tv->event_array[i]->fnode_id);
            }
        }
    }
}

volatile int srv_shutdown = 0;
volatile int graceful_shutdown = 0;
void sigHandler(int sig_num){
    switch (sig_num) {
        case SIGTERM: srv_shutdown = 1; break;
        case SIGINT:
                      if (graceful_shutdown) srv_shutdown = 1;
                      else graceful_shutdown = 1;

                      break;
                      //       case SIGALRM: handle_sig_alarm = 1; break;
                      //       case SIGHUP:  handle_sig_hup = 1; break;
        case SIGCHLD:  break;
    }

}

int main(){
    struct sigaction act;

    act.sa_handler = sigHandler;
    sigaction(SIGINT, &act, NULL);

    tevent* tv = event_init(10);
    int status;
    int i; 
    for(i = 0;i < 3;i++){
        timeout_event_register(tv, i, 5000, timeout_func);
    }
    while(!srv_shutdown){
        process_timeout(tv); 
        sleep(2);
    }
    if(srv_shutdown){
        printf("The Event is shutted down!");
        //free resource
        event_free(tv);

    }

    return 0;
}
