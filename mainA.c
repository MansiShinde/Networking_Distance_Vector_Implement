 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>

/* a rtpkt is the packet sent from one router to
   another*/
struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent 
                         (must be an directly connected neighbor) */
  int *mincost;    /* min cost to all the node  */
  };

struct distance_table
{
    int **costs;     // the distance table of curr_node, costs[i][j] is the cost from node i to node j
};



/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 2 and below network environment:
  - emulates the transmission and delivery (with no loss and no
    corruption) between two physically connected nodes
  - calls the initializations routine rtinit once before
    beginning emulation for each node.

You should read and understand the code below. For Part A, you should fill all parts with annotation starting with "Todo". For Part B and Part C, you need to add additional routines for their features.
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity; 
   int simulation;          /* entity (node) where event occurs */
   struct rtpkt *rtpktptr; /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */
struct distance_table *dts;
struct distance_table *c_dts;
int **link_costs; /*This is a 2D matrix stroing the content defined in topo file*/
int num_nodes;
int cur_simulation;

/* possible events: */
/*Note in this lab, we only have one event, namely FROM_LAYER2.It refer to that the packet will pop out from layer3, you can add more event to emulate other activity for other layers. Like FROM_LAYER3*/
#define  FROM_LAYER2     1

float clocktime = 0.000;

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
float jimsrand() 
{
  double mmm = 2147483647;   
  float x;                   
  x = rand()/mmm;            
  return(x);
}  


void insertevent(struct event *p)
{
   struct event *q,*qold;

   struct event *p_copy = (struct event *)malloc(sizeof(struct event));
   struct rtpkt *pkt = (struct rtpkt *)malloc(sizeof(struct rtpkt));
   pkt->destid = p->rtpktptr->destid;
   pkt->sourceid = p->rtpktptr->sourceid;
   pkt->mincost = p->rtpktptr->mincost;
   p_copy->rtpktptr = pkt;
   p_copy->eventity = p->eventity;
   p_copy->evtime = p->evtime;
   p_copy->evtype = p->evtype;
   p_copy->next = p->next;
   p_copy->prev = p->prev;
   p_copy->simulation = p->simulation;

   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p_copy;
        p_copy->next=NULL;
        p_copy->prev=NULL;
        p->next = NULL;
        p->prev = NULL;
        }
     else {
        if(p_copy->simulation > q->simulation){
          p_copy->evtime = p_copy->evtime +p_copy->simulation;
        }

        for (qold = q; q !=NULL && p_copy->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p_copy;
             p_copy->prev = qold;
             p_copy->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p_copy->next=evlist;
             p_copy->prev=NULL;
             p_copy->next->prev=p;
             evlist = p_copy;
             }
           else {     /* middle of list */
             p_copy->next=q;
             p_copy->prev=q->prev;
             q->prev->next=p_copy;
             q->prev=p_copy;
             }
         }
}


void printevlist()
{
  struct event *q;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d ",q->evtime,q->evtype,q->eventity);
    printf(" Packet details: %d\t%d ",q->rtpktptr->sourceid,q->rtpktptr->destid);
    printf(" Simulations Value: %d\n",q->simulation);
    }
  printf("--------------\n");
}


