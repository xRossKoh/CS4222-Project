#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdio.h> 

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (0.25 * CLOCK_SECOND)
#define LOG_CONSTANT (10 * CLOCK_SECOND)
// static linkaddr_t dest_addr =         {{ 0x00, 0x12, 0x4b, 0x00, 0x13, 0x25, 0x56, 0x80 }}; //replace this with your receiver's link address
static linkaddr_t dest_addr =  {{ 0x00, 0x12, 0x4b, 0x00, 0x12, 0x05, 0x29, 0xb5}};

static unsigned int packet_count = 0;
static signed short rssi_sum = 0;


/*---------------------------------------------------------------------------*/
PROCESS(unicast_process, "One to One Communication");
PROCESS(calc_process, "Counter");
AUTOSTART_PROCESSES(&unicast_process, &calc_process);

/*---------Callback executed immediately after reception---------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) 
{
  if(len == sizeof(unsigned)) {
    packet_count++;
    signed short curr_rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);
    rssi_sum += curr_rssi;
    signed count;
    memcpy(&count, data, sizeof(count));
    printf("Received %u at RSSI %d \n", count, curr_rssi);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count = 0;

  PROCESS_BEGIN();



  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&count; //data transmitted
  nullnet_len = sizeof(count); //length of data transmitted
  nullnet_set_input_callback(input_callback); //initialize receiver callback

  if(!linkaddr_cmp(&dest_addr, &linkaddr_node_addr)) { //ensures destination is not same as sender
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      // LOG_INFO("Sending...\n");
      // LOG_INFO_LLADDR(&dest_addr);
      // LOG_INFO_("\n");

      // LOG_INFO("Sending %u to ", count);
      // LOG_INFO_LLADDR(&dest_addr);
      // LOG_INFO_("\n");

      NETSTACK_NETWORK.output(&dest_addr); //Packet transmission
      count++;
      // packet_count++;
      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}

PROCESS_THREAD(calc_process, ev, data){
  static struct etimer timer;
  PROCESS_BEGIN();
  etimer_set(&timer, LOG_CONSTANT);
  while (1) {
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    printf("Packets: %u, RSSI: %d \n", packet_count, rssi_sum);
    rssi_sum = 0;
    packet_count = 0;
    etimer_reset(&timer);
  }
  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
