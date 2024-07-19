#include "spi_flash.h"

#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#define EXT_FLASH_SPI SPI2
#define EXT_FLASH_PAGE_SIZE 256
#define unused(x) ((void)(x))
inline static void delay(uint32_t wait) {
  while (--wait > 0) __asm__("nop");
}
/*
 * Send a block of data via the SPI bus.
 */
static inline void spi_send_bytes(const uint8_t *data, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    spi_xfer(EXT_FLASH_SPI, data[i]);
  }
}
static inline void spi_recv_bytes(uint8_t *data, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    data[i] = spi_xfer(EXT_FLASH_SPI, 0x00);
  }
}

static inline void spi_flash_enable(void) {
  gpio_clear(GPIOB, GPIO12);
}

static inline void spi_flash_disable(void) {
  gpio_set(GPIOB, GPIO12);
}

static uint8_t spi_flash_read_status(void) {
    uint8_t status = 0;
    spi_flash_enable();
    spi_xfer(EXT_FLASH_SPI, 0x05);
    status = spi_xfer(EXT_FLASH_SPI, 0x00);
    spi_flash_disable();
    return status;
}
static void spi_flash_write_enable(void) {
  spi_flash_enable();
  spi_xfer(EXT_FLASH_SPI, 0x06);
  spi_flash_disable();
  while (!(spi_flash_read_status() & 0x02));
}
static void spi_flash_write_disable(void) {
  spi_flash_enable();
  spi_xfer(EXT_FLASH_SPI, 0x04);
  spi_flash_disable();
  while (spi_flash_read_status() & 0x02);
}
void spi_flash_chip_erase(void) {
  while (spi_flash_read_status() & 0x01);
  spi_flash_write_enable();
  // chip erase cmd
  spi_flash_enable();
  spi_xfer(EXT_FLASH_SPI, 0xc7);
  spi_flash_disable();
  while (spi_flash_read_status() & 0x01);
  spi_flash_write_disable();
}
void spi_flash_page_erase(uint32_t addr) {
  uint8_t cmd[4];
  spi_flash_enable();
  cmd[0] = 0x20;
  cmd[1] = (uint8_t)((addr >> 0x10) & 0xff);
  cmd[2] = (uint8_t)((addr >> 0x08) & 0xff);
  cmd[3] = (uint8_t)(addr & 0xff);
  spi_send_bytes(cmd, 4);
  spi_flash_disable();
  while (spi_flash_read_status() & 0x01);
}
void spi_flash_page_program(uint32_t addr, uint8_t *data, uint32_t len) {
  uint8_t cmd[4];
  spi_flash_write_enable();
  if (0 == (addr & (EXT_FLASH_PAGE_SIZE - 1))) {
    spi_flash_page_erase(addr);
    spi_flash_write_enable();
  }
  cmd[0] = 0x02;
  cmd[1] = (uint8_t)((addr >> 0x10) & 0xff);
  cmd[2] = (uint8_t)((addr >> 0x08) & 0xff);
  cmd[3] = (uint8_t)(addr & 0xff);
  spi_flash_enable();
  spi_send_bytes(cmd, 4);
  delay(500);
  spi_send_bytes(data, len);
  spi_flash_disable();
  while (spi_flash_read_status() & 0x01);
}
void spi_flash_page_read(uint32_t addr, uint8_t *data, uint32_t len) {
  uint8_t cmd[4];

  cmd[0] = 0x03;
  cmd[1] = (uint8_t)((addr >> 0x10) & 0xff);
  cmd[2] = (uint8_t)((addr >> 0x08) & 0xff);
  cmd[3] = (uint8_t)(addr & 0xff);
  spi_flash_enable();
  spi_send_bytes(cmd, 4);
  delay(500);
  spi_recv_bytes(data, len);
  spi_flash_disable();
}
void spi_flash_read_bytes(uint32_t addr, uint8_t *data, uint32_t len) {
  uint8_t cmd[4];

  while (spi_flash_read_status() & 0x01);
  cmd[0] = 0x03;
  cmd[1] = (uint8_t)((addr >> 0x10) & 0xff);
  cmd[2] = (uint8_t)((addr >> 0x08) & 0xff);
  cmd[3] = (uint8_t)(addr & 0xff);
  spi_flash_enable();
  spi_send_bytes(cmd, 4);
  delay(500);
  spi_recv_bytes(data, len);
  spi_flash_disable();
}
void spi_flash_deep_sleep(bool bentry) {
  uint8_t cmd;

  if (bentry) {
    cmd = 0xb9;
  } else {
    cmd = 0xad;
  }
  spi_flash_enable();
  spi_xfer(EXT_FLASH_SPI, cmd);
  spi_flash_disable();
}

bool spi_flash_read(uint32_t offset, uint8_t *data, uint32_t len) {
  spi_flash_read_bytes(offset, data, len);
  return true;
}

bool spi_flash_write(const uint32_t offset, uint8_t *data, uint32_t len) {
  uint32_t pages, remander;
  pages = len / EXT_FLASH_PAGE_SIZE;
  remander = len % EXT_FLASH_PAGE_SIZE;
  for (uint32_t i = 0; i < pages; ++i) {
    spi_flash_page_program(offset + i * EXT_FLASH_PAGE_SIZE, data + i * EXT_FLASH_PAGE_SIZE, EXT_FLASH_PAGE_SIZE);
  }
  if (remander) {
    uint8_t tmp[EXT_FLASH_PAGE_SIZE];
    memset(tmp, 0xff, EXT_FLASH_PAGE_SIZE);
    memcpy(tmp, data + pages * EXT_FLASH_PAGE_SIZE, remander);
    spi_flash_page_program(offset + pages * EXT_FLASH_PAGE_SIZE, tmp, EXT_FLASH_PAGE_SIZE);
  }
  return true;
}


void spi_flash_test(void) {
  // uint8_t data[11] = {0x9F, 0x55, 0x67, 0x77, 0x88, 0x99,
  //                     0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  // uint8_t recv[3] = {0};
  uint8_t write_buffer[EXT_FLASH_PAGE_SIZE] = {0};
  uint8_t read_buffer[EXT_FLASH_PAGE_SIZE] = {0};
  // spi_flash_enable();
  // spi_send_bytes(data, 11);
  // // spi_recv_bytes(EXT_FLASH_SPI, recv, 3);
  // spi_flash_disable();

  // spi_flash_chip_erase(); // 2M Bytes
  memset(write_buffer, 0x5a, EXT_FLASH_PAGE_SIZE);
  spi_flash_write(0, write_buffer, EXT_FLASH_PAGE_SIZE);
  spi_flash_read(0, read_buffer, EXT_FLASH_PAGE_SIZE);
  if (memcmp(write_buffer, read_buffer, EXT_FLASH_PAGE_SIZE) != 0) {
    printf("spi_flash_test failed\n");
  }
  memset(write_buffer, 0xa5, EXT_FLASH_PAGE_SIZE);
  spi_flash_write(0x100, write_buffer, EXT_FLASH_PAGE_SIZE);
  spi_flash_read(0x100, read_buffer, EXT_FLASH_PAGE_SIZE);
  if (memcmp(write_buffer, read_buffer, EXT_FLASH_PAGE_SIZE) != 0) {
    printf("spi_flash_test failed\n");
  }
  memset(write_buffer, 0xa6, EXT_FLASH_PAGE_SIZE);
}
