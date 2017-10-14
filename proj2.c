#include <stdio.h>
#include <stdlib.h>

//======== ECE 465 - Project #2 ===================
//======== Mauricio Salazar, Hamza Tanveer ========


/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

//======== ECE 465 - Project #2 ===================
//======== Mauricio Salazar, Hamza Tanveer ========


int seqnum = 0;             //identifies the next packet in sequence to be sent
int acknum = 0;             //identifies the ack number for acknowledgement packets
int intransit = 0;          //the number of packets currently in transit from A to B
struct pkt prevpkt;

int expectedseqnum = 0;     //The expected packet sequence number at the receiver (B)
int expectedacknum = 0;     //The expected ack number at the sender (A)

int max_transit = 5;        //The N value of Go-Back-N; defines how many unacked packets can be in transit at any time.

int arrayIndex = 0;         //Points to the location in the packet array buffer directly after the most recently stored packet
int arrayLength = 10;       //The maximum length or size of the packet buffer.  This is incremented as needed at runtime.
struct pkt *pktArrayPtr;    //Pointer to the packet array buffer

int pktReadyNum = 0;        //This value counts the number of unique messages that have arrived.  Different from seqnum, because seqnum can can decrease when timeout occurs to allow "going back" and sending previous packets

float tWindow = 13.0;       //Value used for the timeout

sendPacket(){

    //Start timer
    starttimer(0, tWindow);

    //Acquire the correct packet to send from the buffer based on the index value seqnum
    struct pkt p = *(pktArrayPtr + seqnum);

    //Send the packet to layer 3
    tolayer3(0, p);

    printf("\n\tPacket # %d sent!", seqnum);

    //increment indices/counters
    intransit++;
    seqnum++;
}


/* called from layer 5, passed the data to be sent to other side */
A_output(message)
  struct msg message;
{

    //Called only once - this value creates and initializes the packet array buffer at a starting size of 10 packets
    if(arrayIndex == 0){
        pktArrayPtr = (struct pkt *)malloc(sizeof(struct pkt) * arrayLength);
        printf("\n\t===Allocated memory for %d packets in buffer!===", arrayLength);
    }

    //Check for null pointer - If null, exit
    if(pktArrayPtr == NULL){
        printf("Null pointer!");
        exit(0);
    }

    //Once the arraryIndex has been incremented to match the arrayLength, the buffer is full, so allocate more space.
    if(arrayIndex == arrayLength){
        //resize array
        printf("\n\t===Increasing buffer size to accomodate additional packets!===!");
        pktArrayPtr = (struct pkt*) realloc(pktArrayPtr, sizeof(struct pkt) * (arrayLength + 5));
        //arbitrary value of 5 added to buffer size
        arrayLength += 5;
    }


    //create packet corresponding to received message from layer 5, assign seqnum, acknum, and checksum suggested by book authors
    struct pkt p;
    p.seqnum = pktReadyNum;
    p.acknum = acknum;
    p.checksum = p.seqnum + p.acknum;


    //copy data to packet payload
    int i;
    int j = 0;
    for(i=0;i<20;i++){
        j = (int) message.data[i];
        p.checksum += j;
        p.payload[i] = message.data[i];
    }

    //After creating the packet, store it in the packet buffer and increment the arrayIndex
    printf("\n\t===Placing packet # %d in buffer at index %d\n===", p.seqnum, arrayIndex);
    *(pktArrayPtr + arrayIndex) = p;
    arrayIndex++;

    //Check to see if max number of allowable packets are unacked.  If not, send one.
    if(intransit == max_transit){
        //reached max N
        printf("\n\t===Max # in transit reached!===");
    } else {
        sendPacket();
    }

    pktReadyNum++;
}

B_output(message)  /* need be completed only for extra credit */
  struct msg message;
{
}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(packet)
  struct pkt packet;
{
    //A packet has arrived from B! It must be an ack (with bidirectional = 0)
    printf("\n\t===A received ack from B!===\n\tSeq: %d\n\tAck: %d\n\tCheck: %d\n", packet.seqnum, packet.acknum, packet.checksum);

