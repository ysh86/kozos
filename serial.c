#include "defines.h"
#include "serial.h"

#define STM32F4_RCC_BASE      0x40023800
//#define STM32F4_RCC_CR        ((volatile uint32_t *)(STM32F4_RCC_BASE + 0x00))
//#define STM32F4_RCC_CR_HSIRDY (1<<1)
//#define STM32F4_RCC_CFGR      ((volatile uint32_t *)(STM32F4_RCC_BASE + 0x08))
//#define STM32F4_RCC_APB2RSTR  ((volatile uint32_t *)(STM32F4_RCC_BASE + 0x24))
//#define STM32F4_RCC_APB2RSTR_USART6RST (1<<5)
#define STM32F4_RCC_AHB1ENR  ((volatile uint32_t *)(STM32F4_RCC_BASE + 0x30))
#define STM32F4_RCC_AHB1ENR_GPIOCEN (1<<2)
#define STM32F4_RCC_APB2ENR  ((volatile uint32_t *)(STM32F4_RCC_BASE + 0x44))
#define STM32F4_RCC_APB2ENR_USART6EN (1<<5)

#define STM32F4_GPIOC_BASE  0x40020800
#define STM32F4_GPIOC_MODER ((volatile uint32_t *)(STM32F4_GPIOC_BASE + 0x00))
#define STM32F4_GPIOC_AFRL  ((volatile uint32_t *)(STM32F4_GPIOC_BASE + 0x20))
#define STM32F4_GPIOC_MODER_6_MASK ((1<<13)|(1<<12))
#define STM32F4_GPIOC_MODER_7_MASK ((1<<15)|(1<<14))
#define STM32F4_GPIOC_MODER_6_AF   ((1<<13)|(0<<12))
#define STM32F4_GPIOC_MODER_7_AF   ((1<<15)|(0<<14))
#define STM32F4_GPIOC_AFRL_6_MASK  ((1<<27)|(1<<26)|(1<<25)|(1<<24))
#define STM32F4_GPIOC_AFRL_7_MASK  ((1<<31)|(1<<30)|(1<<29)|(1<<28))
#define STM32F4_GPIOC_AFRL_6_AF8   ((1<<27)|(0<<26)|(0<<25)|(0<<24))
#define STM32F4_GPIOC_AFRL_7_AF8   ((1<<31)|(0<<30)|(0<<29)|(0<<28))

#define STM32F4_USART_NUM 1
#define STM32F4_USART6 ((volatile struct stm32f4_usart *)0x40011400)
#define STM32F4_USART6_SR_TXE (1<<7)
#define STM32F4_USART6_SR_TC  (1<<6)
#define STM32F4_USART6_CR1_UE (1<<13)
#define STM32F4_USART6_CR1_TE (1<<3)
#define STM32F4_USART6_CR1_RE (1<<2)

struct stm32f4_usart {
    volatile uint32_t sr;
    volatile uint32_t dr;
    volatile uint32_t brr;
    volatile uint32_t cr1;
    volatile uint32_t cr2;
    volatile uint32_t cr3;
    volatile uint32_t gtpr;
};

static struct {
    volatile struct stm32f4_usart *usart;
} regs[STM32F4_USART_NUM] = {
    { STM32F4_USART6 },
};

int serial_init(int index)
{
    volatile struct stm32f4_usart *usart = regs[index].usart;

    usart->cr1  = 0; // disabled

    // HSI clock ready
    //while (!(*STM32F4_RCC_CR & STM32F4_RCC_CR_HSIRDY))
    //    ;

    // clock enabled
    *STM32F4_RCC_AHB1ENR |= STM32F4_RCC_AHB1ENR_GPIOCEN;
    *STM32F4_RCC_APB2ENR |= STM32F4_RCC_APB2ENR_USART6EN;

    *STM32F4_GPIOC_MODER &= ~(STM32F4_GPIOC_MODER_7_MASK | STM32F4_GPIOC_MODER_6_MASK);
    *STM32F4_GPIOC_MODER |=  (STM32F4_GPIOC_MODER_7_AF   | STM32F4_GPIOC_MODER_6_AF);
    *STM32F4_GPIOC_AFRL  &= ~(STM32F4_GPIOC_AFRL_7_MASK  | STM32F4_GPIOC_AFRL_6_MASK);
    *STM32F4_GPIOC_AFRL  |=  (STM32F4_GPIOC_AFRL_7_AF8   | STM32F4_GPIOC_AFRL_6_AF8);

    // reset
    //*STM32F4_RCC_APB2RSTR |= STM32F4_RCC_APB2RSTR_USART6RST;
    //*STM32F4_RCC_APB2RSTR &= ~STM32F4_RCC_APB2RSTR_USART6RST;

    //usart->brr  = 0x222e; // mbed default: 0x222e 84MHz / 16 / 546.875 = 9600
    usart->brr = 0x0683; // 16MHz / 16 / 104.1875 = 9598
    usart->cr2  = 0;
    usart->cr3  = 0;
    usart->gtpr = 0;

    usart->cr1  = (STM32F4_USART6_CR1_UE | STM32F4_USART6_CR1_TE | STM32F4_USART6_CR1_RE);
    /*
    serial_send_byte(index, ' ');
    serial_send_byte(index, ' ');
    serial_send_byte(index, ' ');
    serial_send_byte(index, ' ');
    serial_send_byte(index, '\r');
    serial_send_byte(index, '\n');
    */

    return 0;
}

int serial_is_send_enable(int index)
{
    volatile struct stm32f4_usart *usart = regs[index].usart;
    return (usart->sr & STM32F4_USART6_SR_TXE);
}

int serial_send_byte(int index, unsigned char c)
{
    volatile struct stm32f4_usart *usart = regs[index].usart;

    while (!serial_is_send_enable(index))
        ;
    usart->dr = c;
    //usart->sr &= ~STM32F4_USART6_SR_TC; // clear TC

    return 0;
}
