/*
 * Copyright (c) 2015-2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_QDN_C$

/** @file qapi_netservices.h
*/

#ifndef _QAPI_NETSERVICES_H_
#define _QAPI_NETSERVICES_H_


/*********************************************************************************
 * Status codes
**********************************************************************************/
#include "qapi/qapi_net_status.h"

/*********************************************************************************
 * Utilities
**********************************************************************************/
#include "qapi/qapi_addr.h"
#include "qapi/qapi_ns_utils.h"
#include "qapi/qapi_netbuf.h"

/*********************************************************************************
 * Socket
**********************************************************************************/
#include "qapi/qapi_socket.h"

/*********************************************************************************
 * General IPv4/IPv6 Network Services
**********************************************************************************/
#include "qapi/qapi_ns_gen_v4.h"
#include "qapi/qapi_ns_gen_v6.h"

/*********************************************************************************
 * DHCPv6 Client
**********************************************************************************/
#include "qapi/qapi_dhcpv6c.h"

/*********************************************************************************
 * DHCPv4 Client
**********************************************************************************/
#include "qapi/qapi_dhcpv4c.h"

/*********************************************************************************
 * DHCPv4 Server
**********************************************************************************/
#include "qapi/qapi_dhcpv4s.h"

/*********************************************************************************
 * Bridging
**********************************************************************************/

/*********************************************************************************
 * HTTP Client
**********************************************************************************/
#include "qapi/qapi_httpc.h"

/*********************************************************************************
 * HTTP Server
**********************************************************************************/
#include "qapi/qapi_httpsvr.h"

/*********************************************************************************
 * DNS Client
**********************************************************************************/
#include "qapi/qapi_dnsc.h"

/*********************************************************************************
 * DNS Server
**********************************************************************************/
#include "qapi/qapi_dnss.h"

/*********************************************************************************
 * SNTP Client
**********************************************************************************/
#include "qapi/qapi_sntpc.h"

/*********************************************************************************
 * DNS Service Discovery client
 *********************************************************************************/
#include "qapi/qapi_dnssd.h"

/*********************************************************************************
 * Multicast DNS Responder
 *********************************************************************************/
#include "qapi/qapi_mdnss.h"

#endif /* _QAPI_NETSERVICES_H_ */
