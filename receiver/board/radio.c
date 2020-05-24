#include <string.h>

#include "nordic_common.h"
#include "nrf.h"

#define PACKET_BASE_ADDRESS_LENGTH  (4UL)    //!< Packet base address length field size in bytes
#define PACKET_STATIC_LENGTH        (0UL)    //!< Packet static length in bytes
#define PACKET_PAYLOAD_MAXSIZE      (250UL)  //!< Packet payload maximum size in bytes

#define PACKET_S1_FIELD_SIZE      (0UL)  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE      (0UL)  /**< Packet S0 field size in bits. */
#define PACKET_LENGTH_FIELD_SIZE  (8UL)  /**< Packet length field size in bits. */

#define ADDR_PREFIX0  0x7ab70116
#define ADDR_PREFIX1  0xbe99fb34
#define ADDR_BASE0    0x7f400324
#define ADDR_BASE1    0x3257b521

uint8_t packet[250];

volatile uint8_t buffer[256];
volatile uint8_t bufflen = 0;

uint8_t outptr = 0;

void radio_rx_enable(void)
{
  NRF_RADIO->INTENSET = RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos;
  NVIC_EnableIRQ(RADIO_IRQn);

  NRF_RADIO->EVENTS_READY = 0U;
  NRF_RADIO->TASKS_RXEN = 1U;
  while (NRF_RADIO->EVENTS_READY == 0U) {}

  NRF_RADIO->EVENTS_END = 0U;
  NRF_RADIO->TASKS_START = 1U;
}

void radio_rx_disable(void)
{
  NRF_RADIO->INTENSET = RADIO_INTENSET_END_Disabled << RADIO_INTENSET_END_Pos;
  NVIC_DisableIRQ(RADIO_IRQn);

  NRF_RADIO->EVENTS_DISABLED = 0U;
  NRF_RADIO->TASKS_DISABLE = 1U;

  while (NRF_RADIO->EVENTS_DISABLED == 0U) {}
}

void radio_send_buffer(uint8_t* buff, uint8_t size)
{
    memcpy(packet+1, buff, size);
    packet[0] = size;

    // Send the packet
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;
    while (NRF_RADIO->EVENTS_READY == 0U) {}

    NRF_RADIO->EVENTS_END  = 0U;
    NRF_RADIO->TASKS_START = 1U;
    while (NRF_RADIO->EVENTS_END == 0U) {}

    NRF_RADIO->EVENTS_DISABLED = 0U;
    NRF_RADIO->TASKS_DISABLE = 1U;
    while (NRF_RADIO->EVENTS_DISABLED == 0U) {}
}

uint8_t rgetc(void)
{
  return buffer[outptr++];
}

uint8_t rbufflen(void)
{
  return (bufflen < outptr) ? (256-outptr) + bufflen : bufflen - outptr;
}

uint8_t ravail()
{
  return (bufflen!=outptr);
}

void RADIO_IRQHandler(void)
{
  if (NRF_RADIO->EVENTS_END) {
    for (uint8_t i=1; i<=packet[0]; i++) {
      buffer[bufflen++] = packet[i];
    }
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_START = 1;
  }
}

void radio_configure(void)
{
  // Radio config
  NRF_RADIO->TXPOWER   = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
  NRF_RADIO->FREQUENCY = 7UL;  // Frequency bin 7, 2407MHz
  NRF_RADIO->MODE      = (RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos);

  // Radio address config
  NRF_RADIO->PREFIX0 = ADDR_PREFIX0;
  NRF_RADIO->PREFIX1 = ADDR_PREFIX1;

  NRF_RADIO->BASE0 = ADDR_BASE0;
  NRF_RADIO->BASE1 = ADDR_BASE1;

  NRF_RADIO->TXADDRESS   = 0x00UL;  // Set device address 0 to use when transmitting
  NRF_RADIO->RXADDRESSES = 0x01UL;  // Enable device address 0 to use to select which addresses to receive

  // Packet configuration
  NRF_RADIO->PCNF0 = (PACKET_S1_FIELD_SIZE     << RADIO_PCNF0_S1LEN_Pos) |
                     (PACKET_S0_FIELD_SIZE     << RADIO_PCNF0_S0LEN_Pos) |
                     (PACKET_LENGTH_FIELD_SIZE << RADIO_PCNF0_LFLEN_Pos);

  // Packet configuration
  NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
                     (RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |
                     (PACKET_BASE_ADDRESS_LENGTH   << RADIO_PCNF1_BALEN_Pos)   |
                     (PACKET_STATIC_LENGTH         << RADIO_PCNF1_STATLEN_Pos) |
                     (PACKET_PAYLOAD_MAXSIZE       << RADIO_PCNF1_MAXLEN_Pos);

  // CRC Config
  NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits
  if ((NRF_RADIO->CRCCNF & RADIO_CRCCNF_LEN_Msk) == (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos))
  {
      NRF_RADIO->CRCINIT = 0xFFFFUL;   // Initial value
      NRF_RADIO->CRCPOLY = 0x11021UL;  // CRC poly: x^16 + x^12^x^5 + 1
  }
  else if ((NRF_RADIO->CRCCNF & RADIO_CRCCNF_LEN_Msk) == (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos))
  {
      NRF_RADIO->CRCINIT = 0xFFUL;   // Initial value
      NRF_RADIO->CRCPOLY = 0x107UL;  // CRC poly: x^8 + x^2^x^1 + 1
  }

  NRF_RADIO->PACKETPTR = (uint32_t)&packet;
}
