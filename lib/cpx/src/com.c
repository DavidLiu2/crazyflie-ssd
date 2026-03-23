/*
 * com.c
 * Responsibility: SPI transport between GAP8 and the ESP32/NINA CPX bridge.
 */

#include "com.h"

#include <string.h>

#include "printf.h"

#include "pmsis.h"

#define max(a, b)               \
  ({                            \
    __typeof__(a) _a = (a);     \
    __typeof__(b) _b = (b);     \
    _a > _b ? _a : _b;          \
  })

#if 0
#define DEBUG_PRINTF printf
#else
#define DEBUG_PRINTF(...) ((void)0)
#endif

#define CONFIG_NINA_GPIO_NINA_ACK 18
#define CONFIG_NINA_GPIO_NINA_ACK_PAD PI_PAD_32_A13_TIMER0_CH1
#define CONFIG_NINA_GPIO_NINA_ACK_PAD_FUNC PI_PAD_32_A13_GPIO_A18_FUNC1

#define CONFIG_NINA_GPIO_NINA_NOTIF 3
#define CONFIG_NINA_GPIO_NINA_NOTIF_PAD PI_PAD_15_B1_RF_PACTRL3
#define CONFIG_NINA_GPIO_NINA_NOTIF_PAD_FUNC PI_PAD_15_B1_GPIO_A3_FUNC1

#define GPIO_HIGH ((uint32_t)1)
#define GPIO_LOW ((uint32_t)0)

#ifndef TXQ_SIZE
#define TXQ_SIZE (80)
#endif

#ifndef RXQ_SIZE
#define RXQ_SIZE (5)
#endif

#define NINA_RTT_BIT (1 << 0)
#define TX_QUEUE_BIT (1 << 1)
#define INITIAL_TRANSFER_SIZE (4)

static struct pi_gpio_conf g_cts_gpio_conf;
static pi_gpio_callback_t g_cb_gpio;
static pi_device_t g_spi_dev;
static pi_device_t g_nina_rtt_dev;
static pi_device_t g_gap8_rtt_dev;
static QueueHandle_t g_txq = NULL;
static QueueHandle_t g_rxq = NULL;
static EventGroupHandle_t g_ev_group;
static packet_t g_rx_buff;
static packet_t g_tx_buff;

static void data_ready_isr(void *args)
{
  BaseType_t x_higher_priority_task_woken = pdFALSE;
  (void)args;
  xEventGroupSetBitsFromISR(g_ev_group, NINA_RTT_BIT, &x_higher_priority_task_woken);
  portYIELD_FROM_ISR(x_higher_priority_task_woken);
}

static void setup_nina_rtt_pin(pi_device_t *device)
{
  pi_gpio_conf_init(&g_cts_gpio_conf);
  pi_open_from_conf(device, &g_cts_gpio_conf);
  pi_gpio_open(device);
  pi_gpio_pin_configure(device, CONFIG_NINA_GPIO_NINA_ACK, PI_GPIO_INPUT);
  pi_gpio_pin_notif_configure(device, CONFIG_NINA_GPIO_NINA_ACK, PI_GPIO_NOTIF_RISE);
  pi_pad_set_function(CONFIG_NINA_GPIO_NINA_ACK_PAD, CONFIG_NINA_GPIO_NINA_ACK_PAD_FUNC);

  pi_gpio_callback_init(
      &g_cb_gpio,
      (1 << (CONFIG_NINA_GPIO_NINA_ACK & PI_GPIO_NUM_MASK)),
      data_ready_isr,
      (void *)CONFIG_NINA_GPIO_NINA_ACK);
  pi_gpio_callback_add(device, &g_cb_gpio);
}

static void setup_gap8_rtt_pin(pi_device_t *device)
{
  struct pi_gpio_conf gpio_conf;

  pi_gpio_conf_init(&gpio_conf);
  pi_open_from_conf(device, &gpio_conf);
  pi_gpio_open(device);
  pi_gpio_pin_configure(device, CONFIG_NINA_GPIO_NINA_NOTIF, PI_GPIO_OUTPUT);
  pi_pad_set_function(CONFIG_NINA_GPIO_NINA_NOTIF_PAD, CONFIG_NINA_GPIO_NINA_NOTIF_PAD_FUNC);
}

static void set_gap8_rtt_pin(pi_device_t *device, uint32_t val)
{
  if (pi_gpio_pin_write(device, CONFIG_NINA_GPIO_NINA_NOTIF, val)) {
    DEBUG_PRINTF("Could not set notification\n");
    pmsis_exit(-1);
  }
}

