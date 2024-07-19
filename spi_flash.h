#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_
#include <stdint.h>
#include <stdbool.h>

bool spi_flash_read(uint32_t offset, uint8_t *data, uint32_t len);
bool spi_flash_write(const uint32_t offset, uint8_t *data, uint32_t len);
void spi_flash_test(void);
#endif



