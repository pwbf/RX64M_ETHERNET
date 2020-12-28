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


uint8_t r_buff[272]={0};
// uint8_t t_buff[]={0x54,0x48,0x55,0x20,0x41,0x53,0x54,0x41,0x52,0x43};
uint8_t t_buff[]={0x47,0x45,0x54,0x20,0x2f,0x3f,0x6b,0x65,0x79,0x3d,0x46,0x36,0x35,0x45,0x45,0x35,0x31,0x38,0x43,0x46,0x32,0x38,0x41,0x42,0x33,0x45,0x34,0x45,0x45,0x33,0x39,0x35,0x31,0x30,0x33,0x37,0x39,0x37,0x41,0x41,0x42,0x46,0x38,0x33,0x41,0x39,0x46,0x30,0x32,0x39,0x43,0x30,0x36,0x43,0x34,0x42,0x33,0x41,0x33,0x36,0x33,0x43,0x32,0x42,0x30,0x46,0x36,0x42,0x36,0x32,0x38,0x39,0x32,0x39,0x26,0x66,0x69,0x65,0x6c,0x64,0x31,0x3d,0x39,0x35,0x32,0x37,0x26,0x66,0x69,0x65,0x6c,0x64,0x32,0x3d,0x34,0x33,0x32,0x31,0x26,0x66,0x69,0x65,0x6c,0x64,0x33,0x3d,0x31,0x31,0x2e,0x35,0x20,0x48,0x54,0x54,0x50,0x2f,0x31,0x2e,0x31,0x0d,0x0a,0x48,0x6f,0x73,0x74,0x3a,0x20,0x73,0x38,0x35,0x31,0x2e,0x70,0x6f,0x72,0x74,0x61,0x6c,0x2e,0x64,0x62,0x2e,0x6e,0x65,0x74,0x2e,0x70,0x77,0x62,0x66,0x2e,0x70,0x77,0x0d,0x0a,0x55,0x73,0x65,0x72,0x2d,0x41,0x67,0x65,0x6e,0x74,0x3a,0x20,0x50,0x46,0x43,0x20,0x36,0x34,0x6d,0x49,0x4f,0x54,0x20,0x41,0x67,0x65,0x6e,0x74,0x0d,0x0a,0x0d,0x0a};
//ascii "THU ASTARC"
uint8_t t_buff_len = sizeof(t_buff)/sizeof(t_buff[0]);
ER tcp_callback(ID cepid, FN fncd , VP p_parblk);

uint32_t encodeIPv4(uint8_t B1, uint8_t B2, uint8_t B3, uint8_t B4){
	return B1<<24 | B2<<16 | B3<<8 | B4<<0; 
}

void decodeIPv4(uint32_t encIP, uint8_t *decIP){
	*(decIP+0) = encIP >> 24;
	*(decIP+1) = encIP >> 16;
	*(decIP+2) = encIP >> 8;
	*(decIP+3) = encIP >> 0;
}

typedef struct _httppacket
{
	T_IPV4EP src;
	T_IPV4EP dst;
	uint8_t httpHeader[1024];
	
}HTTP_PACKET;

HTTP_PACKET *hp;
	
