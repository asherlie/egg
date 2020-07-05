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

tenents of EGG
    EGG is a network of hierarchical trust
    there is no persistence of data
    nothing is stored
    there is no user that is more privileged than any other user

    EGG lets users be bridges between two 
    one node can connect many users who only have the one point of connection
    this lets one user vouch for many
    there is no way for a user to get the ip address of somebody
    that they are talking to through somebody that both parties know

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
TODO: each user should be able to specify max amount of connections
      they will accept
      any connections past this will not be accept()ed

TODO: add a thread that constantly sends a connected alert
      this lets each user know who is connected

TODO: read thread should take in a peer*, not a peer
      this will allow the relationships in the tree to be symbolic
      the parent pointer can have its values changed in case the
      tree is restructured

TODO: maybe add a welcome message when a new member joins
      and an alert when a trust branch is disconnected/severed
----------------------------------------------------------------------------------------------
each client will probably need
    - an accept() thread
    - a read() thread, spawned by accept() thread
          all reads can be blocking
              there should be read threads for each accept()ed peer,
              and the parent after joining
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

#define MSGLEN 500
#define NICKLEN 20

#ifdef COLOR_SUPPORT
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"
#endif


typedef enum {ALERT = 0, TEXT}mtype;

struct msg_header{
    mtype type;
    char nick[NICKLEN];
    /* bufsz is ony used for TEXT mtypes
     * otherwise, a bufsz of sizeof(struct sockaddr_in)*2 is assumed
     */
    int bufsz;
};

/* message queue */

struct mq_msg{
    struct msg_header mh;
    /* should this be dynamically allocated? */
    char txt[MSGLEN];
};

struct msgqueue{
    pthread_mutex_t mq_lock;
    struct mq_msg* msgs;
    int n_msgs, msg_cap;
};

void mq_init(struct msgqueue* mq){
    pthread_mutex_init(&mq->mq_lock, NULL);
    mq->n_msgs = 0;
    mq->msg_cap = 500;
    mq->msgs = malloc(sizeof(struct mq_msg)*mq->msg_cap);
}

void mq_insert(struct msgqueue* mq, struct mq_msg msg){
    pthread_mutex_lock(&mq->mq_lock);
    if(mq->n_msgs == mq->msg_cap){
        mq->msg_cap *= 2;
        mq->msgs = realloc(mq->msgs, sizeof(struct mq_msg)*mq->msg_cap);
    }
    mq->msgs[mq->n_msgs++] = msg;
    pthread_mutex_unlock(&mq->mq_lock);
}

struct mq_msg mq_pop(struct msgqueue* mq){
    pthread_mutex_lock(&mq->mq_lock);
    /* TODO: messages should be popped from beginning */
    struct mq_msg ret = {0};
    if(mq->n_msgs)ret = mq->msgs[--mq->n_msgs];
    pthread_mutex_unlock(&mq->mq_lock);
    return ret;
}

/* message queue end */


struct peer{
    struct sockaddr_in addr;
    int sock;
};

struct node{
    volatile _Bool active;
    pthread_mutex_t children_lock;
    int sock, n_children, children_cap;
    /* parent is the peer we've connected to directly to join the network */

    /* TODO: should likely be
     * struct peer* parent, ** children;
     */
    struct peer parent, * children;
    struct msgqueue* mq;
};

struct node_peer{
    struct node* n;
    struct peer/* * */ p;
};

/* node operations */

