/* This sketch allows a local bus to be segmented. It can be used for:

1. Extending its medium as a repeater, for example if SoftwareBitBang bus
wires are getting too long, they can be splitted in two sections connected by
a repeater, extending range. This scheme can be repeated to enable really
long distances.

2. Let devices on different media communicate with each other transparently, for
example to let devices on a SoftwareBitBang bus wire communicate with devices
using the LocalUDP strategy on a LAN.

3. Tunnel packets through another medium using two extenders mirroring each
other. For example, two SoftwareBitBang buses that are far from each other
can be joined transparently through a LAN using the LocalUDP strategy, or
through a WAN or Internet using the EthernetTCP strategy.

4. Extend (3) to let packets flow freely between multiple SoftwareBitBang
buses on different media with multiple extenders exchanging packets on a common
medium like a LAN using LocalUDP strategy.

A non-local version of this sketch, connecting two buses with different bus
ids, is available as the "Router" sketch among the Network examples.

Extender examples contributed by Fred Larsen. */

#include <PJON.h>

const uint8_t DEVICE_ID = 50;

// <Strategy name> bus(selected device id)
PJON<SoftwareBitBang> busA(DEVICE_ID);
PJON<SoftwareBitBang> busB(DEVICE_ID);

// All packets to devices listed here will be forwarded to bus B
const uint8_t device_id_ranges_on_B_side[] = {10, 19,
                                              80, 89};
const uint8_t single_device_ids_on_B_side[] = {30, 33, 37, 44, 46};

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW); // Initialize LED 13 to be off

  busA.strategy.set_pin(7);
  busA.set_receiver(receiver_functionA);
  busA.set_router(true);
  busA.begin();

  busB.strategy.set_pin(8);
  busB.set_receiver(receiver_functionB);
  busB.set_router(true);
  busB.begin();
}

void receiver_functionA(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  // Forward packet to B segment of local bus
  if (is_device_on_B_side(packet_info.receiver_id)) {
    busA.strategy.send_response(PJON_ACK);
    toggle_led();
    busB.send_from_id(packet_info.sender_id, packet_info.sender_bus_id,
      packet_info.receiver_id, packet_info.receiver_bus_id, payload, length, packet_info.header);
  }
}

void receiver_functionB(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  // Forward packet to A segment of local bus
  if (!is_device_on_B_side(packet_info.receiver_id)) {
    busB.strategy.send_response(PJON_ACK);
    toggle_led();
    busA.send_from_id(packet_info.sender_id, packet_info.sender_bus_id,
      packet_info.receiver_id, packet_info.receiver_bus_id, payload, length, packet_info.header);
  }
}

void toggle_led() {
  static bool led_is_on = false;
  led_is_on = !led_is_on;
  digitalWrite(13, led_is_on ? HIGH : LOW);
}

bool is_device_on_B_side(uint8_t device_id) {
  // Check if in one of the B ranges
  for (uint8_t i = 0; i < sizeof device_id_ranges_on_B_side - 1; i += 2)
    if (device_id_ranges_on_B_side[i] <= device_id && device_id <= device_id_ranges_on_B_side[i+1])
      return true;

  // Check if one of the individually registered B side devices
  for (uint8_t i = 0; i < sizeof single_device_ids_on_B_side; i++)
    if (single_device_ids_on_B_side[i] == device_id) return true;

  return false;
}

void loop() {
  busA.receive(1000);
  busB.update();
  busB.receive(1000);
  busA.update();
};
