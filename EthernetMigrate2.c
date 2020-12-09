/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer *
* Copyright (C) 2014 Renesas Electronics Corporation
* and Renesas Solutions Corp. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* File Name    : main.c
* Version      : 1.00
* Device(s)    : Renesas R5F564MxxDxx
* Tool-Chain   : Renesas e2 studio v3.0.1.09 or later
*              : C/C++ Compiler Package for RX Family V2.02.00 or later
* OS           : None
* H/W Platform : Renesas Starter Kit+ RX64M
* Description  : This is the main tutorial code.
******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
*         : 04.08.2014 1.00    First Release
*         : 31.10.2014 1.01    All network modules supported
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <string.h>
#include "platform.h"
#include "r_t4_itcpip.h"
#include "r_ether_rx_if.h"
#include "r_t4_dhcp_client_rx_if.h"
#include "r_t4_dns_client_rx_if.h"

#include "timer.h"

/******************************************************************************
Section    <Section Definition> , "Project Sections"
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

extern uint8_t _myethaddr[][6];
extern TCPUDP_ENV tcpudp_env[];
extern uint8_t dnsaddr1[];
extern uint8_t dnsaddr2[];

/******************************************************************************
Private global variables and functions
******************************************************************************/
static UW tcpudp_work[ 21504 / sizeof(UW)]; // calculated by W tcpudp_get_ramsize( void )

void   set_tcpudp_env(DHCP *dhcp);

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/
extern volatile uint8_t  link_detect[ETHER_CHANNEL_MAX];

/******************************************************************************
Function Name   : main
Description     : sample main
Arguments       : none
Return value    : none
******************************************************************************/

void main(void)
{
	printf("Hi--\n");
    ER          ercd;
    W           ramsize;
    volatile DHCP       dhcp;

    /* Initialize the Ethernet driver */
    R_ETHER_Initial();

    /* start LAN controller */
    ercd = lan_open();
    if (ercd != E_OK)
    {
        for( ;; );
    }

    /* DHCP client */
    OpenTimer();
    while (ETHER_FLAG_ON_LINK_ON != link_detect[0] && ETHER_FLAG_ON_LINK_ON != link_detect[1])
    {
        R_ETHER_LinkProcess(0);
        R_ETHER_LinkProcess(1);
    }

    if (!r_dhcp_open(&dhcp, (unsigned char*)tcpudp_work, &_myethaddr[0][0]))
    {
        set_tcpudp_env(&dhcp);
    }
    CloseTimer();

    if (NULL != &dhcp)
    {
        nop();
    }

    /* Get the size of the work area used by the T4 (RAM size). */
    ramsize = tcpudp_get_ramsize();
    if (ramsize > (sizeof(tcpudp_work)))
    {
        /* Then reserve as much memory array for the work area as the size 
           indicated by the returned value. */
        for( ;; );
    }

    /* Initialize the TCP/IP */
    ercd = tcpudp_open(tcpudp_work);
    if (ercd != E_OK)
    {
        for( ;; );
    }

    /* Init DNS clienr */
    R_dns_init();

    /* Sample code main loop */
    while( 1 )
    {
        /* Ethernet driver link up processing */
        R_ETHER_LinkProcess(0);
        R_ETHER_LinkProcess(1);
    }
}
/******************************************************************************
End of function main
******************************************************************************/

/******************************************************************************
Function Name   : set_tcpudp_env
Description     : Set DHCP's error logs to TCP event buffer.
Arguments       : none
Return value    : none
******************************************************************************/
void    set_tcpudp_env(DHCP *dhcp)
{
    if (NULL != dhcp)
    {
        memcpy(tcpudp_env[0].ipaddr, dhcp->ipaddr, 4);
        memcpy(tcpudp_env[0].maskaddr, dhcp->maskaddr, 4);
        memcpy(tcpudp_env[0].gwaddr, dhcp->gwaddr, 4);

        memcpy((char *)dnsaddr1, (char *)dhcp->dnsaddr, 4);
        memcpy((char *)dnsaddr2, (char *)dhcp->dnsaddr2, 4);
    }
}

/******************************************************************************
End of file
******************************************************************************/