static void init_spi(pi_device_t *device)
{
  struct pi_spi_conf spi_conf = {0};

  pi_spi_conf_init(&spi_conf);
  spi_conf.wordsize = PI_SPI_WORDSIZE_8;
  spi_conf.big_endian = 1;
  spi_conf.max_baudrate = 10000000;
  spi_conf.polarity = 0;
  spi_conf.phase = 0;
  spi_conf.itf = 1;
  spi_conf.cs = 0;

  pi_open_from_conf(device, &spi_conf);

  if (pi_spi_open(device)) {
    DEBUG_PRINTF("SPI open failed\n");
    pmsis_exit(-1);
  }
}

static void com_task(void *parameters)
{
  EventBits_t ev_bits;
  uint32_t startup_esp_rtt_value;

  (void)parameters;

  DEBUG_PRINTF("Starting com task\n");

  pi_gpio_pin_read(&g_nina_rtt_dev, CONFIG_NINA_GPIO_NINA_ACK, &startup_esp_rtt_value);
  if (startup_esp_rtt_value > 0) {
    xEventGroupSetBits(g_ev_group, NINA_RTT_BIT);
  }

  while (1) {
    if (uxQueueMessagesWaiting(g_txq) == 0) {
      ev_bits = xEventGroupWaitBits(
          g_ev_group,
          NINA_RTT_BIT | TX_QUEUE_BIT,
          pdTRUE,
          pdFALSE,
          portMAX_DELAY);
    } else {
      ev_bits = 0;
    }

    if (uxQueueMessagesWaiting(g_txq) > 0) {
      xQueueReceive(g_txq, &g_tx_buff, 0);
    } else {
      memset(&g_tx_buff, 0, sizeof(packet_t));
    }

    if (g_tx_buff.len > 0) {
      set_gap8_rtt_pin(&g_gap8_rtt_dev, GPIO_HIGH);
      if ((ev_bits & NINA_RTT_BIT) == 0) {
        xEventGroupWaitBits(g_ev_group, NINA_RTT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
      }
    }

    if ((ev_bits & NINA_RTT_BIT) == NINA_RTT_BIT || g_tx_buff.len > 0) {
      int tx_len;
      int rx_len;
      int size_left;

      pi_spi_transfer(
          &g_spi_dev,
          &g_tx_buff,
          &g_rx_buff,
          INITIAL_TRANSFER_SIZE * 8,
          PI_SPI_LINES_SINGLE | PI_SPI_CS_KEEP);

      tx_len = g_tx_buff.len;
      rx_len = g_rx_buff.len;
      size_left = max(tx_len - INITIAL_TRANSFER_SIZE + 2, rx_len - INITIAL_TRANSFER_SIZE + 2);
      size_left = max(0, size_left);

      if ((size_left % 4) > 0) {
        size_left += (4 - size_left % 4);
      }

      if (size_left == 0) {
        size_left = 4;
      }

      set_gap8_rtt_pin(&g_gap8_rtt_dev, GPIO_LOW);

      pi_spi_transfer(
          &g_spi_dev,
          ((uint8_t *)&g_tx_buff) + INITIAL_TRANSFER_SIZE,
          ((uint8_t *)&g_rx_buff) + INITIAL_TRANSFER_SIZE,
          size_left * 8,
          PI_SPI_LINES_SINGLE | PI_SPI_CS_AUTO);

      if (g_rx_buff.len > 0) {
        if (xQueueSend(g_rxq, &g_rx_buff, portMAX_DELAY) != pdPASS) {
          DEBUG_PRINTF("RX Queue full!\n");
        }
      }
    }
  }
}

void com_init(void)
{
  BaseType_t x_task;

  setup_gap8_rtt_pin(&g_gap8_rtt_dev);
  init_spi(&g_spi_dev);

  g_txq = xQueueCreate(TXQ_SIZE, sizeof(packet_t));
  g_rxq = xQueueCreate(RXQ_SIZE, sizeof(packet_t));

  if (g_txq == NULL || g_rxq == NULL) {
    printf("Could not allocate txq and/or rxq in com\n");
    pmsis_exit(1);
  }

  g_ev_group = xEventGroupCreate();
  if (g_ev_group == NULL) {
    printf("Could not allocate event group in com\n");
    pmsis_exit(1);
  }

  x_task = xTaskCreate(
      com_task,
      "com_task",
      configMINIMAL_STACK_SIZE * 6,
      NULL,
      tskIDLE_PRIORITY + 1,
      NULL);
  if (x_task != pdPASS) {
    DEBUG_PRINTF("COM task did not start!\n");
    pmsis_exit(-1);
  }

  setup_nina_rtt_pin(&g_nina_rtt_dev);
}

void com_read(packet_t *packet)
{
  xQueueReceive(g_rxq, packet, portMAX_DELAY);
}

void com_write(packet_t *packet)
{
  xQueueSend(g_txq, packet, portMAX_DELAY);
  xEventGroupSetBits(g_ev_group, TX_QUEUE_BIT);
}