/************************** send update to neighbor (packet.destid)***************/
void send2neighbor(struct rtpkt packet)
{
 struct event *evptr, *q;
 float jimsrand(),lastime;

//  struct rtpkt *packet_copy = (struct rtpkt*)malloc(sizeof(struct rtpkt));
//  packet_copy = &packet;

 /* be nice: check if source and destination id's are reasonable */
 if (packet.sourceid<0 || packet.sourceid >num_nodes) {
   printf("WARNING: illegal source id in your packet, ignoring packet!\n");
   return;
   }
 if (packet.destid<0 || packet.destid > num_nodes) {
   printf("WARNING: illegal dest id in your packet, ignoring packet!\n");
   return;
   }
 if (packet.sourceid == packet.destid)  {
   printf("WARNING: source and destination id's the same, ignoring packet!\n");
   return;
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER2;   /* packet will pop out from layer3 */
  evptr->eventity = packet.destid; /* event occurs at other entity */
  evptr->rtpktptr = &packet;       /* save ptr to my copy of packet */
  evptr->simulation = cur_simulation ; 

/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = clocktime;
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER2  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  cur_simulation + (jimsrand()/2);
 insertevent(evptr);
} 

 

void rtinit(struct distance_table *dt, int node, int *link_costs, int num_nodes)
{
    /* Todo: Please write the code here*/

  for(int i=0; i<num_nodes; i++){
    for(int j=0; j<num_nodes; j++){
      if(i == node){
        dt->costs[node][j] = link_costs[j];

        if(dt->costs[node][j] != -1 && dt->costs[node][j]!=0){
          struct rtpkt packet;
          packet.sourceid = node;
          packet.destid = j;
          packet.mincost = dt->costs[node] ; 

          send2neighbor(packet);
      }

    }
    else if(i == j){
      dt->costs[i][j] = 0;
    }
    else{
      dt->costs[i][j] = -1;
    }
    }
  }
}


void rtupdate(struct distance_table *dt, struct rtpkt recv_pkt)
{
    /* Todo: Please write the code here*/

    int source = recv_pkt.sourceid;
    int dest = recv_pkt.destid;
    int flag = 0;

    int **myNodeDts = (int **) malloc(num_nodes * sizeof(int *));
    for(int i =0;i<num_nodes;i++) {
        myNodeDts[i] = (int *)malloc(num_nodes * sizeof(int));
    }

    for(int i=0;i<num_nodes;i++){
      dt->costs[source][i] = c_dts[source].costs[source][i];
    }

    for(int i =0;i<num_nodes;i++){
      for(int j = 0;j<num_nodes;j++){
        myNodeDts[i][j] = dt->costs[i][j];
      }
    }


    for(int i=0;i<num_nodes;i++){ 
      int min_cur = myNodeDts[dest][i];
      for(int j=0;j<num_nodes;j++){
        if(link_costs[dest][j]!=-1 && myNodeDts[j][i]!=-1){
          if((min_cur > link_costs[dest][j] + myNodeDts[j][i]) || min_cur==-1){
            min_cur = link_costs[dest][j] + myNodeDts[j][i] ;
            flag = 1;
          }
        }
     }
     dt->costs[dest][i] = min_cur;
   }

    if(flag == 1){
        for(int i=0;i<num_nodes;i++){
          if(dt->costs[dest][i]!=-1 && dest!=i){
            struct rtpkt packet;
            packet.sourceid = dest;
            packet.destid =i;
            packet.mincost = dt->costs[dest] ; 

            send2neighbor(packet);
          }
        }
    }

}


int main(int argc, char *argv[])
{
    struct event *eventptr;

    /* Todo: Please write the code here to process the input. 
    Given different flag, you have different number of input for part A, B, C. 
    Please write your own code to parse the input for each part. 
    Specifically, in part A you need parse the input file and get “num_nodes”, 
    and fill in the content of dts and link_costs */
    int k_max = atoi(argv[1]);
    FILE *file = fopen(argv[2], "r") ; 
    int k = 0;
    int count = 0;
    char ch;
    char line[1024] ;
    char* costs[1024]; 
    int index = 0; 

    if (file == NULL)
    {
        printf("Couldn't open file\n");
        return 1;
    }
    
    for (ch = getc(file); ch != EOF; ch = getc(file)){
      if (ch == '\n') {
        num_nodes++;
        line[index++] = ' ';
      } else {
        line[index++] = ch;
      }
    }
    num_nodes = num_nodes + 1;

    char *p = strtok (line, " ");
    index = 0;
    while (p != NULL)
    {
        costs[index++] = p;
        p = strtok (NULL, " ");
    }

    dts = (struct distance_table *) malloc(num_nodes * sizeof(struct distance_table));
    link_costs = (int **) malloc(num_nodes * sizeof(int *));
    c_dts = (struct distance_table *) malloc(num_nodes * sizeof(struct distance_table));

    for (int i = 0; i < num_nodes; i++)
    {
        link_costs[i] = (int *)malloc(num_nodes * sizeof(int));
    }

    index = 0;
    for(int i=0;i<num_nodes;i++){
      for(int j=0;j<num_nodes;j++){
        link_costs[i][j] = atoi(costs[index++]);
      }
    }

    for (int i = 0; i < num_nodes; i++) {
      dts[i].costs = (int **) malloc(num_nodes * sizeof(int *));
      c_dts[i].costs = (int **) malloc(num_nodes * sizeof(int *));
      
    for (int j = 0; j < num_nodes; j++) {
        dts[i].costs[j] = (int *) malloc(num_nodes * sizeof(int));
        c_dts[i].costs[j] = (int *) malloc(num_nodes * sizeof(int));
    }
  }

    for (int i = 0; i < num_nodes; i++){
        rtinit(&dts[i], i, link_costs[i], num_nodes);
    }

    // copying dts data to copydts

    for(int i = 0;i<num_nodes;i++){
      for(int a= 0;a<num_nodes;a++){
        for (int b = 0; b < num_nodes; b++)
        {
          c_dts[i].costs[a][b] = dts[i].costs[a][b];
        }
        
      }
    } 

    while(cur_simulation<=k_max) {
      if(cur_simulation>=1){

        while (1) {
            /* Todo: Please write the code here to handle the update of time slot k (We assume that in one slot k, the traffic can go through all the routers to reach the destination router)*/
            //printevlist(); 
            eventptr = evlist;            /* get next event to simulate */
            if (eventptr==NULL){
                goto terminate;
            }

            if(eventptr->simulation!=cur_simulation-1){
              goto donotend;
            }

            evlist = evlist->next;        /* remove this event from event list */
            if (evlist!=NULL)
              evlist->prev=NULL;
            clocktime = eventptr->evtime;    /* update time to next event time */
            if (eventptr->evtype == FROM_LAYER2 ) {
                /* Todo: You need to modify the rtupdate method and add more codes here for Part B and Part C, since the link costs in these parts are dynamic.*/
                
                rtupdate(&dts[eventptr->eventity], *(eventptr->rtpktptr));
            }
            else 
            {
                printf("Panic: unknown event type\n"); exit(0);
            }
            if (eventptr->evtype == FROM_LAYER2 ) {
                free(eventptr->rtpktptr);        /* free memory for packet, if any */
            }
            
            free(eventptr);                    /* free memory for event struct   */
        }
        terminate:
          //printf("\nSimulator terminated at t=%f, no packets in medium\n", clocktime);
        
        donotend:
          printf("");
          for(int i = 0;i<num_nodes;i++){
            for(int a= 0;a<num_nodes;a++){
              for (int b = 0; b < num_nodes; b++)
                {
                  c_dts[i].costs[a][b] = dts[i].costs[a][b];
                }
              }
            }

      }
    
      //print the contents here
      printf("k=%d:\n",cur_simulation);
      for(int i = 0; i < num_nodes;i++){
        printf("node-%d: ",i);
        for(int j = 0; j<num_nodes;j++){
          printf("%d ",dts[i].costs[i][j]);
        }
        printf("\n");
      }

      if(cur_simulation == 4){
        cur_simulation = 10; 
      }else if(cur_simulation == 10)
        cur_simulation += 10;
      else 
        cur_simulation++;
    }


return 0;

}





