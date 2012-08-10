/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2011, 2012 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file wirish/HardwareSerial.cpp
 * @brief Wirish serial port implementation.
 */

// #include <wirish/HardwareSerialFlowControl.h>
#include "HardwareSerialFlowControl.h"

#include <libmaple/libmaple.h>
#include <libmaple/gpio.h>
#include <libmaple/timer.h>
#include <libmaple/usart.h>


HardwareSerialFlowControl::HardwareSerialFlowControl(usart_dev *usart_device,
                          uint8 tx_pin,
                          uint8 rx_pin,
                          uint8 cts_pin,
                          uint8 rts_pin)
{
            this->usart_device = usart_device;
            this->tx_pin = tx_pin;
            this->rx_pin = rx_pin;
            this->cts_pin = cts_pin;
            this->rts_pin = rts_pin;
}



/*
 * Set up/tear down
 */

#if STM32_MCU_SERIES == STM32_SERIES_F1
/* F1 MCUs have no GPIO_AFR[HL], so turn off PWM if there's a conflict
 * on this GPIO bit. */
static void disable_timer_if_necessary(timer_dev *dev, uint8 ch) {
    if (dev != NULL) {
        timer_set_mode(dev, ch, TIMER_DISABLED);
    }
}
#elif (STM32_MCU_SERIES == STM32_SERIES_F2) ||    \
      (STM32_MCU_SERIES == STM32_SERIES_F4)
#define disable_timer_if_necessary(dev, ch) ((void)0)
#else
#warning "Unsupported STM32 series; timer conflicts are possible"
#endif

void HardwareSerialFlowControl::begin(uint32 baud) {
    ASSERT(baud <= this->usart_device->max_baud);

    if (baud > this->usart_device->max_baud) {
        return;
    }

    const stm32_pin_info *txi = &PIN_MAP[this->tx_pin];
    const stm32_pin_info *rxi = &PIN_MAP[this->rx_pin];

    //TODO figureout how to extract correct pin from &PIN_MAP[this->rts_pin]
    const stm32_pin_info *ctsi = &PIN_MAP[this->cts_pin];
    const stm32_pin_info *rtsi = &PIN_MAP[this->rts_pin];

    disable_timer_if_necessary(txi->timer_device, txi->timer_channel);
    /* Configure USART2 RTS and USART2 Tx as alternate function push-pull */
    /* Configure USART2 CTS and USART2 Rx as input floating */
    usart_config_gpios_async(this->usart_device,
                             rxi->gpio_device, rxi->gpio_bit,
                             txi->gpio_device, txi->gpio_bit,
                             0);
    // need to configure the rts and cts pins here as
    // gpio_set_mode(ctsi->gpio_device, ctsi->gpio_bit, GPIO_INPUT_FLOATING);
    // gpio_set_mode(rtsi->gpio_device, rtsi->gpio_bit, GPIO_AF_OUTPUT_PP);
    // note this would do the same thing but is less explicit
    usart_config_gpios_async(this->usart_device,
                             ctsi->gpio_device, ctsi->gpio_bit,
                             rtsi->gpio_device, rtsi->gpio_bit,
                             0);
    
    usart_init(this->usart_device);
    // enable rts/cts flow control in registers
    // should this be in usart.c for libmaple? does this apply to USART1,2..
    this->usart_device->regs->CR3 |= USART_CR3_RTSE_BIT |USART_CR3_CTSE_BIT;
    
    usart_set_baud_rate(this->usart_device, USART_USE_PCLK, baud);
    usart_enable(this->usart_device);
}

void HardwareSerialFlowControl::end(void) {
    usart_disable(this->usart_device);
}

/*
 * I/O
 */

uint8 HardwareSerialFlowControl::read(void) {
    // Block until a byte becomes available, to save user confusion.
    while (!this->available())
        ;
    return usart_getc(this->usart_device);
}

uint32 HardwareSerialFlowControl::available(void) {
    return usart_data_available(this->usart_device);
}

void HardwareSerialFlowControl::write(unsigned char ch) {
    usart_putc(this->usart_device, ch);
}

void HardwareSerialFlowControl::flush(void) {
    usart_reset_rx(this->usart_device);
}