    //Make sure the ack received is the next one expected, so as to continue linearly through the list of packets
    if(packet.acknum == expectedacknum){
        printf("\n\t===Ack was in order!===");
        printf("\n\tAcknum: %d\n\tSeqnum: %d\n\tChecksum: %d", packet.acknum, packet.seqnum, packet.checksum);
        int tempchecksum = packet.acknum + packet.seqnum;

        //If the checksum of the ack is correct, it has not been corrupted!
        if(tempchecksum == packet.checksum){
            printf("\n\tAck # %d is correct!", packet.acknum);
            //Once an ack arrives, the RTT for communication is over, so reduce number of currently in transit packets
            intransit--;
            expectedacknum++;

            //If the last packet out to be delivered arrives, then no more packets are out, so the timer shouldn't be counting
            if(intransit == 0){
                stoptimer(0);
            }

            //Once an ack arrives, there may be packets waiting to be sent from the packet buffer that could not due to max_transit.  So check to see if there are any.  If so, send.
            if (arrayIndex > expectedacknum + intransit){
                printf("\n\tRemoving packet # %d from queue.\n", expectedacknum + intransit);
                sendPacket();
            }
        } else {
            printf("\n\tAck was corrupted! Got checksum %d, expected %d", tempchecksum, packet.checksum);
        }
    }
}

/* called when A's timer goes off */
A_timerinterrupt()
{
    //Called when timeout occurs
    //Once this happens, seqnum should be reset to the currently expected ack num, since this is the oldest packet that has not been acked.  Seqnum identifies the "starting point" of the window of packets to be sent.
    seqnum = expectedacknum;
    expectedseqnum = seqnum;
    acknum = seqnum;
    //Reset to 0
    intransit = 0;
    printf("\nTimeout! Resending packets %d through %d!", seqnum, arrayIndex-1);

    int i;

    stoptimer(0);
    //When a timeout occurs, the timeout wait increases.  Arbitrarily chosen, and probably not even necessary.
    tWindow += 12.0;

    //Start the timer again
    starttimer(0, tWindow);

    //Loop through and send packets from seqnum to arrayIndex, stopping once max_transit has been reached.
    for(i=seqnum;i< arrayIndex;i++){
        if(intransit < max_transit){
            sendPacket();
        } else {
            printf("\n\t==Max Transit Reached. Packet # %d will be sent later.", i);
        }
    }

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
A_init()
{
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
B_input(packet)
  struct pkt packet;
{
    printf("\n\tLOG: B received packet # %d from A!\n\tCheck: %d\n", packet.seqnum);


    //Check to see if the packet is in order - ie, the next one expected by the receiver
    if(packet.seqnum == expectedseqnum){
        printf("\n\tPacket %d is in order!", packet.seqnum);
        int i, j;
        int tempchecksum = packet.seqnum + packet.acknum;

        //Sum the casted int values from ascii characters to derive the checksum (method suggested by book's authors)
        for(i=0;i<20;i++){
            j = (int) packet.payload[i];
            tempchecksum += j;
        }

        //If the checksum matches, then the packet is not corrupt
        if (tempchecksum == packet.checksum){
            printf("\n\tChecksum is correct!");
            //increment to next expected packet
            expectedseqnum++;


            //create a message from the received packet
            struct msg m;
            int i;
            for(i=0;i<20;i++){
                m.data[i] = packet.payload[i];
            }

            //send the created message back up to layer 5
            tolayer5(1, m.data);

            //create the ack packet that is to be sent back to the sender
            struct pkt ack;
            ack.seqnum = -1;
            ack.acknum = acknum;
            ack.checksum = ack.seqnum + ack.acknum;

            //Send the ack back to the sender
            tolayer3(1, ack);
            printf("\n\tAck # %d sent!", acknum);
            acknum++;
        } else {
            //packet is corrupt
            printf("\n\tLOG: Pkt was corrupt! Got checksum %d, expected %d", tempchecksum, packet.checksum);
        }

    } else {
        //packet is out of order
        printf("\n\tLOG: Pkt was out of order! Got seqnum %d, expected seqnum: %d",packet.seqnum, expectedseqnum);
    }

}

/* called when B's timer goes off */
B_timerinterrupt()
{

}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
B_init()
{
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;

   int i,j;
   char c;

   init();
   A_init();
   B_init();

   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i=0; i<20; i++)
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++)
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A)
               A_output(msg2give);
             else
               B_output(msg2give);
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}



init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();


   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" );
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
  double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
   // char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
}


insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime);
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q;
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 //char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }

/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
}


/************************** TOLAYER3 ***************/
tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 //char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)
	printf("          TOLAYER3: packet being lost\n");
      return;
    }

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) )
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();



 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)
	printf("          TOLAYER3: packet being corrupted\n");
    }

  if (TRACE>2)
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
}

tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)
        printf("%c",datasent[i]);
     printf("\n");
   }

}

