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
    
    example tree:

            a
          / | \
         b  c  d
        /|
       e f

    terms: root   - first user - a is the root of the tree above
           child  - b, c, and d are children of a in the tree above
           parent - b is the parent of e and f in the tree above
           leaf   - user with no children - e, f, c, and d are leaves
                    in the tree above


    ALL USERS ARE AWARE OF THE ENTIRE STRUCTURE OF THE NETWORK

    ANY USER CAN KICK ANYONE THAT HAS CONECTED DIRECTLY TO THEM 
    OFF OF THE NETWORK

    THIS ESTABLISHES A SORT OF `HIERARCHY OF TRUST`
    THE IMPLICIT MOST TRUSTED USER IS THE ONE WHO STARTED THE NETWORK

    THE USERS THAT CONNECTED DIRECTLY TO THIS ONE ARE `VOUCHED FOR` BY
    THE INITIAL USER - BOTH BECAUSE THEY ARE AWARE OF THE IP ADDRESS OF
    THE INITIAL USER, AND BECAUSE THEY HAVE NOT BEEN KICKED BY THIS USER


    THIS SYSTEM OF HIERARCHICAL TRUST EXTENDS TO THE LEAVES OF THE TREE

    IF A MEMBER OF THE NETWORK IS UNDERSTOOD TO BE `UNTRUSTED`, WHATEVER
    THAT MEANS, THEY CAN BE KICKED BY THEIR PARENT, AND ARE CUT OFF
    FROM THE REST OF THE NETWORK
    THIS WILL FRAGMENT THE NETWORK INTO TWO SEPARATE BUT FULLY FUNCTIONAL
    NETWORKS EACH TIME IT OCCURS, SINCE THE BANISHED USER IS STILL IN
    CONTACT WITH ALL OF IT''S CHILDREN, GRANDCHILDREN, ETC.


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

#include "pp.h"

#define VER_STR "1.0.3"

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

#if !1
we will add a new mtype - TS_REQUEST - tree structure
this spreads throughout the network
once the message reaches a leaf, it is no longer spread
if(!n->n_children)

then the leaves do an upwards message
send to parents only, recursively
/*

        a
      / | \
    /   |   \
   |    |     \
   b    |      c
   |    d      | \
   e           g   \
              /      f
            h
          / |
         j  i

*/
a,b,e
a,d
a,c,g,h,j
a,c,g,h,i
a,c,f

squashes are dealt with first
squash matching a
        a
       /|\

squash matching c

_,b,e
_,d
_,_,g,h,j
_,_,g,h,i
_,_,f

        a
       /|\
      c

squash matching g

_,b,e
_,d
_,_,_,h,j
_,_,_,h,i
_,_,f

        a
       /|\
      c
    /
   g

squash matching h

_,b,e
_,d
_,_,_,_,j
_,_,_,_,i
_,_,f

            a
           /|\
          c
         /
       g
     /
   h

matches will be in the same point in the order
all `a`s wil line up
as will `c`s, `g`, `h`

go through looking for matches
if paths[i] matches

HUGE TODO:
    any user can kick any one of their children off of the network
------------
REAL METHOD
------------
0. spread_msg() reaches leaves informing them that their action is requested
1. leaves pass_up() message to let root know how many strings to expect
2. leaves **maybe** usleep(100) to provide time for root to add up values
3. root receives expected number of strings. this is stored
4. leaves pass_up() their nickname to their parents
5. when a node recieves a PASS_UP_DIAGRAM alert with a nick string, they
   prepend (","++ their nickname) to the nick string before sending it along
6. if there is nobody to pass nick string to:
        if n_received < expected: append a '|' to 
        else: we have our string. distribute it.
------------
REAL METHOD
------------

    asher - james
    /   \
maxime  eteri
  |       |
 nic     hannah
            \
            joel

asher,maxime,nic,|asher,eteri,hannah,joel,|asher,james,|


a,b,e|a,d|a,c,g,h,i|a,c,f|


a-b-e
^-d
^-c-g-h-i
^-^-f

