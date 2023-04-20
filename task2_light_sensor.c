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
#include "board-peripherals.h"
#include <limits.h>
#include "sys/etimer.h"

// Identification information of the node


// Configures the wake-up timer for neighbour discovery 
#define WAKE_TIME RTIMER_SECOND/10    // 10 HZ, 0.1s

#define SLEEP_CYCLE  1       	        // 0 for never sleep
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
// static struct rtimer light_sensor_rt;

// Protothread variable
static struct pt pt;
// static struct pt light_sensor_pt;

// Structure holding the data to be transmitted
static data_packet_struct data_packet;
// data_packet->light_readings = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Current time stamp of the node
unsigned long curr_timestamp;

// Variables for light sensor readings
unsigned long light_readings[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int start_pos = 0;

// Starts the main contiki neighbour discovery process
PROCESS(light_sensor_process, "Light sensor reading process");
PROCESS(nbr_discovery_process, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&nbr_discovery_process, &light_sensor_process);

// Function called after reception of a packet
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) 
{
  // Check if the received packet size matches with what we expect it to be
  if(len == sizeof(data_packet)) {

    static data_packet_struct received_packet_data;
    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);
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

      // int curr_start_pos = start_pos - 1;
      for (int j = 0; j < 10; j++) {
        data_packet.light_readings[9 - j] = light_readings[(j + start_pos) % 10];
      }

      NETSTACK_NETWORK.output(&dest_addr); //Packet transmission
      

      // wait for WAKE_TIME before sending the next packet
      if(i != (NUM_SEND - 1)){

        rtimer_set(t, RTIMER_TIME(t) + WAKE_TIME, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
      
      }
    }

    // radio off
    NETSTACK_RADIO.off();

    // Sleep for 0.1s
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

  // last_received_timestamp = clock_time();
  // reboot_time = clock_time();

  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending packet of size %d Bytes\n", node_id, (int)sizeof(data_packet_struct));

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  

  PROCESS_END();
}

/*-------------------- LIGHT SENSOR FUNCTIONS --------------------*/

static void init_light_sensor()
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}

static void print_light_reading(int value)
{
  printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
}

void light_sensor_scan()
{
  static uint16_t i = 0;

  int value;

  value = opt_3001_sensor.value(0);
  if (value == INT_MIN || value == CC26XX_SENSOR_READING_ERROR) {
    printf("OPT: Light Sensor's Warming Up\n\n");
  } else {
    print_light_reading(value);
    light_readings[start_pos] = value;
    start_pos++;
    start_pos %= 10;
  }

  printf("Stored readings: ");
  for (i = 0; i < 10; i++) {
    printf("%ld, ", light_readings[i]);
  }
  printf("\n");

  printf("Sent readings: ");
  for (i = 0; i < 10; i++) {
    printf("%ld, ", data_packet.light_readings[i]);
  }
    printf("\n");
  
  init_light_sensor();
}

PROCESS_THREAD(light_sensor_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  init_light_sensor();

  while (1) {
    etimer_set(&et, CLOCK_SECOND * 5);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    light_sensor_scan();
  }

  PROCESS_END();
}
