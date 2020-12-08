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
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2014 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* File Name    : r_sys_time.c
* Version      : 1.00
* Device(s)    :
* Tool-Chain   :
* OS           : none
* H/W Platform :
* Description  : This is T4 application system time program code.
* Operation    :
******************************************************************************/
/******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 09.05.2014 1.00    First Release
******************************************************************************/

/******************************************************************************
Includes <System Includes> , "Project Includes"
******************************************************************************/
#include "r_t4_itcpip.h"
#include "r_t4_http_server_rx_if.h"
#include "r_cmt_rx_if.h"
#include "r_sys_time.h"

/******************************************************************************
Macro definitions
******************************************************************************/
#define SYSTEM_TIME_CNT_FREQ			100		// system timer frequency 100 Hz ( = 10 ms )
#define SYSTEM_TIME_CMT_CHANNEL_INVALID	0xFF	// invalid channel

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/
/******************************************************************************
Private global variables and functions
******************************************************************************/
static uint32_t system_time_cmt_channel = SYSTEM_TIME_CMT_CHANNEL_INVALID;
static void	update_sys_time( void *pdata );

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/
SYS_TIME sys_time;

void	*get_sys_time( void )
{
	return	&sys_time;
}

/* Start system time	*/
void	start_system_time( void )
{
	sys_time.sec 	= 0;
	sys_time.min 	= 0;
	sys_time.hour 	= 0;
	sys_time.day 	= 16;
	sys_time.month 	= 4;
	sys_time.year 	= 2013;

	if (true != R_CMT_CreatePeriodic(SYSTEM_TIME_CNT_FREQ, update_sys_time, &system_time_cmt_channel))
	{
		system_time_cmt_channel = SYSTEM_TIME_CMT_CHANNEL_INVALID;
		while(1)
		{
			/* infinite loop(setting error) */
		}
	}
}

/* Update system time every second */
void	update_sys_time( void *pdata )
{
	static int32_t _1s_timer;
	static const uint8_t stTime[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	uint8_t	n;

	/* for Web server timer */
	_1s_timer++;
	if((_1s_timer%100)==0){
		_1s_timer = 0;
		if (++sys_time.sec >= 60) 
		{
			sys_time.sec = 0;
			if (++sys_time.min >= 60) 
			{
				sys_time.min = 0;
				if (++sys_time.hour >= 24) 
				{
					sys_time.hour = 0;
					n = stTime[sys_time.month - 1];
					if ((n == 28) && !(sys_time.year & 3))
					{
						n++;
					}
					if (++sys_time.day > n) 
					{
						sys_time.day = 1;
						if (++sys_time.month > 12) 
						{
							sys_time.month = 1;
							sys_time.year++;
						}
					}
				}
			}
		}	
	}
}

/* Stop system time */
void	stop_system_time( void )
{
	if(SYSTEM_TIME_CMT_CHANNEL_INVALID == system_time_cmt_channel)
	{
		return;
	}

	/* frees the CMT channel and disabling interrupt. */
	if (true != R_CMT_Stop(system_time_cmt_channel))
	{
		/* The CMT channel could not be closed or it was not open */
	}
}