go thru each line one str at a time
if it is the same as the string directly above it,
print a '^'
otherwise print the string

then print a "-\n"

a-b-e
a-d
a-c-g-h-i
a-c-f

a
|-be
|-d
|-cghi
|-f

a
 b
 d
 c


a
be d cghi cf
        a
       /|\
     /  |  \
   b    d    c

e _ ghi f

        a
       /|\
     /  |  \
   b    d    c
   |         |
   e         g

a----c---g--h--i
| \
b   \                               -
|    d
e

abe|ad|acg|acf

e passes up e
b passes up be
a recieves be
a inserts a
-- abe

d passes up d
a recieves d
a inserts a
-- ad

g passes up g
c recieves g
c inserts c
-- cg

f passes up f
c recieves f
c inserts c
-- cf

c passes up cg
a recieves cg
a inserts a
-- acg

c passes up cf
a recieves cf
a inserts a
-- acf

a combines inputs to create full tree diagram
-- abe|ad|acg|acf

NOTE: this method relies upon the root node spreading the
      tree diagram string around the network after it has
      been generated
      
      this is relatively expensive as a single tree diagram
      generation includes
        requester initiates spread_msg, sets flag indicating
        that a diagram has been requested - this node will
        now pretty print the diagram when it is recieved
        1 spread_msg to reach leaves
        1 pass up from each node to communicate nicks
        1 spread_msg to reach requester

how does a parent know when all of its children have notified it?
easy: each parent is aware of the number of childen it has
until n_children nick notifications have been recieved, nothing
will be passed up the tree

but what about parents who have multiple messages from one child
in the case of a-c, for example
c could indicate that it is sending up 2 paths
how does c know to wait for 2 paths?
easy: it has 2 children

b sends TS_REQUEST
TS_REQUEST is ignored and passed along 
until it reaches
{e, d, g, f}

{e, d, g, f}
pass their nicknames up in the buf field
NOTE: this is not a scalable solution - buf is limited to 500 bytes
TODO: buf should be dynamically allocated

// from leaf node
init pass up PASS_UP_DIAGRAM:
    pass_up(HEADS_UP_DIAGRAM 1)
    /* wait for all heads up diagrams to be recieved */
    usleep(1000);
    pass_up(header.nick)

from read():
    if header.type == HEADS_UP_DIAGRAM:
        expected_diag += n_expected;

    if header.type == PASS_UP_DIAGRAM:
        // build string before passing up
        tmp_pass_str += (header.nick ++ sender.nick)|
        pass_up(header.nick ++ sender.nick)
        if()pass_up()

e passes up "e"
b passes up "b"
a receives  "eb"

d passes up "d"
a recieves  "d"

g passes up "g"

f passes up "f"

c combines g and f
c passes up "cg|f"

a recieves "g|f"

a combines "eb|d|g|f"
this does not work - g|f 

#endif

typedef enum {TEXT=0, HIER_REQ, N_PASS_UP_ALERT, NICK_ALERT, HIER_ALERT}mtype;

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
    struct mq_msg ret;
    memset(&ret, 0, sizeof(struct mq_msg));
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
    _Bool awaiting_alert;
    pthread_mutex_t await_lock;

    volatile _Bool active;
    pthread_mutex_t children_lock,
                    expected_paths_lock;
    int sock, n_children, children_cap,
        expected_paths, paths_recvd,

        /* children_notified is used only for intermediate N_PASS_UP_ALERTs */
        children_notified;

    char nick[NICKLEN],
         /* lol */
         path_str[MSGLEN*100];
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

