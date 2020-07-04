#if !1
a

b -> a
  a
  |
  b

c -> a

  a
 / \
b   c

d -> b
      a
     / \
    b   c
   /
  d

e -> b
f -> b

      a
     / \
    b   c
   /|\
  d f e


maybe there is a manual mend command
that bridges the gap

called egg because of how fragile the network is

/*each message that is sent contains {namelen, msglen, name, message}*/
each message that is sent contains {namelen, name, {struct msg}, {optional message buffer}}
/*struct msg contains {msg type} alert/text for now, {buf_sz if optional is filled}*/
struct msg contains {msg type} alert/text, buflen in case text
    if alert, buffer will contain two `struct sockaddr_in`s containing grandparent, sibling

    no message will ever be sent without a buffer

everyone printes message, then sends the message to everyone except for who they got it from
base case is no children
everyone should get each message exactly once

egg is a very precarious network
when one non-root member disconnects, they will destroy the entire network


must be thoroughly abstracted
spread_message(node, message) should alert the entire network
alert_children(node, message) should alert immediate children of backup plan
~~~~
if we want to get clever, each node could alert its neighbors/children of the contingency plan
in case it disconnects
a child connects to some node, the new parent tells the child what the address of their grandparent is

OR
the new parent tells the left child what the address of their grandparent is and all other children
what the address of their sibling to the left is

OR

----------------------------------------------------------------------------------------------
this is the one
----------------------------------------------------------------------------------------------
the new parent tells the leftmost child about only grandparent and all other children about
grandparent as well as sibling to the left
when parent disconnects, each child attempts to connect to grandparent
if this does not work, they connect to left sibling
if leftomst child ca not connect to grandparent, or none exists (in the case of root node disappearing)
it will wait for the other children to connect to it
----------------------------------------------------------------------------------------------


is there an advantage to either?:

once parent disconnects, child connects to grandparent

what if parent is the root?:
then the child is informed of its sibling


----------------------------------------------------------------------------------------------
each client will probably need
    - an accept() thread
    - a read() thread, spawned by accept() thread
          all reads can be blocking
    - a thread that monitors the connection status of the current parent peer
          this is the thread that will maintain the structure of the graph
          we ONLY need to monitor parent, if a child disconnects, it''s on
          our grandchildren to find us from our child''s instructions

          how should we detect bad connection?: does blocking make is easier?:
          wait omg it is so easy with blocking reads
          blocking read will return -1 when failing and will then return 0
          but the first one is -1. we can just check for -1
    
    we can use ifdefs to disable the repairs if it gets too complicated
    this will remove the monitoring thread and the alert mtype from this code
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

struct peer{
    struct sockaddr_in addr;
    int sock;
};

struct node{
    volatile _Bool active;
    int sock, n_children, children_cap;
    /* parent is the peer we've connected to directly to join the network */
    struct peer parent, * children;
};

typedef enum {ALERT = 0, TEXT}mtype;

struct msg_header{
    mtype type;
    /* bufsz is ony used for TEXT mtypes
     * otherwise, a bufsz of sizeof(struct sockaddr_in)*2 is assumed
     */
    int bufsz;
};

/* node operations */