void init_node(struct node* n, struct sockaddr_in local_addr){
/*void init_node(struct node* n){*/
    mq_init(n->mq);
    pthread_mutex_init(&n->children_lock, NULL);
    if((n->sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)perror("socket()");
    if(bind(n->sock, (struct sockaddr*)&local_addr, sizeof(struct sockaddr_in)) == -1)perror("bind()");

    if(listen(n->sock, 5) == -1)perror("listen()");
    n->n_children = 0;
    n->children_cap = 50;
    memset(&n->parent, 0, sizeof(struct peer));
    n->children = malloc(sizeof(struct peer)*n->children_cap);
    n->active = 1;
    n->parent.sock = -1;
}

struct peer init_peer(int sock, struct sockaddr_in addr){
    struct peer ret;
    ret.sock = sock;
    ret.addr = addr;
    return ret;
}

/*_Bool spread_msg(struct node* n, int msglen, char* msg, int from_sock){*/
_Bool spread_msg(struct node* n, struct msg_header header, char* msg, int from_sock){
    _Bool ret = 1;
    /*
     *struct msg_header header;
     *header.type = TEXT;
     *header.bufsz = msglen;
     */
    /*if(n->parent.sock != from_sock)send(n->parent.sock, msg, );*/
    if(n->parent.sock != from_sock){
        ret &= (send(n->parent.sock, &header, sizeof(struct msg_header), 0)
                == sizeof(struct msg_header));
        /*ret &= (send(n->parent.sock, msg, msglen, 0) == msglen);*/
        ret &= (send(n->parent.sock, msg, header.bufsz, 0) == header.bufsz);
    }
    pthread_mutex_lock(&n->children_lock);
    for(int i = 0; i < n->n_children; ++i){
        if(n->children[i].sock == from_sock)continue;
        ret &= (send(n->children[i].sock, &header, sizeof(struct msg_header), 0)
                == sizeof(struct msg_header));
        ret &= (send(n->children[i].sock, msg, header.bufsz, 0) == header.bufsz);
    }
    pthread_mutex_unlock(&n->children_lock);
    return ret;
}

_Bool peer_eq(struct peer x, struct peer y){
    return (x.sock == y.sock && x.addr.sin_addr.s_addr == y.addr.sin_addr.s_addr);
}

/* a thread is spawned for each new accepted peer */
/*data needs to be added to some kind of buffer*/
/*data will be limited to MSGLEN bytes*/
void* read_peer_msg_thread(void* node_peer_v){
    struct node_peer* np = node_peer_v;

    struct msg_header header;
    int b_read;
    
    char buf[MSGLEN];
    while(1){
        if((b_read = read(np->p.sock, &header, sizeof(struct msg_header))) <= 0 ||
           (b_read = read(np->p.sock, buf, header.bufsz)) <= 0){
            /*puts("peer removed");*/
            if(peer_eq(np->p, np->n->parent)){
                memset(&np->n->parent.addr, 0, sizeof(struct sockaddr_in));
                np->n->parent.sock = -1;
                return NULL;
            }
            pthread_mutex_lock(&np->n->children_lock);
            for(int i = 0; i < np->n->n_children; ++i){
                if(peer_eq(np->p, np->n->children[i])){
                    memset(&np->n->children[i].addr, 0, sizeof(struct sockaddr_in));
                    np->n->children[i].sock = -1;
                    memmove(np->n->children+i, np->n->children+i+1, 
                            (np->n->n_children-i-1)*sizeof(struct peer));
                    pthread_mutex_unlock(&np->n->children_lock);
                    return NULL;
                }
            }
            pthread_mutex_unlock(&np->n->children_lock);
        }

        buf[b_read] = 0;
        /*
         * struct mq_msg mqm;
         * mqm.mh = header;
         * memcpy(mqm.txt, buf, b_read);
         * mq_insert(np->n->mq, mqm);
        */

        /* TODO: handle b_read != sizeof(struct msg_header) */
        /* TODO: handle b_read != header.bufsz */
        #ifdef COLOR_SUPPORT
        printf("%s%s%s: \"%s\"\n", ANSI_BLUE, header.nick, ANSI_RESET, buf);
        #else
        printf("%s: \"%s\"\n", header.nick, buf);
        #endif
        /*spread_msg(np->n, b_read, buf, np->p.sock);*/
        spread_msg(np->n, header, buf, np->p.sock);
        /* TODO: pop it in mq */
    }
}

pthread_t spawn_read_peer_msg_thread(struct node* n, struct peer p){
    struct node_peer* np = malloc(sizeof(struct node_peer));
    np->n = n;
    np->p = p;

    pthread_t pth;
    pthread_create(&pth, NULL, read_peer_msg_thread, (void*)np);

    return pth;
}

void* pop_mq_thread(void* mq_v){
    struct msgqueue* mq = mq_v;
    while(1){
        struct mq_msg m = mq_pop(mq);
        if(!m.mh.nick[0]){usleep(100);continue;}
        printf("%s\n", m.txt);
        /*
         * everything's gotta be a pointer
         * need to be able to return NULL
        */
    }
    return NULL;
}

void insert_child(struct node* n, struct peer p){
    if(n->n_children == n->children_cap){
        n->children_cap *= 2;
        n->children = realloc(n->children, sizeof(struct peer)*n->children_cap);
    }
    n->children[n->n_children++] = p;
    spawn_read_peer_msg_thread(n, p);
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
    n->parent.addr = addr;
    n->parent.sock = sock;
    spawn_read_peer_msg_thread(n, n->parent);
    return ret;
}

/* node operations end */

void* accept_connections_thread(void* node_v){
    struct node* n = node_v;

    struct sockaddr_in addr;
    socklen_t addrlen;

    while(n->active){
        /* TODO: check if addrlen != sizeof(struct sockaddr_in) */
        int fd = accept(n->sock, (struct sockaddr*)&addr, &addrlen);
        if(fd == -1)perror("accept()");
        insert_child(n, init_peer(fd, addr));
    }
    return NULL;
}

pthread_t spawn_accept_connections_thread(struct node* n){
    pthread_t pth;
    pthread_create(&pth, NULL, accept_connections_thread, (void*)n);
    return pth;
}

struct sockaddr_in strtoip(char* ip){
    struct sockaddr_in ret;
    memset(&ret, 0, sizeof(struct sockaddr_in));

    ret.sin_port = htons(PORT);
    ret.sin_family = AF_INET;
    inet_aton(ip, &ret.sin_addr);
    return ret;
}

int read_stdin(char* buf){
    int i;
    char c;
    for(i = 0; i < MSGLEN-1; ++i){
        if((c = getc(stdin)) == EOF || c == '\n')break;
        buf[i] = c;
    }
    buf[i+1] = 0;
    return i;
}

void p_welcome(char* nick){
    printf("welcome to EGG, ");
    #ifdef COLOR_SUPPORT
    printf("%s%s%s\n", ANSI_RED, nick, ANSI_RESET);
    #else
    printf("%s\n", nick);
    #endif
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

    struct msgqueue MQ;
    struct node n;
    n.mq = &MQ;

    /* TODO: if *b[2] == '_', use INADDR_ANY */
    if(*b[2] == '_'){
        struct sockaddr_in any;
        memset(&any, 0, sizeof(struct sockaddr_in));
        any.sin_family = AF_INET;
        any.sin_port = htons(PORT);
        any.sin_addr.s_addr = htonl(INADDR_ANY);
        init_node(&n, any);
    }
    else init_node(&n, strtoip(b[2]));
    /*init_node(&n);*/
    pthread_t accept_th = spawn_accept_connections_thread(&n);
    if(a > 3){
        struct sockaddr_in addr = strtoip(b[3]);
        if(!join_tree(&n, addr))puts("failed to connect");
    }

    char buf[MSGLEN];

    struct msg_header header;
    memcpy(header.nick, b[1], NICKLEN-1);
    header.nick[NICKLEN-1] = 0;

    p_welcome(header.nick);
    /*
     * pthread_t iff;
     * pthread_create(&iff, NULL, pop_mq_thread, n.mq);
    */

    while(1){
        header.bufsz = read_stdin(buf);
        if(!header.bufsz)continue;
        spread_msg(&n, header, buf, n.sock);
    }

    pthread_join(accept_th, NULL);
    return 0;
}