void init_node(struct node* n, char* nick, struct sockaddr_in local_addr){
    memcpy(n->nick, nick, NICKLEN-1);
    n->nick[NICKLEN-1] = 0;

    mq_init(n->mq);

    n->awaiting_alert = 0;
    pthread_mutex_init(&n->await_lock, NULL);

    pthread_mutex_init(&n->children_lock, NULL);
    pthread_mutex_init(&n->expected_paths_lock, NULL);

    if((n->sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        perror("socket()");
    int tr = 1;
    if(setsockopt(n->sock, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1)
        perror("setsockopt()");
    if(bind(n->sock, (struct sockaddr*)&local_addr, sizeof(struct sockaddr_in)) == -1)
        perror("bind()");

    if(listen(n->sock, 5) == -1)
        perror("listen()");
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

_Bool is_leaf(struct node* n){
    return n->n_children == 0;
}

_Bool is_root(struct node* n){
    return n->parent.sock == -1;
}

_Bool spread_msg(struct node* n, struct msg_header header, char* msg, int from_sock){
    _Bool ret = 1;
    /*
     *struct msg_header header;
     *header.type = TEXT;
     *header.bufsz = msglen;
     */
    /*if(n->parent.sock != from_sock)send(n->parent.sock, msg, );*/

    #ifdef DEBUG
    puts("spread msg called");
    #endif
    if(n->parent.sock != from_sock && n->parent.sock != -1){
        #ifdef DEBUG
        puts("sending to parent");
        #endif
        ret &= (send(n->parent.sock, &header, sizeof(struct msg_header), 0)
                == sizeof(struct msg_header));
        /*ret &= (send(n->parent.sock, msg, msglen, 0) == msglen);*/
        ret &= (send(n->parent.sock, msg, header.bufsz, 0) == header.bufsz);
    }
    pthread_mutex_lock(&n->children_lock);
    for(int i = 0; i < n->n_children; ++i){
        #ifdef DEBUG
        printf("sending to child? %i %i\n", n->children[i].sock, from_sock);
        #endif
        if(n->children[i].sock == from_sock ||
           n->children[i].sock == -1)continue;
       #ifdef DEBUG
       puts("YES");
       #endif
        ret &= (send(n->children[i].sock, &header, sizeof(struct msg_header), 0)
                == sizeof(struct msg_header));
        ret &= (send(n->children[i].sock, msg, header.bufsz, 0) == header.bufsz);
    }
    pthread_mutex_unlock(&n->children_lock);
    return ret;
}

_Bool pass_msg_up(struct node* n, struct msg_header header, char* msg, int from_sock){
    _Bool ret = 1;
    /* likely don't need this check */
    if(n->parent.sock != from_sock){
        ret &= (send(n->parent.sock, &header, sizeof(struct msg_header), 0)
                == sizeof(struct msg_header));
        ret &= (send(n->parent.sock, msg, header.bufsz, 0) == header.bufsz);
    }
    return ret;
}

void init_diagram_request(struct node* n){
    struct msg_header header;
    header.type = HIER_REQ;
    /* at least 1 byte must be sent */
    header.bufsz = 1;
    char spoof = (is_leaf(n) || is_root(n)) ? 'z' : 'a';
    pthread_mutex_lock(&n->await_lock);
    n->awaiting_alert = 1;
    pthread_mutex_unlock(&n->await_lock);
    spread_msg(n, header, &spoof, n->sock);
}

_Bool peer_eq(struct peer x, struct peer y){
    /* w can't simply check for address equality, as addresses
     * aren't guaranteed to be unique
     */
    return (x.sock == y.sock &&
            x.addr.sin_addr.s_addr == y.addr.sin_addr.s_addr);
}

void handle_msg(struct node_peer* np, struct msg_header header, char* buf){
    switch(header.type){
        case TEXT:
            #ifdef COLOR_SUPPORT
            printf("%s%s%s: \"%s\"\n", ANSI_BLUE, header.nick, ANSI_RESET, buf);
            #else
            printf("%s: \"%s\"\n", header.nick, buf);
            #endif
            spread_msg(np->n, header, buf, np->p.sock);
            break;
        case HIER_REQ:
        #if !1
        in the new system when a HIER_REQ is recvd,
        each user sets a flag indicating the amount
        of NICK_ALERTs it is expecting

        each node can still use the ridiculously sized path_str
        to modify the hier in progress before sending it off

        i would like to keep the passing and parsing system the same
        so it will not be possible to send 

        the only thing that should change is the expected_paths accuracy
        we can achieve this by introducing a new command, or rather
        modifying the N_PASS_UP_ALERT

        when a N_PASS_UP_ALERT command is recieved, 
        node increments counter
        /* node appends new pass up alert nick */
        node checks whether all of its direct children
        have sent an N_PASS_UP_ALERT
        if not, do nothing
        otherwise

        GET RID OF usleep(x)
        #endif

        /*
        if /p is called by the root, ncn never goes down
        it instead just keeps going up :eek:
        */

            #ifdef DEBUG
            puts("got HIER_REQ, testing leafiness...");
            #endif
            if(is_leaf(np->n)){
                #ifdef DEBUG
                puts("we've been reached :)  a leaf");
                #endif
                struct msg_header pu_h;
                pu_h.type = N_PASS_UP_ALERT;
                /*
                 * pu_h.bufsz = 1;
                 * char spoof;
                */
                pu_h.bufsz = sizeof(int);
                int one = 1;
                /* else? */pass_msg_up(np->n, pu_h, (char*)&one, np->n->sock);
                #ifdef DEBUG
                puts("passed up N_PASS_UP_ALERT");
                #endif

                /* give a little time */
                /*usleep(100000);*/
                pu_h.type = NICK_ALERT;
                /* we're not using NICKLEN because we need
                 * to fit many nicks
                 */
                pu_h.bufsz = strlen(np->n->nick)+1;
                char tmp_buf[MSGLEN] = {0};
                sprintf(tmp_buf, "%s,", np->n->nick);
                pass_msg_up(np->n, pu_h, tmp_buf, np->n->sock);
                break;
            }
            #ifdef DEBUG
            puts("expected_paths, paths_recvd set to 0");
            #endif
            pthread_mutex_lock(&np->n->expected_paths_lock);
            #ifdef DEBUG
            puts("\n\n\nRESETTING\n\n");
            #endif
            *np->n->path_str = 0;
            np->n->expected_paths = 0;
            np->n->paths_recvd = 0;
            np->n->children_notified = 0;
            pthread_mutex_unlock(&np->n->expected_paths_lock);
            /*
             * else if(is_root(np->n)){
             *     expected_paths = 0;
             * }
            */
            /* *buf == 'z' when sender is
             * a leaf
             * and we must ensure that they
             * end up with a HIER_REQ
             */
            /* not the most elegant solution */
            if(*buf == 'z'){
                *buf = 'a';
                spread_msg(np->n, header, buf, -1);
            }
            else spread_msg(np->n, header, buf, np->p.sock);
            break;
        case N_PASS_UP_ALERT:
            pthread_mutex_lock(&np->n->expected_paths_lock);
            ++np->n->children_notified;
            #ifdef DEBUG
            printf("npn incremented to %i!\n", np->n->children_notified);
            #endif
            /* would this happen from diff threads? 
             * could just lock on expected_paths_lock
             * i think it couldn't tbh - there's only one thread
             * for each child
             * oh wait this means we do need to lock
             * if another child sends a simultaneous msg
             * we'll be inaccurate
             */
            /* after a new request, expected_paths is always 0 */
            /* n_new is recursive children */
            int n_new;
            memcpy(&n_new, buf, sizeof(int));
            /*printf();*/
            np->n->expected_paths += n_new;// + 1;
            /*np->n->expected_paths += n_new + 1;*/
            /* only once all of our children have notified us
             * will we pass the message up
             */
            #ifdef DEBUG
            printf("expected pahts inc to %i\n", np->n->expected_paths);
            #endif
            if(np->n->children_notified == np->n->n_children){
                /* we're the root and have recvd all children's paths */
                if(is_root(np->n)){
                    
                }
                else{
                    #ifdef DEBUG
                    printf("we've collected all childrens' numbers :)\n");
                    #endif
                    /* ourself */
                    /*
                     * ++np->n->expected_paths;
                     * wait this is actually decent code to print the number of users in the net...
                     * it doesn't, however, inform the root of how many leaves there are in total
                    */
                    pass_msg_up(np->n, header, (char*)&np->n->expected_paths, np->p.sock);
                }
            }

            pthread_mutex_unlock(&np->n->expected_paths_lock);
            break;
        case NICK_ALERT:
            /* TODO: is_root() return should be stored up top */
            if(is_root(np->n)){
                #ifdef DEBUG
                printf("we're the root and we've been sent: \"%s\"\n", buf);
                #endif
                /*gotta combine it all until expected_paths is reached*/
                pthread_mutex_lock(&np->n->expected_paths_lock);
                ++np->n->paths_recvd;
                /*#ifdef DEBUG*/
                /*printf("%i/%i strings recvd, got our string: %s\n", np->n->paths_recvd, np->n->expected_paths, buf);*/
                /*#endif*/
                char tmp_buf[MSGLEN*1000];

                /*sprintf(tmp_buf, "%s,%s|", np->n->nick, np->n->path_str);*/
                sprintf(tmp_buf, "%s,%s|%s", np->n->nick, buf, np->n->path_str);
                #ifdef DEBUG
                printf("constructed str: %s\n", tmp_buf);
                #endif

                /*memcpy(tmp_buf, np->n->path_str, MSGLEN*100);*/
                memcpy(np->n->path_str, tmp_buf, MSGLEN*100);
                /*sprintf(np->n->path_str, "%s,%s|", np->n->nick, np->n->path_str);*/
                /*printf("str is nw: %s\n", np->n->path_str);*/
                #if 0
                this is what is up:
                    2 HIER_ALERTs are being sent
                    when the first is recieved, it thikns that is it

                    2 might be being sent because expected paths is incremented
                    to 1 and then our first alert is recieved

                    they are equal in this moment

                    confirm this by printing %i == %i

                    if they are 1 == 1
                    ::))

                    this is a hard problem to fix
                    could just wait longer before sending nick alert from leaves
                    
                    or could make it so that each node sends up a formatted str
                    then each parent knows to expect node->n_children responses

                    #endif

                /*if(np->n->paths_recvd == np->n->expected_paths){*/
                /*proper string is constructed but message isn't spread... why?:*/
                #ifdef DEBUG
                printf("ncn: %i, nc: %i, pathsrcvd: %i, exp: %i\n", np->n->children_notified, np->n->n_children, np->n->paths_recvd, np->n->expected_paths);
                #endif
                if(np->n->children_notified == np->n->n_children && np->n->paths_recvd == np->n->expected_paths){
                    struct msg_header sp_h;
                    sp_h.type = HIER_ALERT;
                    sp_h.bufsz = strlen(np->n->path_str);
                    *sp_h.nick = 0;
                    #ifdef DEBUG
                    printf("spreading message: %s\n", np->n->path_str);
                    #endif
                    spread_msg(np->n, sp_h, np->n->path_str, np->n->sock);
                    pthread_mutex_lock(&np->n->await_lock);
                    if(np->n->awaiting_alert){
                        np->n->awaiting_alert = 0;
                        print_tree(np->n->path_str);
                    }
                    pthread_mutex_unlock(&np->n->await_lock);
                }
                pthread_mutex_unlock(&np->n->expected_paths_lock);
            }
            /* !root */
            else{
                struct msg_header pu_h;
                pu_h.type = NICK_ALERT;
                /* lol */
                char tmp_buf[MSGLEN*100] = {0};
                sprintf(tmp_buf, "%s,%s", np->n->nick, buf);
                pu_h.bufsz = strlen(tmp_buf);
                pass_msg_up(np->n, pu_h, tmp_buf, np->p.sock);
                #ifdef DEBUG
                printf("doing my duty, passing up childrens' nick along with mine: %s\n", tmp_buf);
                #endif
            }
            break;
        case HIER_ALERT:
            #ifdef DEBUG
            printf("got hier alert: %s\n", buf);
            #endif
            pthread_mutex_lock(&np->n->await_lock);
            if(np->n->awaiting_alert){
                /* TODO: take the expensive call to print_tree()
                 * out of the threadsafe section */
                #if 0
                oops. the root of the tree might mistakenly send multiple strings
                all but the last will be inaccurate. this is because
                leaves send their N_PASS_UP_ALERT and immediately after send their
                NICK_ALERT
                if one path is quicker than others, n_expected will be 1 
                and when that one is recieved, it will be spread.
                
                not a huge issue because expected is only set to 0 when a new
                HIER_REQ comes through

                we could solve this by keeping track of an dchecking when a paths_recvd == expected
                if we have spread the str already

                this system is so ridiculous... why send a N_PASS_UP_ALERT when it is essentially the same
                as a NICK_ALERT

                one parent at a time can confirm that all of their children have passed up their nicks

                ... what happens when > 1 users initialize a HIER_REQ simultaneously
                or before the root has done its job

                ok legit this will work:
                    leaves get request alert
                        they send N_PASS_UP_ALERT
                    
                    every non-leaf waits for n->n_children responses
                    then passes up the sum of all nums
                #endif

                print_tree(buf);
                np->n->awaiting_alert = 0;
            }
            pthread_mutex_unlock(&np->n->await_lock);
            spread_msg(np->n, header, buf, np->p.sock);
            break;
        default:{;}
    }
}

/* NOTE: THIS FUNCTION MAY ONLY BE CALLED WHEN `children_lock` IS ACQUIRED */
void remove_node(struct node* n, int ind){
    memset(&n->children[ind].addr, 0, sizeof(struct sockaddr_in));
    n->children[ind].sock = -1;
    memmove(n->children+ind, n->children+ind+1, 
            (n->n_children-ind-1)*sizeof(struct peer));
    --n->n_children;
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
                #if COLOR_SUPPORT
                printf("%sCONNECTION WITH PARENT HAS BEEN LOST%s\n", ANSI_RED, ANSI_RESET);
                #else
                puts("CONNECTION WITH PARENT HAS BEEN LOST");
                #endif
                memset(&np->n->parent.addr, 0, sizeof(struct sockaddr_in));
                np->n->parent.sock = -1;
                return NULL;
            }
            pthread_mutex_lock(&np->n->children_lock);
            for(int i = 0; i < np->n->n_children; ++i){
                if(peer_eq(np->p, np->n->children[i])){
                    remove_node(np->n, i);
                    pthread_mutex_unlock(&np->n->children_lock);
                    return NULL;
                }
            }
            pthread_mutex_unlock(&np->n->children_lock);
            /* TODO: when would this happen?
             * it seems like this occurs when we've /[k]icked
             * somebody
             * why?
             */
            return NULL;
        }

        buf[b_read] = 0;

        handle_msg(np, header, buf); 
        /*
         * struct mq_msg mqm;
         * mqm.mh = header;
         * memcpy(mqm.txt, buf, b_read);
         * mq_insert(np->n->mq, mqm);
        */

        /* TODO: handle b_read != sizeof(struct msg_header) */
        /* TODO: handle b_read != header.bufsz */

        /*spread_msg(np->n, header, buf, np->p.sock);*/
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
    socklen_t addrlen = sizeof(struct sockaddr_in);

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
    buf[i] = 0;
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

/* we don't store nicks so child must be specified by ip */
_Bool remove_child(struct node* n, char* child_ip){
    _Bool ret = 0;
    pthread_mutex_lock(&n->children_lock);
    struct sockaddr_in addr = strtoip(child_ip);
    for(int i = 0; i < n->n_children; ++i){
        if(n->children[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr){
            if(shutdown(n->children[i].sock, SHUT_RDWR) == -1)
                perror("shutdown()");
            close(n->children[i].sock);
            remove_node(n, i);
            ret = 1;
            break;
        }
    }
    pthread_mutex_unlock(&n->children_lock);
    return ret;
}

/* prints children and their IPs to FP */
/* TODO: nicks should also be printed, a single level pass down with spoofing
 * children into thinking they are leaves should do the trick
 */
void print_children(struct node* n, FILE* fp){
    pthread_mutex_lock(&n->children_lock);
    fprintf(fp, "%i children connected:\n", n->n_children);
    for(int i = 0; i < n->n_children; ++i){
        char* ip_str = inet_ntoa(n->children[i].addr.sin_addr);
        fprintf(fp, "  %i: %s\n", i, ip_str);
    }
    pthread_mutex_unlock(&n->children_lock);
}

void p_help(){
    printf(
         "%s|*******************************************************|%s\n"
         "%s|%s<text>        : send message                           %s|%s\n"
         "%s|%s/[k]ick <IP>  : kick direct child at <IP>              %s|%s\n"
         "%s|%s/[p]rint      : print tree structure of entire network %s|%s\n"
         "%s|%s/[c]children  : print children                         %s|%s\n"
         "%s|%s/[h]elp       : print this menu                        %s|%s\n"
         "%s\\*******************************************************/%s\n",

         #ifdef COLOR_SUPPORT
         ANSI_RED, ANSI_RESET, ANSI_RED, ANSI_RESET,
         ANSI_RED, ANSI_RESET, ANSI_RED, ANSI_RESET,
         ANSI_RED, ANSI_RESET, ANSI_RED, ANSI_RESET,
         ANSI_RED, ANSI_RESET, ANSI_RED, ANSI_RESET,
         ANSI_RED, ANSI_RESET, ANSI_RED, ANSI_RESET,
         ANSI_RED, ANSI_RESET, ANSI_RED, ANSI_RESET
         #else
         "", "", "", "", "", "",
         "", "", "", "", "", "",
         "", "", "", "", "", "",
         "", "", "", "", "","" 
         #endif
         );
}

int main(int a, char** b){
    /* ./egg <nick> <local ip>
     * ./egg <nick> <local ip> <remote ip>
     */
    if(a < 3){
        printf("egg v%s\nusage:\n  %s <nick> <local ip>\n  %s <nick> <local ip> <remote ip>\n",
               VER_STR, *b, *b);
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
        init_node(&n, b[1], any);
    }
    else init_node(&n, b[1], strtoip(b[2]));
    /*init_node(&n);*/
    pthread_t accept_th = spawn_accept_connections_thread(&n);
    if(a > 3){
        struct sockaddr_in addr = strtoip(b[3]);
        if(!join_tree(&n, addr))puts("failed to connect");
    }

    char buf[MSGLEN+1];

    /* TODO: header.nick should be inserted in helper functions
     * now that it's included in struct node
     */
    struct msg_header header;
    header.type = TEXT;
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
        if(*buf == '/'){
            switch(buf[1]){
                /* kick */
                case 'K':
                case 'k':
                    {
                    char* child = strchr(buf, ' ');
                    if(!child){
                        puts("this command requires a nickname to kick");
                        break;
                    }
                    ++child;
                    if(!remove_child(&n, child))printf("no child matching \"%s\" was found\n", child);
                    break;
                    }
                /* print */
                case 'P':
                case 'p':
                    /* when root of two node network initiats request, expected paths isn't set to 0
                     * TODO: investigate
                     */
                    pthread_mutex_lock(&n.expected_paths_lock);
                    *n.path_str = 0;
                    n.expected_paths = 0;
                    n.children_notified = 0;
                    n.paths_recvd = 0;
                    pthread_mutex_unlock(&n.expected_paths_lock);
                    init_diagram_request(&n);
                    break;
                case 'H':
                case 'h':
                    p_help();
                    break;
                /* TODO: add a /block command */
                case 'B':
                case 'b':
                    break;
                case 'C':
                case 'c':
                    /* TODO: allow optionally writing to a file */
                    print_children(&n, stdout);
                    break;
            }
        }
        else spread_msg(&n, header, buf, n.sock);
    }

    pthread_join(accept_th, NULL);
    return 0;
}
/* TODO:
 * should we be able to join a tree from within the program?
 * this would allow us to mend
 * i say only allow this is /[b]lock is implemented
*/
