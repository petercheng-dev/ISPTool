/**************************************************************************//**
 * @file     uart_transfer.c
 * @version  V1.00
 * $Date: 14/11/17 5:36p $
 * @brief    General UART ISP slave Sample file
 *
 * @note
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <string.h>
#include "targetdev.h"
#include "uart_transfer.h"

__align(4) uint8_t  uart_rcvbuf[MAX_PKT_SIZE] = {0};

uint8_t volatile bUartDataReady = 0;
uint8_t volatile bufhead = 0;


/* please check "targetdev.h" for chip specifc define option */

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel 0 interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void UART_T_IRQHandler(void)
{
    /*----- Determine interrupt source -----*/
    uint32_t u32IntSrc = UART_T->ISR;

    if(u32IntSrc & 0x11) { //RDA FIFO interrupt & RDA timeout interrupt
        while(((UART_T->FSR & UART_FSR_RX_EMPTY_Msk) == 0) && (bufhead < MAX_PKT_SIZE))	//RX fifo not empty
            uart_rcvbuf[bufhead++] = UART_T->RBR;
    }

    if(bufhead == MAX_PKT_SIZE) {
        bUartDataReady = TRUE;
        bufhead = 0;
    } else if(u32IntSrc & 0x10) {
        bufhead = 0;
    }
}

extern __align(4) uint8_t response_buff[64];
void PutString(void)
{
    uint32_t i;

    for(i = 0; i < MAX_PKT_SIZE; i++) {
        while ((UART_T->FSR & UART_FSR_TX_FULL_Msk));
        UART_T->THR = response_buff[i];
    }
}

void UART_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Select UART function */
#if defined(UART_FUNC_SEL_UART)
    UART_T->FUN_SEL = UART_FUNC_SEL_UART;
#endif

// #if defined(TARGET_NANO1X2) || defined(TARGET_NANO103)
#if defined(UART_TLCTL_RFITL_14BYTES)
    UART_T->TLCTL = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1 |
                  UART_TLCTL_RFITL_14BYTES | UART_TLCTL_RTS_TRI_LEV_14BYTES;
#else
    /* Set UART line configuration */
    UART_T->LCR = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;

    /* Set UART Rx and RTS trigger level */
    UART_T->FCR = UART_FCR_RFITL_14BYTES | UART_FCR_RTS_TRI_LEV_14BYTES;
#endif

    /* Set UART baud rate */
#if defined(TARGET_NANO103)
    UART_T->BAUD = (UART_BAUD_MODE0 | UART_BAUD_MODE0_DIVIDER(__MIRC, 115200));
#elif defined(TARGET_M480)
    UART_T->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, 115200));
#else
    UART_T->BAUD = (UART_BAUD_MODE0 | UART_BAUD_MODE0_DIVIDER(__HIRC, 115200));
#endif

#ifdef UART_TOR_TOIC_Msk
    /* Set time-out interrupt comparator */
    UART_T->TOR = (UART_T->TOR & ~UART_TOR_TOIC_Msk)| (0x40);
#else
    UART_T->TOR = 0x40;
#endif

    NVIC_SetPriority (UART_T_IRQn, 2);
    NVIC_EnableIRQ(UART_T_IRQn);

    /* 0x0811 */
    UART_T->IER = (UART_IER_TIME_OUT_EN_Msk | UART_IER_RTO_IEN_Msk | UART_IER_RDA_IEN_Msk);
}