void main(void)
{
	printf("System Initial\n");
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

    // if (!r_dhcp_open(&dhcp, (unsigned char*)tcpudp_work, &_myethaddr[0][0]))
    // {
        // set_tcpudp_env(&dhcp);
    // }
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
    int32_t rtn = 0x00;
	
	hp->dst.ipaddr = encodeIPv4(45,77,22,98);
	hp->dst.portno = 80;
	// hp->dst.ipaddr = encodeIPv4(10,4,15,100);
	// hp->dst.portno = 9527;
	
	printf("pack TCP packet\n");
	
	printf("[CUS] packed ip=0x%X pt=%d\n",hp->dst.ipaddr, hp->dst.portno);
	printf("Request TCP connection\n");
	
	rtn = tcp_con_cep(1, (&hp->src), (&hp->dst), TMO_NBLK);
		
	switch(rtn){
		case E_OK:
			printf("connection established\n");
			break;
		case E_PAR:
			printf("invalid \"tmout\" value\n");
			break;
		case E_QOVR:
			printf("API does not end\n");
			break;
		case E_OBJ:
			printf("Object status error\n");
			break;
		case E_TMOUT:
			printf("Time out\n");
			break;
		case E_WBLK:
			printf("non-blocking call is accepted\n");
			break;
	}
	printf("Entering MAIN WHILE\n");
    while(1)
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
void set_tcpudp_env(DHCP *dhcp)
{
	printf("set_tcpudp_env\n");
    if (NULL != dhcp)
    {
		printf("DHCP replied!\n");
        memcpy(tcpudp_env[0].ipaddr, dhcp->ipaddr, 4);
        memcpy(tcpudp_env[0].maskaddr, dhcp->maskaddr, 4);
        memcpy(tcpudp_env[0].gwaddr, dhcp->gwaddr, 4);
		printf("\nDHCP->IP:");
		printf(dhcp->ipaddr);
		printf("\nDHCP->MASK:");
		printf(dhcp->maskaddr);
		printf("\nDHCP->Gateway:");
		printf(dhcp->gwaddr);

        memcpy((char *)dnsaddr1, (char *)dhcp->dnsaddr, 4);
        memcpy((char *)dnsaddr2, (char *)dhcp->dnsaddr2, 4);
    }
	printf("DHCP no reply\n");
}
/******************************************************************************
End of file
******************************************************************************/

ER tcp_callback(ID cepid, FN fncd , VP p_parblk){

    ER   parblk = *(ER *)p_parblk;
    ER   ercd;
    ID   cepid_oft;
	printf("\nfncd=0x%X\n", fncd);
	switch (fncd){
		case TFN_TCP_CRE_REP:
			printf("case TFN_TCP_CRE_REP\n");
			break;
		case TFN_TCP_DEL_REP:
			printf("case TFN_TCP_DEL_REP\n");
			break;
		case TFN_TCP_CRE_CEP:
			printf("case TFN_TCP_CRE_CEP\n");
			break;
		case TFN_TCP_DEL_CEP:
			printf("case TFN_TCP_DEL_CEP\n");
			break;
		case TFN_TCP_ACP_CEP:
			printf("case TFN_TCP_ACP_CEP\n");
			break;
		case TFN_TCP_CON_CEP:
			printf("case TFN_TCP_CON_CEP\n");
			printf("TCP [ACK] or [PSH, ACK]\n");
			tcp_snd_dat(cepid, &t_buff, t_buff_len, TMO_NBLK);
			break;
		case TFN_TCP_SHT_CEP:
			printf("case TFN_TCP_SHT_CEP\n");
			break;
		case TFN_TCP_CLS_CEP:
			printf("case TFN_TCP_CLS_CEP\n");
			printf("TCP >> [FIN2]\n");
			tcp_sht_cep(cepid);
			break;
		case TFN_TCP_SND_DAT:
			printf("case TFN_TCP_SND_DAT\n");
			printf("TCP >> [FIN]\n");
			tcp_cls_cep(cepid, TMO_NBLK);
			break;
		case TFN_TCP_RCV_DAT:
			printf("case TFN_TCP_RCV_DAT\n");
			break;
		case TFN_TCP_GET_BUF:
			printf("case TFN_TCP_GET_BUF\n");
			break;
		case TFN_TCP_SND_BUF:
			printf("case TFN_TCP_SND_BUF\n");
			break;
		case TFN_TCP_RCV_BUF:
			printf("case TFN_TCP_RCV_BUF\n");
			break;
		case TFN_TCP_REL_BUF:
			printf("case TFN_TCP_REL_BUF\n");
			break;
		case TFN_TCP_SND_OOB:
			printf("case TFN_TCP_SND_OOB\n");
			break;
		case TFN_TCP_RCV_OOB:
			printf("case TFN_TCP_RCV_OOB\n");
			break;
		case TFN_TCP_CAN_CEP:
			printf("case TFN_TCP_CAN_CEP\n");
			break;
		case TFN_TCP_SET_OPT:
			printf("case TFN_TCP_SET_OPT\n");
			break;
		case TFN_TCP_GET_OPT:
			printf("case TFN_TCP_GET_OPT\n");
			break;
		case TFN_TCP_ALL:
			printf("case 0\n");
			break;
		case TFN_UDP_CRE_CEP:
			printf("case TFN_UDP_CRE_CEP\n");
			break;
		case TFN_UDP_DEL_CEP:
			printf("case TFN_UDP_DEL_CEP\n");
			break;
		case TFN_UDP_SND_DAT:
			printf("case TFN_UDP_SND_DAT\n");
			break;
		case TFN_UDP_RCV_DAT:
			printf("case TFN_UDP_RCV_DAT\n");
			break;
		case TFN_UDP_CAN_CEP:
			printf("case TFN_UDP_CAN_CEP\n");
			break;
		case TFN_UDP_SET_OPT:
			printf("case TFN_UDP_SET_OPT\n");
			break;
		case TFN_UDP_GET_OPT:
			printf("case TFN_UDP_GET_OPT\n");
			break;
		default:
			printf("NOPE\n");
			break;
	}
    return(0);
}