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

struct destination {
  int hop[10];
  int hop_index;
};

struct forward_table {
  struct destination dest[10];
  int cost[10]; 
};

struct distance_table
{
    int **costs;     // the distance table of curr_node, costs[i][j] is the cost from node i to node j
};

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
struct forward_table *f_table;
int **link_costs; /*This is a 2D matrix stroing the content defined in topo file*/
int **new_link_costs;
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

 

void rtinit(struct distance_table *dt, int node, int *link_costs, int num_nodes, struct forward_table *ft)
{
  for(int i=0; i<num_nodes; i++){
    for(int j=0; j<num_nodes; j++){
      if(i == node){
        dt->costs[node][j] = link_costs[j];

        ft->cost[j] = link_costs[j];
        ft->dest[j].hop_index = 0;
        ft->dest[j].hop[ft->dest[j].hop_index] = j;
        
        if(link_costs[j] == -1)
          ft->dest[j].hop[ft->dest[j].hop_index] = -1;
        else
          ft->dest[j].hop[ft->dest[j].hop_index] = j;

        // printf(" %d ", dt->costs[node][j]);
        // printf(" %d ", ft->cost[j]);
        // printf(" %d ",ft->dest[j].hop[ft->dest[j].hop_index]);
        // printf("Index value:%d \n", ft->dest[j].hop_index);
        // fflush(stdout);

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
      if(dest == i)
        continue; 

      int min_cur = myNodeDts[dest][i];

      for(int j=0;j<num_nodes;j++){
          
        if(new_link_costs[dest][j]!=-1 && myNodeDts[j][i]!=-1 && dest != j){

          if((min_cur > new_link_costs[dest][j] + myNodeDts[j][i]) || min_cur==-1){
            min_cur = new_link_costs[dest][j] + myNodeDts[j][i] ;

            
            if(f_table[dest].dest[i].hop_index > 0){
              for(int l=0;l< f_table[dest].dest[i].hop_index; l++){
                f_table[dest].dest[i].hop[l] = -1; 
              }
              f_table[dest].dest[i].hop_index = 0;
            }

            // printf("Hop index from %d to %d:%d\n", dest, i, f_table[dest].dest[i].hop_index);
            // printf("Hop value from %d to %d:%d\n", dest, i, f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index]);

            f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index++] = j;

            if(f_table[j].dest[i].hop_index > 0 ){
              for(int m=0;m<f_table[j].dest[i].hop_index; m++){
              f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index++] = f_table[j].dest[i].hop[m]; 
            }
            }


            f_table[dest].cost[i] =  min_cur;

            // printf("source : %d ", dest);
            // printf("dest: %d ", i);
            // printf("hop: %d ", j);
            // printf("cost: %d ", min_cur);
            // printf("\nIndex value of %d to %d is %d ", dest, i, f_table[dest].dest[i].hop_index);
            // printf("\n");

            flag = 1;

          } else if(min_cur == new_link_costs[dest][j] + myNodeDts[j][i]) {

            if(j == i){

            // printf("Hop index from %d to %d:%d\n", dest, i, f_table[dest].dest[i].hop_index);
            // printf("Hop value from %d to %d:%d\n", dest, i, f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index]);

              if(f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index] != j){

                  f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index++] = j ; 
                } 
                
             } else if(f_table[dest].dest[i].hop[0]>j){

                if(f_table[dest].dest[i].hop_index > 0){
                  for(int l=0;l< f_table[dest].dest[i].hop_index; l++){
                    f_table[dest].dest[i].hop[l] = -1; 
                  }
                  f_table[dest].dest[i].hop_index = 0;
                }

                f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index++] = j;

                if(f_table[j].dest[i].hop_index > 0 ) {
                  for(int m=0;m<f_table[j].dest[i].hop_index; m++){
                  f_table[dest].dest[i].hop[f_table[dest].dest[i].hop_index++] = f_table[j].dest[i].hop[m]; 
                  }
                }

              }
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
    FILE *file2 = fopen(argv[3],"r");
    cur_simulation = 0;
    int count = 0;
    char ch;
    char line[1024] ;
    char* costs[1024]; 
    int index = 0; 
    char line2[1024]={0};
    char* routes[1024];
    int num_rows_in_traffic = 0;

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

  // calculating num of rows in traffic 
  index = 0;

  if (file2 == NULL)
  {
        printf("Couldn't open file\n");
        return 1;
  }
  
    for (char ch = getc(file2); ch != EOF; ch = getc(file2)){
    if (ch == '\n') {
      num_rows_in_traffic++;
      line2[index++] = ' ';
    } else {
      line2[index++] = ch;
      
    }
  }
    num_rows_in_traffic = num_rows_in_traffic + 1;
    // printf("no of nodes in traffic:%d\n",num_rows_in_traffic);
    // printf("line2:%s\n", line2);

    char *p1 = strtok (line2, " ");
    index = 0;
    while (p1 != NULL)
    {
        routes[index++] = p1;
        p1 = strtok (NULL, " ");
    }

    // for(int i=0;i<index;i++){
    //   printf(" %s\n",routes[i]);
    // }

    dts = (struct distance_table *) malloc(num_nodes * sizeof(struct distance_table));
    link_costs = (int **) malloc(num_nodes * sizeof(int *));
    new_link_costs = (int **) malloc(num_nodes * sizeof(int *));
    int **src_dest_packets = (int **) malloc(num_rows_in_traffic * sizeof(int *));
    c_dts = (struct distance_table *) malloc(num_nodes * sizeof(struct distance_table));
    f_table = (struct forward_table *) malloc(num_nodes * sizeof(struct forward_table));


    for (int i = 0; i < num_nodes; i++)
    {
        link_costs[i] = (int *)malloc(num_nodes * sizeof(int));
        new_link_costs[i] = (int *)malloc(num_nodes * sizeof(int));
    }

    for (int i = 0; i < num_rows_in_traffic; i++)
    {
        src_dest_packets[i] = (int *)malloc(3 * sizeof(int));
    }

    index = 0;
    for(int i=0;i<num_nodes;i++){
      for(int j=0;j<num_nodes;j++){
        link_costs[i][j] = atoi(costs[index++]);
        new_link_costs[i][j] = link_costs[i][j];
      }
    }

    index = 0;
    for(int i=0;i<num_rows_in_traffic;i++){
      for(int j=0;j<3;j++){
        src_dest_packets[i][j] = atoi(routes[index++]);
      }
    }
  
    // for(int i=0;i<num_rows_in_traffic;i++){
    //   for(int j=0;j<3;j++){
    //     printf(" %d ",src_dest_packets[i][j]);
    //   }
    //   printf("\n");
    // }

    for (int i = 0; i < num_nodes; i++) {
      dts[i].costs = (int **) malloc(num_nodes * sizeof(int *));
      // f_table[i].cost = (int *) malloc(num_nodes * sizeof(int)); 
      // f_table[i].dest = (struct destination * )malloc(num_nodes * sizeof(struct destination));
      c_dts[i].costs = (int **) malloc(num_nodes * sizeof(int *));
      
    for (int j = 0; j < num_nodes; j++) {
        dts[i].costs[j] = (int *) malloc(num_nodes * sizeof(int));
        // f_table[i].dest[j].hop = (int *) malloc(num_nodes * sizeof(int));
        c_dts[i].costs[j] = (int *) malloc(num_nodes * sizeof(int));
    }
  }

    for (int i = 0; i < num_nodes; i++){
        rtinit(&dts[i], i, link_costs[i], num_nodes, &f_table[i]);
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
          printf("\nSimulator terminated at t=%f, no packets in medium\n", clocktime);
        
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

      int drop_packet[num_rows_in_traffic];

      for(int i=0;i<num_rows_in_traffic;i++){
        int src = 0, dest = 0, hop = 0, hop_set = 0, drop_set = 0;
        int visited[num_nodes];

        for(int k=0;k<num_nodes;k++){
          visited[k] = 0;
        }

        drop_packet[i] = 0;

        for(int j=0;j<3;j++){
          if(j == 0){
            src = src_dest_packets[i][j];
          } else if (j == 1){
            dest = src_dest_packets[i][j];
          }
          printf(" %d ",src_dest_packets[i][j]);
        }
        printf("%d>",src);
        visited[src] = 1; 
        for(int k=0;k< f_table[src].dest[dest].hop_index ; k++){
          hop = f_table[src].dest[dest].hop[k];
          if(visited[hop] == 0 ){
            hop_set = 1;
            printf("%d>",hop);
            visited[hop] = 1;
          } else {
            printf("%d(drop)",hop);
            drop_set = 1;
            drop_packet[i] = 1;
            break; 
          }
          fflush(stdout);
        }
        if(hop !=dest && hop_set ==1 && drop_set == 0)
          printf("%d",dest);
        else if(drop_set == 0)
          printf("%d",dest);

        printf("\n");
      }



      if(cur_simulation>0){

        for(int i=0;i<num_nodes;i++){
          for(int j=0;j<num_nodes;j++){
            if(new_link_costs[i][j]!= -1)
              new_link_costs[i][j] = 0;
          }
        }

        for(int i=0;i<num_rows_in_traffic;i++){

            if(drop_packet[i] == 1)
              continue; 

          int src = src_dest_packets[i][0];
          int dest = src_dest_packets[i][1];
          int packet = src_dest_packets[i][2];
          int hop = f_table[src].dest[dest].hop[0];

          // printf("first for loop:%d, %d, %d\n",src, hop, dest);

          if(hop == dest){
              if(new_link_costs[dest][src] == 0 && new_link_costs[src][dest] == 0 ){
                new_link_costs[dest][src] = packet;
                new_link_costs[src][dest] = packet;
              }
              else {
                new_link_costs[src][dest] += packet;
                new_link_costs[dest][src] += packet;
              }
                
            } else {
              if(new_link_costs[hop][src] == 0 && new_link_costs[src][hop] == 0){
                new_link_costs[hop][src] = packet;
                new_link_costs[src][hop] = packet ;
              }
              else{
                new_link_costs[src][hop] += packet;      
                new_link_costs[hop][src] += packet;      
              }
                        
            }

          if(f_table[src].dest[dest].hop_index > 1){
            for(int k=1;k< f_table[src].dest[dest].hop_index ; k++) {
              hop = f_table[src].dest[dest].hop[k];
              int hop_1 = f_table[src].dest[dest].hop[k-1];
              if(new_link_costs[hop][hop_1] == 0 && new_link_costs[hop_1][hop] == 0){
                new_link_costs[hop][hop_1] = packet ; 
                new_link_costs[hop_1][hop] = packet;
              }
              else  {
                new_link_costs[hop_1][hop] += packet ; 
                new_link_costs[hop][hop_1] += packet ; 
              }
                
            }
          } else {
              if(hop!=dest){
                if(new_link_costs[dest][hop] == 0 && new_link_costs[hop][dest] == 0){
                  new_link_costs[dest][hop] = packet;
                  new_link_costs[hop][dest] = packet ;
                }
                else{
                  new_link_costs[hop][dest] += packet;  
                  new_link_costs[dest][hop] += packet;  
                }
              }
            }

      // printf("New Link Costs:\n");
      // for(int x=0;x<num_nodes;x++){
      //   for(int y=0;y<num_nodes;y++){
      //     printf("%d ", new_link_costs[x][y]);
      //   }
      //   printf("\n");
      // }


        }

      // printf("New Link Costs:\n");
      // for(int i=0;i<num_nodes;i++){
      //   for(int j=0;j<num_nodes;j++){
      //     printf("%d ", new_link_costs[i][j]);
      //   }
      //   printf("\n");
      // }

      for(int i=0;i<num_nodes;i++){
        for(int j=0;j<num_nodes;j++){
          if(dts[i].costs[i][j] != new_link_costs[i][j]){
            dts[i].costs[i][j] = new_link_costs[i][j] ; 
            c_dts[i].costs[i][j] = new_link_costs[i][j] ; 

            f_table[i].cost[j] = new_link_costs[i][j];
            f_table[i].dest[j].hop_index = 0;
            
            if(new_link_costs[i][j] == -1)
              f_table[i].dest[j].hop[f_table[i].dest[j].hop_index] = -1;
            else
              f_table[i].dest[j].hop[f_table[i].dest[j].hop_index] = j;
          }
        }
      }

    }
       cur_simulation++;
    }

   



return 0;

}





