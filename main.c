#include "stm8s.h"
#include <intrinsics.h>
// Определения для UART
#define SOFT_UART_BAUD_RATE 9600
#define SOFT_UART_TX_PIN GPIO_PIN_6
#define SOFT_UART_TX_PORT GPIOB
#define SOFT_UART_RX_PIN GPIO_PIN_5
#define SOFT_UART_RX_PORT GPIOB
#define SOFT_UART_RTS_PIN GPIO_PIN_4   // Добавляем определение для RTS
#define SOFT_UART_RTS_PORT GPIOB       // Добавляем определение для RTS
#define SOFT_UART_DELAY 56522

void SoftUART_Init(void);
void SoftUART_SendByte(uint8_t data);
uint8_t SoftUART_ReceiveByte(void);
void SoftUART_SendString(const char *str);
void delay_us(uint32_t us);
volatile uint8_t SoftUART_TxBuffer;
volatile uint8_t SoftUART_TxBufferEmpty;
volatile uint8_t SoftUART_RxBuffer;
volatile uint8_t SoftUART_RxBufferFull;

volatile uint32_t ticks;



void delay_us(uint32_t us) {
    volatile uint32_t i;
    for (; us > 0; us--) {
     void __no_operation(void); // Используем стандартную функцию NOP
    }
}

void SoftUART_Init() {
    // Инициализация Tx, Rx и RTS выводов
    GPIO_Init(SOFT_UART_TX_PORT, SOFT_UART_TX_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(SOFT_UART_RX_PORT, SOFT_UART_RX_PIN, GPIO_MODE_IN_PU_NO_IT);
    GPIO_Init(SOFT_UART_RTS_PORT, SOFT_UART_RTS_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);  // Инициализируем RTS как выход с высоким уровнем по умолчанию

    SoftUART_TxBufferEmpty = 1;
    SoftUART_RxBufferFull = 0;

    TIM4_DeInit();
    TIM4_TimeBaseInit(TIM4_PRESCALER_128, 1);
    TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
    TIM4_Cmd(ENABLE);
}

void SoftUART_SendByte(uint8_t data) {
    while (!SoftUART_TxBufferEmpty);
    SoftUART_TxBuffer = data;
    SoftUART_TxBufferEmpty = 0;

    // Устанавливаем низкий уровень на RTS перед началом передачи
    GPIO_WriteLow(SOFT_UART_RTS_PORT, SOFT_UART_RTS_PIN);

    GPIO_WriteLow(SOFT_UART_TX_PORT, SOFT_UART_TX_PIN);
    delay_us(SOFT_UART_DELAY / SOFT_UART_BAUD_RATE);
    for (uint8_t i = 0; i < 8; ++i) {
        if (data & 0x01) GPIO_WriteHigh(SOFT_UART_TX_PORT, SOFT_UART_TX_PIN);
        else GPIO_WriteLow(SOFT_UART_TX_PORT, SOFT_UART_TX_PIN);
        delay_us(SOFT_UART_DELAY / SOFT_UART_BAUD_RATE);
        data >>= 1;
    }
    GPIO_WriteHigh(SOFT_UART_TX_PORT, SOFT_UART_TX_PIN);
    delay_us(SOFT_UART_DELAY / SOFT_UART_BAUD_RATE);

    // Возвращаем высокий уровень на RTS после завершения передачи
    GPIO_WriteHigh(SOFT_UART_RTS_PORT, SOFT_UART_RTS_PIN);

    SoftUART_TxBufferEmpty = 1;
}

void SoftUART_SendString(const char *str) {
    uint8_t i = 0;
    while (str[i] != '\0') {
        SoftUART_SendByte(str[i]);
        i++;
    }
}

uint8_t SoftUART_ReceiveByte() {
    while (!SoftUART_RxBufferFull);
    uint8_t data = SoftUART_RxBuffer;
    SoftUART_RxBufferFull = 0;
    return data;
}

#pragma vector=0x16
__interrupt void TIM4_UPD_SR_Handler() {
    TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
    ticks++;
    if (ticks == 10) {
        ticks = 0;
        uint8_t data = 0;
        if (GPIO_ReadInputPin(SOFT_UART_RX_PORT, SOFT_UART_RX_PIN)) return;
        delay_us(SOFT_UART_DELAY / SOFT_UART_BAUD_RATE);
        for (uint8_t i = 0; i < 8; ++i) {
            data >>= 1;
            if (GPIO_ReadInputPin(SOFT_UART_RX_PORT, SOFT_UART_RX_PIN)) data |= 0x80;
            delay_us(SOFT_UART_DELAY / SOFT_UART_BAUD_RATE);
        }
        if (!GPIO_ReadInputPin(SOFT_UART_RX_PORT, SOFT_UART_RX_PIN)) return;
        SoftUART_RxBuffer = data;
        SoftUART_RxBufferFull = 1;
    }
}

int main() {
    SoftUART_Init();
    while (1) {
        //SoftUART_SendString("hello world\n");
        if (SoftUART_RxBufferFull) {
            uint8_t data = SoftUART_ReceiveByte();
            SoftUART_SendByte('"');
            SoftUART_SendByte(data);
            SoftUART_SendByte('"');
        }
    }
}
