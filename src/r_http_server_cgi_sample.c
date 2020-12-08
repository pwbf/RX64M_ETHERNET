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
* File Name    : r_http_server_cgi_sample.c
* Version      : 1.00
* Device(s)    : Renesas
* Tool-Chain   : Renesas
* OS           : None
* H/W Platform : Independent
* Description  : This is HTTP server cgi sample source.
******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
*         : 04.08.2014 1.00    First Release
*         : 31.10.2014 1.01    Adds DNS demo function and callback
******************************************************************************/

/******************************************************************************
Includes <System Includes> , "Project Includes"
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "led.h"
#include "r_t4_itcpip.h"
#include "r_t4_http_server_rx_config.h"
#include "r_t4_http_server_rx_if.h"
#include "r_t4_dns_client_rx_if.h"

/* --------------------------------------------------------------
	CGI function interface:
-----------------------------------------------------------------

	Prototype:
		ER cgi_sample_function(ID cepid, void *res_info);

	Argument:
		ID cepid
			Corresponds communication endpoint established peer.
		void *res_info
			Includes pointer to response information.
			User needs to make responce body and response body size
			and copy these data to this pointer area.

			res_info->body and res_info->body_size are needed
			to be set the value.

	Return value:
		ER
			error code:
				 0: Normal termination
				-1: Internal error
				-2: CGI pending

	Description:
		CGI function that is defined as CGI_FILE_NAME_TABLE_LIST in
		"r_t4_http_server_config.h" .
		The second element (cgi function pointer) of CGI_FILE_NAME_TABLE_LIST
		will be called when web browser requests the defined cgi file URL.
		And next, HTTPd will call cgi function.
		HTTPd behavior will be changed by the return value.

		case: Normal termination
			CGI process finishes in this function.
		case: Internal error
			CGI process errors occur in this function.
		case: CGI pending
			CGI process does not finish in this function.
			The third element (cgi function pointer) of CGI_FILE_NAME_TABLE_LIST
			will be called when user will call R_httpd_pending_release_request()
			in finishing CGI process.



	Sample sequence:
		1. execute user code.
		2. generate html file and copy to res_info_p->body
		3. calculate html file size and copy to res_info_p->body_size

-------------------------------------------------------------- */

static ID		cgi_dns_cepid;
static uint8_t	chk_domain_name[HTTP_TCP_CEP_NUM][256];
static NAME_TABLE dns_name_table[HTTP_TCP_CEP_NUM];
static int32_t		dns_ercd[HTTP_TCP_CEP_NUM];

void cgi_dns_calback(int32_t ercd, NAME_TABLE *name_table);

ER cgi_sample_function(ID cepid, void *res_info)
{
	static uint8_t led_init_flag = 0;
	HTTPD_RESOURCE_INFO *res_info_p;

	res_info_p = (HTTPD_RESOURCE_INFO*)res_info;

	if (led_init_flag == 0)
	{
		/* init port direction */
		led_init();
		led_init_flag = 1;
	}

	switch (res_info_p->param[0])
	{
		case '1':
			if (LED0 == LED_ON)
			{
				LED0 = LED_OFF;
			}
			else
			{
				LED0 = LED_ON;
			}
			break;
		case '2':
			if (LED1 == LED_ON)
			{
				LED1 = LED_OFF;
			}
			else
			{
				LED1 = LED_ON;
			}
			break;
		case '3':
			if (LED2 == LED_ON)
			{
				LED2 = LED_OFF;
			}
			else
			{
				LED2 = LED_ON;
			}
			break;
		case '4':
			if (LED3 == LED_ON)
			{
				LED3 = LED_OFF;
			}
			else
			{
				LED3 = LED_ON;
			}
			break;
		default:
			break;
	}

	sprintf((char*)res_info_p->res.body, "<html><body>request accepted.</body></html>");
	res_info_p->res.body_size = strlen((char*)res_info_p->res.body);

	return 0;
}

ER cgi_dns_demo_function(ID cepid, void *res_info)
{
	int32_t dns_func_ret;
	HTTPD_RESOURCE_INFO *res_info_p;

	memset(chk_domain_name[cepid-1], 0, sizeof(chk_domain_name));
	res_info_p = (HTTPD_RESOURCE_INFO*)res_info;

	if (('x' == res_info_p->param[0])
	        && ('t' == res_info_p->param[1])
	   )
	{
		strncat((char *)chk_domain_name[cepid-1], (const char *)&res_info_p->param[3], sizeof(chk_domain_name[cepid-1]));

		if (strlen((const char *)chk_domain_name[cepid-1]) < sizeof(chk_domain_name[cepid-1]))
		{
			/* Make request accepted page */
			dns_func_ret = R_dns_resolve_name((char *)chk_domain_name[cepid-1], (DNS_CB_FUNC)cgi_dns_calback);
			if (dns_func_ret == E_OK)
			{
				cgi_dns_cepid = cepid;
				/* pend sending  */
				return -2;
			}
		}
	}
	sprintf((char*)res_info_p->res.body, "<html><body>request not accepted.</body></html>");
	res_info_p->res.body_size = strlen((char*)res_info_p->res.body);
	return 0;

}

/* dns client callback */
void cgi_dns_calback(int32_t ercd, NAME_TABLE *name_table)
{
	if (E_OK == ercd)
	{
		memcpy(&dns_name_table[cgi_dns_cepid-1], name_table, sizeof(NAME_TABLE));
	}
	dns_ercd[cgi_dns_cepid-1] = ercd;
	R_httpd_pending_release_request(cgi_dns_cepid);
}

ER cgi_dns_demo_pending_release(ID cepid, void *res_info)
{
	uint8_t	res_str[256];
	HTTPD_RESOURCE_INFO *res_info_p;

	memset(res_str, 0, sizeof(res_str));
	res_info_p = (HTTPD_RESOURCE_INFO*)res_info;

	if (E_OK == dns_ercd[cepid-1])
	{
		sprintf((char *)res_str, "<html><body>IP address : %d.%d.%d.%d", dns_name_table[cepid-1].ipaddr[0],
		        dns_name_table[cepid-1].ipaddr[1],
		        dns_name_table[cepid-1].ipaddr[2],
		        dns_name_table[cepid-1].ipaddr[3]);
		strncat((char *)res_str, "</body></html>", sizeof(res_str));
	}
	else
	{
		sprintf((char *)res_str, "<html><body>Not found!!");
		strncat((char *)res_str, "</body></html>", sizeof(res_str));

	}

	memcpy((char*)res_info_p->res.body, res_str, strlen((const char *)res_str) + 1);
	res_info_p->res.body_size = strlen((char*)res_info_p->res.body);

	return 0;
}

/******************************************************************************
End of file
******************************************************************************/