void init_node(struct node* n, struct sockaddr_in local_addr){
/*void init_node(struct node* n){*/
    if((n->sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)perror("socket()");
    /*
     *struct sockaddr_in addr = {0};
     *addr.sin_family = AF_INET;
     *addr.sin_port = htons(PORT);
     *addr.sin_addr.s_addr = htonl(INADDR_ANY);
     */

    /*
     *struct sockaddr_in s = strtoip("192.168.0.5");
     *addr.sin_addr.s_addr = s.sin_addr.s_addr;
     */

    /*if(bind(n->sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)perror("bind()");*/
    if(bind(n->sock, (struct sockaddr*)&local_addr, sizeof(struct sockaddr_in)) == -1)perror("bind()");

    /*++local_addr.sin_port;*/
    /*if(bind(n->sock, (struct sockaddr*)&local_addr, sizeof(struct sockaddr_in)) == -1)perror("bind()");*/
    if(listen(n->sock, 5) == -1)perror("listen()");
    n->n_children = 0;
    n->children_cap = 50;
    memset(&n->parent, 0, sizeof(struct peer));
    n->children = malloc(sizeof(struct peer)*n->children_cap);
    n->active = 1;
}

struct peer init_peer(int sock, struct sockaddr_in addr){
    struct peer ret;
    ret.sock = sock;
    ret.addr = addr;
    return ret;
}

void insert_child(struct node* n, struct peer p){
    if(n->n_children == n->children_cap){
        n->children_cap *= 2;
        n->children = realloc(n->children, n->children_cap);
    }
    n->children[n->n_children++] = p;
}

/* TODO: we'll need to lock a mutex lock when altering structure
 * of tree if we're enabling mending
 */
_Bool join_tree(struct node* n, struct sockaddr_in addr){
    n->parent.addr = addr;
     /*_Bool ret = !connect(n->sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));*/
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    _Bool ret = !connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    if(!ret)perror("connect()");
    n->parent.sock = sock;
    return ret;
}

/* node operations end */

_Bool spread_msg(struct node* n, int msglen, char* msg, int from_sock){
    _Bool ret = 1;
    struct msg_header header;
    header.type = TEXT;
    header.bufsz = msglen;
    /*if(n->parent.sock != from_sock)send(n->parent.sock, msg, );*/
    if(n->parent.sock != from_sock){
        ret &= (send(n->parent.sock, &header, sizeof(struct msg_header), 0)
                == sizeof(struct msg_header));
        ret &= (send(n->parent.sock, msg, msglen, 0) == msglen);
    }
    for(int i = 0; i < n->n_children; ++i){
        if(n->children[i].sock == from_sock)continue;
        ret &= (send(n->children[i].sock, &header, sizeof(struct msg_header), 0)
                == sizeof(struct msg_header));
        ret &= (send(n->children[i].sock, msg, msglen, 0) == msglen);
    }
    return ret;
}

void* accept_connections_thread(void* node_v){
    struct node* n = node_v;

    struct sockaddr_in addr;
    socklen_t addrlen;

    while(n->active){
        /* TODO: check if addrlen != sizeof(struct sockaddr_in) */
        int fd = accept(n->sock, (struct sockaddr*)&addr, &addrlen);
        if(fd == -1)perror("accept()");
        insert_child(n, init_peer(fd, addr));
        printf("accepted new conn at %i\n", fd);
    }
    return NULL;
}

pthread_t spawn_accept_connections_thread(struct node* n){
    pthread_t pth;
    pthread_create(&pth, NULL, accept_connections_thread, (void*)n);
    return pth;
}

struct sockaddr_in strtoip(char* ip){
    struct sockaddr_in ret = {0};
    ret.sin_port = htons(PORT);
    ret.sin_family = AF_INET;
    inet_aton(ip, &ret.sin_addr);
    return ret;
}

/* TODO: 
 * use INADDR_ANY to simplify use
 */
int main(int a, char** b){
    /* ./egg <nick> <local ip>
     * ./egg <nick> <local ip> <remote ip>
     */
    if(a < 3){
        printf("usage:\n  %s <nick> <local ip>\n  %s <nick> <local ip> <remote ip>\n",
               *b, *b);
        return EXIT_FAILURE;
    }
    struct node n;
    init_node(&n, strtoip(b[2]));
    /*init_node(&n);*/
    pthread_t accept_th = spawn_accept_connections_thread(&n);
    if(a > 3){
        struct sockaddr_in addr = strtoip(b[3]);
        if(!join_tree(&n, addr))puts("failed to connect");
    }
    pthread_join(accept_th, NULL);
    return 0;
}
