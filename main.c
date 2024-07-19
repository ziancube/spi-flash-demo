#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

#include "spi_flash.h"

void svc_handler_main(uint32_t *stack) {
  (void)stack;
  // do nothing
}

static void setup(void) {
  SCB_CCR |= SCB_CCR_STKALIGN;

  // setup clock
  struct rcc_clock_scale clock = rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_120MHZ];
  rcc_clock_setup_hse_3v3(&clock);

  // enable GPIO clock - A (oled), B(oled), C (buttons)
  rcc_periph_clock_enable(RCC_GPIOB);

  // enable SPI clock
  rcc_periph_clock_enable(RCC_SPI2);

  // GPIO for SPI2
  //    NSS: PB12
  //    SCK: PB13
  //    MISO: PB14
  //    MOSI: PB15
  
  // spi2
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
  gpio_set(GPIOB, GPIO12);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14| GPIO15);
  gpio_set_af(GPIOB, GPIO_AF5, GPIO13 | GPIO14 | GPIO15);
  spi_init_master(
      SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
      SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
  spi_enable_software_slave_management(SPI2);
  spi_set_nss_high(SPI2);
  spi_enable(SPI2);
  // spi2 end
}

int main(void) {
    setup();
    spi_flash_test();
    while(1);
}