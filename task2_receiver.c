/*
* CS4222/5422: Assignment 3b
* Perform neighbour discovery
*/

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "net/linkaddr.h"
#include <string.h>
#include <stdio.h> 
#include "node-id.h"

// Identification information of the node


// Configures the wake-up timer for neighbour discovery 
#define WAKE_TIME RTIMER_SECOND/10    // 10 HZ, 0.1s

#define SLEEP_CYCLE  4       	        // 0 for never sleep
#define SLEEP_SLOT RTIMER_SECOND/10   // sleep slot should not be too large to prevent overflow

// For neighbour discovery, we would like to send message to everyone. We use Broadcast address:
linkaddr_t dest_addr;

#define NUM_SEND 2
/*---------------------------------------------------------------------------*/
typedef struct {
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;
  unsigned long light_readings[10]; 
  
} data_packet_struct;

/*---------------------------------------------------------------------------*/
// duty cycle = WAKE_TIME / (WAKE_TIME + SLEEP_SLOT * SLEEP_CYCLE)
/*---------------------------------------------------------------------------*/

// sender timer implemented using rtimer
static struct rtimer rt;

// Protothread variable
static struct pt pt;

// Structure holding the data to be transmitted
static data_packet_struct data_packet;

// Current time stamp of the node
unsigned long curr_timestamp;
unsigned long prox_timestamp;
unsigned long out_of_prox_timestamp;

// Proximity flag
bool in_prox = false;

// Global state
bool is_detect_state = false;

// Starts the main contiki neighbour discovery process
PROCESS(nbr_discovery_process, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&nbr_discovery_process);

// Function called after reception of a packet
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) 
{

  // Check if the received packet size matches with what we expect it to be

  if(len == sizeof(data_packet)) {

 
    static data_packet_struct received_packet_data;
    
    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);

    signed short rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);

    // check if node has been detected in proximity
    if (!in_prox) {

      // node has come into proximity
      if (rssi >= -70) {

        // set proximity flag
        in_prox = true;

        // start proximity counter
        prox_timestamp = clock_time();
      } 
      // node is still not in proximity
      // check if node has been out of proximity for 30s
      else if (is_detect_state && clock_time() - out_of_prox_timestamp >= 3840) {
        // change state to absent
        is_detect_state = false;
        printf("%ld ABSENT %ld", out_of_prox_timestamp, received_packet_data.src_id);
      }
    }
    // node already detected in proximity 
    else {

      // node is now out of proximity
      if (rssi < -70) {

        // clear proximity flag
        in_prox = false;

        out_of_prox_timestamp = clock_time();
      
      // node is still in proximity
      // check if node has been in proximity for 15s
      } else if (!is_detect_state && clock_time() - prox_timestamp >= 1920) {
        // change state to detect
        is_detect_state = true;
        printf("%ld DETECT %ld", prox_timestamp, received_packet_data.src_id);

        // TODO send request for light sensor data
      }
    }
    

    // // Print the details of the received packet
    // printf("Received neighbour discovery packet %lu with rssi %d from %ld, %ld since reboot\n", 
    //   received_packet_data.seq, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),
    //   received_packet_data.src_id,
    //   received_packet_data.timestamp);

    printf("\n");

    for (int i = 0; i < 10; i++) {
      printf("%ld, ", received_packet_data.light_readings[i]);
    }

    printf("\n");
 
  }

}

// Scheduler function for the sender of neighbour discovery packets
char sender_scheduler(struct rtimer *t, void *ptr) {
 
  static uint16_t i = 0;
  
  static int NumSleep=0;
 
  // Begin the protothread
  PT_BEGIN(&pt);

  // Get the current time stamp
  curr_timestamp = clock_time();

  while(1){

    // radio on
    NETSTACK_RADIO.on();

    // send NUM_SEND number of neighbour discovery beacon packets
    for(i = 0; i < NUM_SEND; i++){

      
     
       // Initialize the nullnet module with information of packet to be trasnmitted
      nullnet_buf = (uint8_t *)&data_packet; //data transmitted
      nullnet_len = sizeof(data_packet); //length of data transmitted
      
      data_packet.seq++;
      
      curr_timestamp = clock_time();
      
      data_packet.timestamp = curr_timestamp;

      // printf("Send seq# %lu  @ %8lu ticks   %3lu.%03lu\n", data_packet.seq, curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

      NETSTACK_NETWORK.output(&dest_addr); //Packet transmission
      

      // wait for WAKE_TIME before sending the next packet
      if(i != (NUM_SEND - 1)){

        rtimer_set(t, RTIMER_TIME(t) + WAKE_TIME, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
      
      }
    }

    // // sleep for a random number of slots
    // if(SLEEP_CYCLE != 0){

    //   // SLEEP_SLOT cannot be too large as value will overflow,
    //   // to have a large sleep interval, sleep many times instead

    //   // get a value that is uniformly distributed between 0 and 2*SLEEP_CYCLE
    //   // the average is SLEEP_CYCLE 
    //   unsigned long curr_sleep_cycle = sleep_cycle;
    //   if (cycles_since_last_received != 0)
    //   {
    //     curr_sleep_cycle /= (cycles_since_last_received * 2);
    //   }
    //   NumSleep = random_rand() % (2 * curr_sleep_cycle + 1);  
    //   // printf(" Sleep for %d slots \n",NumSleep);

    //   // NumSleep should be a constant or static int
    //   for(i = 0; i < NumSleep; i++){
    //     rtimer_set(t, RTIMER_TIME(t) + SLEEP_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
    //     PT_YIELD(&pt);
    //   }

    // }

    // radio off
    NETSTACK_RADIO.off();

    // SLEEP_SLOT cannot be too large as value will overflow,
    // to have a large sleep interval, sleep many times instead

    // get a value that is uniformly distributed between 0 and 2*SLEEP_CYCLE
    // the average is SLEEP_CYCLE 
    // unsigned long curr_sleep_cycle = sleep_cycle;
    // if (cycles_since_last_received != 0)
    // {
    //   curr_sleep_cycle /= (cycles_since_last_received * 2);
    // }
    // NumSleep = random_rand() % (curr_sleep_cycle + 1);  
    // cycles_since_last_received++;
    // printf(" Sleep for %d slots \n",NumSleep);

    NumSleep = SLEEP_CYCLE;

    // NumSleep should be a constant or static int
    for(i = 0; i < NumSleep; i++){
      rtimer_set(t, RTIMER_TIME(t) + SLEEP_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
      PT_YIELD(&pt);
    }
  }
  
  PT_END(&pt);
}


// Main thread that handles the neighbour discovery process
PROCESS_THREAD(nbr_discovery_process, ev, data)
{

 // static struct etimer periodic_timer;

  PROCESS_BEGIN();

    // initialize data packet sent for neighbour discovery exchange
  data_packet.src_id = node_id; //Initialize the node ID
  data_packet.seq = 0; //Initialize the sequence number of the packet
  
  nullnet_set_input_callback(receive_packet_callback); //initialize receiver callback
  linkaddr_copy(&dest_addr, &linkaddr_null);

  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending packet of size %d Bytes\n", node_id, (int)sizeof(data_packet_struct));

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  

  PROCESS_END();
}
