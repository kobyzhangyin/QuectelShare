/*! \file pmip_mag_proc.c
 * \brief
 * \author OpenAir3 Group
 * \date 12th of October 2010
 * \version 1.0
 * \company Eurecom
 * \project OpenAirInterface
 * \email: openair3@eurecom.fr
 */
#define PMIP
#define PMIP_MAG_PROC_C
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif
#include <sys/sysctl.h>
//---------------------------------------------------------------------------------------------------------------------
#include "pmip_mag_proc.h"
#include "pmip_handler.h"
#include "pmip_hnp_cache.h"
#include "pmip_tunnel.h"
//---------------------------------------------------------------------------------------------------------------------
#include "rtnl.h"
#include "tunnelctl.h"
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
//---------------------------------------------------------------------------------------------------------------------
void pmip_timer_retrans_pbu_handler(struct tq_elem *tqe);
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
void pmip_timer_retrans_pbu_handlerv4(struct tq_elem *tqe);
//---------------------------------------------------------------------------------------------------------------------


static int pmip_cache_get_prefix_match(void *data, void *arg, void *usr_data)
	//---------------------------------------------------------------------------------------------------------------------
{
	pmip_entry_t *bce = (pmip_entry_t *) data;
	assert(bce && arg && usr_data);

	struct in6_addr * rcvd_prefix = (struct in6_addr *)arg;
	struct in6_addr in_prefix;
	uint8_t * prefix_match = (uint8_t *)usr_data;

	*prefix_match = 0;
	in_prefix = get_node_prefix(rcvd_prefix);

	if(bce->type == BCE_PMIP &&
			IN6_ARE_ADDR_EQUAL(&in_prefix, &bce->mn_prefix))
	{
		*prefix_match = 1;
		dbg("#pmip_cache_get_prefix_match to prefix: %x:%x:%x:%x:%x:%x:%x:%x & prefix_match:%d \n",NIP6ADDR(&bce->mn_prefix), *prefix_match);
		return 1;
	}
	return 0;
}


//---------------------------------------------------------------------------------------------------------------------
int mag_setup_route(struct in6_addr *pmip6_addr, int downlink)
{
	int res = 0;
	uint8_t prefix_exist = 0;
	struct in6_addr in_prefix;

	if (conf.TunnelingEnabled) {
		pmip_cache_iterate_ext(pmip_cache_get_prefix_match, (void*)pmip6_addr, (void *)&prefix_exist);
		if(prefix_exist == 1)
		{
			dbg("mag_setup_route: Matching prefix BCE entry exists. Do not Add setup rules\n", NIP6ADDR(pmip6_addr));
			return 1;
		}

		in_prefix = get_node_prefix(pmip6_addr);
		//add a rule for MN for uplink traffic from MN must query the TABLE for PMIP --> tunneled
		dbg("Uplink: Add new rule for tunneling src=%x:%x:%x:%x:%x:%x:%x:%x/64\n", NIP6ADDR(&in_prefix));
		res = rule_add(NULL, RT6_TABLE_PMIP, IP6_RULE_PRIO_PMIP6_FWD, RTN_UNICAST, &in_prefix, 64, &in6addr_any, 0, 0);
		if (res < 0) {
			dbg("ERROR Add new rule for tunneling\n");
		}
		//add a route for downlink traffic through LMA (any src) ==> MN
		dbg("Downlink: Add new route for %x:%x:%x:%x:%x:%x:%x:%x in table %d\n", NIP6ADDR(pmip6_addr), RT6_TABLE_MIP6);
		res |= route_add(downlink, RT6_TABLE_MIP6, RTPROT_MIP, 0, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, pmip6_addr, 64, NULL);
		if (res < 0) {
			dbg("ERROR Add new rule for tunneling\n");
		}
	} else {
		dbg("WARNING CANNOT ADD ROUTING RULES SINCE TUNNELING DISABLED IN CONFIG\n");
	}
	return res;
}

int mag_setup_v4_route(struct in_addr mn_v4_addr, char *table)
{
	char command[200];
	if (table == NULL)
	{
		dbg("Table name pointer is NULL\n");
		return -1;
	}
	snprintf (command,200,"ip rule del from %s table %s",inet_ntoa(mn_v4_addr), table);
	pmip_system_call(command,strlen(command),1);
	//adding Route uplink
	snprintf (command,200,"ip rule add from %s table %s",inet_ntoa(mn_v4_addr), table);
	pmip_system_call(command,strlen(command),1);
	//downlink Route are added while running dnsmasq for network
	dbg("mag_setup_v4_route added route for %s\n" , inet_ntoa(mn_v4_addr));
	return 1;
}

int mag_remove_v4_route(struct in_addr mn_v4_addr, char *table)
{
	char command[200];
	if (table == NULL)
	{
		dbg("Table name pointer is NULL\n");
		return -1;
	}
	//Removing Route
	snprintf (command,200,"ip rule del from %s table %s",inet_ntoa(mn_v4_addr), table);
	pmip_system_call(command,strlen(command),1);
	dbg("mag_remove_v4_route removed route for %s\n" , inet_ntoa(mn_v4_addr));
	return 1;

}
int mag_setup_v6_route(struct in6_addr mn_v6_addr, char *table)
{
	char command[200]={0};;
	char str[INET6_ADDRSTRLEN];
	uint8_t prefix_exist = 0;
	struct in6_addr in_prefix;
	if (table == NULL)
	{
		dbg("Table name pointer is NULL\n");
		return -1;
	}
	pmip_cache_iterate_ext(pmip_cache_get_prefix_match, (void*)&mn_v6_addr, (void *)&prefix_exist);
	if(prefix_exist == 1)
	{
		dbg("mag_setup_v6_route: Matching prefix BCE entry exists. Do not Add setup rules\n", NIP6ADDR(&mn_v6_addr));
		return 1;
	}
	in_prefix = get_node_prefix(&mn_v6_addr);

	inet_ntop(AF_INET6, &in_prefix,  str, INET6_ADDRSTRLEN);

	//adding Route uplink
	snprintf (command,200,"ip -6 rule add from %s/%d table %s",str,PREFIX_LENGTH,table);
	pmip_system_call(command,strlen(command),1);
	//downlink Route
	snprintf (command,200,"ip -6 r add %s/%d dev %s",str,PREFIX_LENGTH,
				BRIDGE_IFACE);
	pmip_system_call(command,strlen(command),1);
	dbg("mag_setup_v6_route added route for %s\n" , str);
	return 1;
}

int mag_remove_v6_route(struct in6_addr mn_v6_addr, char *table, int force_remove)
{
	char command[200]={0};
	char str[INET6_ADDRSTRLEN];
	uint8_t prefix_exist = 0;
	struct in6_addr in_prefix;
	if (table == NULL)
	{
		dbg("Table name pointer is NULL\n");
		return -1;
	}
	pmip_cache_iterate_ext(pmip_cache_get_prefix_match, (void*)&mn_v6_addr, (void *)&prefix_exist);
	if((force_remove != 1) && (prefix_exist == 1))
	{
		dbg("mag_remove_v6_route: Matching prefix BCE entry exists. Do not remove setup rules\n", NIP6ADDR(&mn_v6_addr));
		return 1;
	}
	if (force_remove)
	{
		dbg("mag_remove_v6_route force remove all rules/route \n");
	}
	in_prefix = get_node_prefix(&mn_v6_addr);

	inet_ntop(AF_INET6, &in_prefix,  str, INET6_ADDRSTRLEN);
	//Removing Route
	snprintf (command,200,"ip -6 rule del from %s/%d table %s",str,PREFIX_LENGTH,table);
	pmip_system_call(command,strlen(command),1);
	//downlink Route delete
	snprintf (command,200,"ip -6 r del %s/%d dev %s",str,PREFIX_LENGTH,
		  BRIDGE_IFACE);
	pmip_system_call(command,strlen(command),1);
	dbg("mag_remove_v6_route removed route for %s/%d\n" , str,PREFIX_LENGTH);
	return 1;
}



//---------------------------------------------------------------------------------------------------------------------
int mag_remove_route(struct in6_addr *pmip6_addr, int downlink, int force_remove)
{
	int res = 0;
	uint8_t prefix_exist = 0;
	struct in6_addr in_prefix;
	if (conf.TunnelingEnabled) {
		pmip_cache_iterate_ext(pmip_cache_get_prefix_match, (void*)pmip6_addr, (void *)&prefix_exist);
		if((force_remove != 1) && (prefix_exist == 1))
		{
			dbg("mag_remove_route: Matching prefix BCE entry exists. Do not remove setup rules\n", NIP6ADDR(pmip6_addr));
			return 1;
		}
		if (force_remove)
		{
			dbg("mag_remove_route force remove all rules/route \n");
		}

		in_prefix = get_node_prefix(pmip6_addr);
		//Delete existing rule for the deleted MN
		dbg("Uplink: Delete old rule for tunneling src=%x:%x:%x:%x:%x:%x:%x:%x/64\n", NIP6ADDR(&in_prefix));
		res = rule_del(NULL, RT6_TABLE_PMIP, IP6_RULE_PRIO_PMIP6_FWD, RTN_UNICAST, &in_prefix, 64, &in6addr_any, 0, 0);
		if (res < 0) {
			dbg("ERROR Del new rule for tunneling \n");
		}
		//Delete existing route for the deleted MN
		dbg("Downlink: Delete old routes for: %x:%x:%x:%x:%x:%x:%x:%x from table %d\n", NIP6ADDR(pmip6_addr), RT6_TABLE_MIP6);
		res |= route_del(downlink, RT6_TABLE_MIP6, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, pmip6_addr, 64, NULL);
		if (res < 0) {
			dbg("ERROR Del new rule for tunneling\n");
		}
	} else {
		dbg("WARNING CANNOT DELETE ROUTING RULES SINCE TUNNELING DISABLED IN CONFIG\n");
	}
	return res;
}
//---------------------------------------------------------------------------------------------------------------------
int mag_dereg(pmip_entry_t * bce, int propagate,int is_v4_pmip_mode)
{
	//Delete existing route & rule for the deleted MN
	int res = 0;
	bce->type = BCE_NO_ENTRY;
	int force_remove = 0;

	//v6 route delete if v6 tunnel
	if (!is_v4_pmip_mode && conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
	{
		dbg("mag_dereg v6 route delete if v6 tunnel\n");
		res = mag_remove_route(get_mn_addr(bce), bce->link, force_remove);
	}
	//v6 route delete if v4 tunnel
	if (!is_v4_pmip_mode && conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		dbg("mag_dereg v6 route delete if v4 tunnel\n");
		res = mag_remove_v6_route(bce->mn_addr, "pmipv4", force_remove);
	}

	//v4 route delete if v6 tunnel
	if (is_v4_pmip_mode == 1 && conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
	{
		dbg("mag_dereg v4 route delete if v6 tunnel\n");
		mag_remove_v4_route(bce->mnv4_addr, "pmipv4");
	}


	//v4 route delete if v4 tunnel
	if (is_v4_pmip_mode == 1 && conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		dbg("mag_dereg v4 route delete if v4 tunnel\n");
		mag_remove_v4_route(bce->mnv4_addr,"pmipv4");
	}

	//check if  v6 Tunnel is being used
	if (conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
	{
		int usercount = tunnel_getusers(bce->tunnel);
		dbg("# of binding entries %d \n", usercount);
		if (usercount == 1) {
			route_del(bce->tunnel, RT6_TABLE_PMIP, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
		}
		//decrement users of old tunnel.
		pmip_tunnel_del(bce->tunnel);
	}
	//check if  v4 Tunnel is being used
	if (conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		//decrement users of old tunnel.
		pmipv4_tunnel_del(bce->tunnel_v4);

	}
	if (propagate) {
		dbg("Propagate the deregistration... \n");
		struct in6_addr_bundle addrs;
		struct in_addr_bundle addrs1;
		memset(&addrs, 0, sizeof(addrs));
		memset(&addrs1, 0, sizeof(addrs1));
		struct timespec Lifetime = { 0, 0 };

		if(conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
		{
			addrs.src = &conf.OurAddress;
			addrs.dst = &conf.LmaAddress;
			dbg("Create PBU for LMA for deregistration v6 ip....\n");
			mh_send_pbu(&addrs, bce, &Lifetime, 0, is_v4_pmip_mode);
		}
		if (conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
		{
			//store v4 address in v6 format will parse back v4 while sending packet
			addrs1.src = &conf.Magv4AddressEgress;
			addrs1.dst = &conf.LmaV4Address;
			dbg("Create PBU for LMA for deregistration v4 ip....\n");
			mh_send_pbu(&addrs1, bce, &Lifetime, 0, is_v4_pmip_mode);
		}
	}
	//Delete PBU cache entry
	dbg("Delete PBU entry....\n");
	return res;
}
//---------------------------------------------------------------------------------------------------------------------
int mag_start_registration(pmip_entry_t * bce, int is_v4_pmip_mode)
{
	//Create PBU and send to the LMA
	struct in6_addr_bundle addrs;
	struct in_addr_bundle addrs1;
	memset(&addrs, 0, sizeof(addrs));
	memset(&addrs1, 0, sizeof(addrs1));
	struct timespec lifetime = { 10, 0 };

	//if v6 tunnel
	if(conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
	{
		addrs.src = &conf.MagAddressEgress;
		addrs.dst = &conf.LmaAddress;
		mh_send_pbu(&addrs, bce, &conf.PBULifeTime, 0, is_v4_pmip_mode);

	}
	if (conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		//store v4 address in v6 format will parse back v4 while sending packet
		addrs1.src = &conf.Magv4AddressEgress;
		addrs1.dst = &conf.LmaV4Address;
		mh_send_pbu(&addrs1, bce, &conf.PBULifeTime, 0, is_v4_pmip_mode);
	}

	// min 4 seconds, because lifetime is sent over the network in PBU divided by 4
	// mh_send_pbu(&addrs, bce, &lifetime, 0);

	//add a new task for PBU retransmission.
	struct timespec expires;
	clock_gettime(CLOCK_REALTIME, &bce->add_time);
	tsadd(bce->add_time, conf.RetransmissionTimeOut, expires);

	//if pmip mode is v6
	if (!is_v4_pmip_mode)
	{
		dbg("Registered time for v6 pmip mode....\n");
	add_task_abs(&expires, &bce->tqe, pmip_timer_retrans_pbu_handler);
	}
	if (is_v4_pmip_mode == 1)
	{
		dbg("Registered time for v4 pmip mode....\n");
		add_task_abs(&expires, &bce->tqe, pmip_timer_retrans_pbu_handlerv4);
	}
	dbg("PBU Retransmissions Timer is registered....\n");
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int mag_end_registration(pmip_entry_t * bce, int iif, int is_v4_pmip_mode)
{
	char command[200];
	int ret;
	char fname[IFNAMSIZ]={0};
	//Reset the Retransmissions counter.
	bce->n_rets_counter = conf.MaxMessageRetransmissions;
	//Add task for entry expiry.
	dbg("Timer for Expiry is initialized: %d(s)!\n", bce->lifetime.tv_sec);
	if (is_v4_pmip_mode == 1)
	{
		pmipv4_cache_start(bce);
	}
	if(!is_v4_pmip_mode)
	{
		pmip_cache_start(bce);
	}
	//if v6 Tunnel
	if (conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
	{
		//create a tunnel between MAG and LMA.
		bce->tunnel = pmip_tunnel_add(&conf.OurAddress, &conf.LmaAddress, iif, bce->gre_key_valid, bce->gre_key);
		int usercount = tunnel_getusers(bce->tunnel);
		dbg("# of binding entries %d\n", usercount);
		if (usercount == 1) {
			dbg("Add routing entry for uplink traffic\n");
			route_add(bce->tunnel, RT6_TABLE_PMIP, RTPROT_MIP, 0, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
		}
	}
	//if v4 tunnel
	if  (conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		//create a v4 tunnel between MAG and LMA.
		bce->tunnel_v4 = pmipv4_tunnel_add(&conf.Magv4AddressEgress, &conf.LmaV4Address, iif, bce->gre_key_valid, bce->gre_key);
		int usercount = tunnelv4_getusers(bce->tunnel_v4);
		dbg("Tunnel user count %d\n", usercount);
	}

	//ading Route for Ipv6 mn ip in v6 tunnel
	if (!is_v4_pmip_mode && conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
	{
		mag_kickoff_ra(bce, 0);
		dbg("Adding route for : %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(get_mn_addr(bce)));
		mag_setup_route(get_mn_addr(bce), bce->link);
	}
	//ading Route for Ipv6 ip in v4 tunnel
	if (!is_v4_pmip_mode && conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		mag_kickoff_ra(bce, 0);
		dbg("Adding route for : %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(get_mn_addr(bce)));
		ret = tunnelv4_getname(bce->tunnel_v4,fname);
		if (ret == 0)
		{
			dbg("Get Tunnel name failed for v4 ipv4 tunnel do not add route\n");
		}
		else
		{
			//add Route
			snprintf (command,200,"ip -6 route add default dev %s table pmipv4", fname);
			system (command);
			dbg("ip v6 Route table com=%s=\n",command);
			ret = mag_setup_v6_route(bce->mn_addr, "pmipv4");
			dbg ("mag_setup_v6_route Route status %d\n",ret);
		}
	}

	//ading Route for Ipv4 mn ip in v6 tunnel
	if (is_v4_pmip_mode == 1 && conf.pmip_tunnel_mode == IP_FAMILY_V6_V01 )
	{
		//get tunnel Iface name
		ret = tunnel_getname(bce->tunnel,fname);
		if (ret == 0)
		{
			dbg("Get Tunnel name failed for v4 ipv6 tunnel\n");
		}
		else
		{
			//add Route
			snprintf (command,200,"ip route add default dev %s table pmipv4", fname);
			system (command);
			dbg("Route table com=%s=\n",command);
			ret = mag_setup_v4_route(bce->mnv4_addr, "pmipv4");
			dbg ("mag_setup_v4_route Route status %d\n",ret);
		}

	}
	//ading Route for Ipv4 mn ip in v4 tunnel
	if (is_v4_pmip_mode == 1 && conf.pmip_tunnel_mode == IP_FAMILY_V4_V01 )
	{
		ret = tunnelv4_getname(bce->tunnel_v4,fname);
		if (ret == 0)
		{
			dbg("Get Tunnel name failed for v4 ipv4 tunnel\n");
		}
		else
		{
			dbg("Got current Tunnel name =%s\n",fname);
			//add Route
			snprintf (command,200,"ip route add default dev %s table pmipv4", fname);
			system (command);
			dbg("Route table com=%s=\n",command);
			ret = mag_setup_v4_route(bce->mnv4_addr,"pmipv4");
			dbg ("mag_setup_v4_route Route status %d\n",ret);
		}
	}
	//Change the BCE type.
	bce->type = BCE_PMIP;
	dbg("New PMIP cache entry type: %d\n", bce->type);

	dbg("Out of setup route\n");
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int mag_end_registration_no_new_tunnel(pmip_entry_t * bce, int iif, int is_v4_pmip_mode)
{
	//Force the BCE type.
	bce->type = BCE_PMIP;
	dbg("PMIP cache entry type: %d\n", bce->type);
	//Reset the Retransmissions counter.
	bce->n_rets_counter = conf.MaxMessageRetransmissions;
	//Add task for entry expiry.
	dbg("Timer for Expiry is initialized: %d(s)!\n", bce->lifetime.tv_sec);
	if (!is_v4_pmip_mode)
	{
		dbg("mag_end_registration_no_new_tunnel v6 case\n");
		pmip_cache_start(bce);
		mag_kickoff_ra(bce, 0);
	}
	if (is_v4_pmip_mode == 1)
	{
		dbg("mag_end_registration_no_new_tunnel v4 case\n");
		pmipv4_cache_start(bce);

	}
	//dbg("Adding route for : %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(get_mn_addr(bce)));
	//mag_setup_route(get_mn_addr(bce), bce->link);
	//dbg("Out of setup route\n");
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int mag_force_update_registration(pmip_entry_t * bce, int iif,int is_v4_pmip_mode)
{
	//int mutex_return_code;

	// first delete current task
	/*mutex_return_code = pthread_rwlock_wrlock(&bce->lock);
	  if (mutex_return_code != 0) {
	  dbg("pthread_rwlock_wrlock(&bce->lock) %s\n", strerror(mutex_return_code));
	  }*/
	if (bce->tqe.task != NULL) {
		dbg("Deleting current BCE task\n");
		del_task(&bce->tqe);
	}
	/*mutex_return_code = pthread_rwlock_unlock(&bce->lock);
	  if (mutex_return_code != 0) {
	  dbg("pthread_rwlock_unlock(&bce->lock) %s\n", strerror(mutex_return_code));
	  }*/

	//Create PBU and send to the LMA
	struct in6_addr_bundle addrs;
	struct in_addr_bundle addrs1;
	memset(&addrs, 0, sizeof(addrs));
	memset(&addrs1, 0, sizeof(addrs1));

	if (conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
	{
		addrs.src = &conf.MagAddressEgress;
		addrs.dst = &conf.LmaAddress;
		mh_send_pbu(&addrs, bce, &conf.PBULifeTime, 0, is_v4_pmip_mode);

	}
	if  (conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		//store v4 address in v6 format will parse back v4 while sending packet
		addrs1.src = &conf.Magv4AddressEgress;
		addrs1.dst = &conf.LmaV4Address;
		mh_send_pbu(&addrs1, bce, &conf.PBULifeTime, 0, is_v4_pmip_mode);
	}

	//struct timespec lifetime = { 3, 0 };
	//add a new task for PBU retransmission.
	struct timespec expires;
	clock_gettime(CLOCK_REALTIME, &bce->add_time);
	tsadd(bce->add_time, conf.RetransmissionTimeOut, expires);
	if (is_v4_pmip_mode == 0)
	add_task_abs(&expires, &bce->tqe, pmip_timer_retrans_pbu_handler);
	else if (is_v4_pmip_mode ==1)
		add_task_abs(&expires, &bce->tqe, pmip_timer_retrans_pbu_handlerv4);
	dbg("PBU Retransmissions Timer is registered....\n");
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int mag_kickoff_ra(pmip_entry_t * bce, int send_deprecate_ra)
{
	struct in6_addr *src;
	src = malloc(sizeof(struct in6_addr));
	memset(src, 0, sizeof(struct in6_addr));
	struct iovec iov;
	struct nd_router_advert *radvert;
	adv_prefix_t prefix;
	unsigned char buff[MSG_SIZE];
	struct rdnss_info *rdnss;
	char v6add_strprim[INET6_ADDRSTRLEN] = {0};
	char v6add_strsec[INET6_ADDRSTRLEN] = {0};
	int haspridns = 0;
	int hassecdns = 0;
	size_t len = 0;
	memset(&buff, 0, sizeof(buff));
	radvert = (struct nd_router_advert *) buff;
	radvert->nd_ra_type = ND_ROUTER_ADVERT;
	radvert->nd_ra_code = 0;
	radvert->nd_ra_cksum = 0;
	radvert->nd_ra_curhoplimit = router_ad_iface.AdvCurHopLimit;
	radvert->nd_ra_flags_reserved = (router_ad_iface.AdvManagedFlag) ? ND_RA_FLAG_MANAGED : 0;
	radvert->nd_ra_flags_reserved |= (router_ad_iface.AdvOtherConfigFlag) ? ND_RA_FLAG_OTHER : 0;
	/* Mobile IPv6 ext */
	radvert->nd_ra_flags_reserved |= (router_ad_iface.AdvHomeAgentFlag) ? ND_RA_FLAG_HOME_AGENT : 0;
	if (send_deprecate_ra == 1)
	{
		radvert->nd_ra_router_lifetime =  0;
		radvert->nd_ra_reachable = 0;
		radvert->nd_ra_retransmit = 0;
	}
	else
	{
		/* if forwarding is disabled, send zero router lifetime */
		radvert->nd_ra_router_lifetime = !check_ip6_forwarding()? htons(router_ad_iface.AdvDefaultLifetime) : 0;
		radvert->nd_ra_reachable = htonl(router_ad_iface.AdvReachableTime);
		radvert->nd_ra_retransmit = htonl(router_ad_iface.AdvRetransTimer);
	}

	len = sizeof(struct nd_router_advert);
	prefix = router_ad_iface.Adv_Prefix;
	/*
	 *  add prefix options
	 */
	struct nd_opt_prefix_info *pinfo;
	pinfo = (struct nd_opt_prefix_info *) (buff + len);
	pinfo->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
	pinfo->nd_opt_pi_len = 4;
	pinfo->nd_opt_pi_prefix_len = prefix.PrefixLen;
	pinfo->nd_opt_pi_flags_reserved = (prefix.AdvOnLinkFlag) ? ND_OPT_PI_FLAG_ONLINK : 0;
	pinfo->nd_opt_pi_flags_reserved |= (prefix.AdvAutonomousFlag) ? ND_OPT_PI_FLAG_AUTO : 0;
	/* Mobile IPv6 ext */
	pinfo->nd_opt_pi_flags_reserved |= (prefix.AdvRouterAddr) ? ND_OPT_PI_FLAG_RADDR : 0;
	if (send_deprecate_ra == 1)
	{
		pinfo->nd_opt_pi_valid_time = htonl(2);
		pinfo->nd_opt_pi_preferred_time = htonl(1);
	}
	else
	{
		pinfo->nd_opt_pi_valid_time = htonl(prefix.AdvValidLifetime);
		pinfo->nd_opt_pi_preferred_time = htonl(prefix.AdvPreferredLifetime);
	}

	pinfo->nd_opt_pi_reserved2 = 0;
	memcpy(&pinfo->nd_opt_pi_prefix, &bce->mn_prefix, sizeof(struct in6_addr));
	len += sizeof(*pinfo);
	//mobile ip extension
	if (router_ad_iface.AdvHomeAgentInfo
			&& (router_ad_iface.AdvMobRtrSupportFlag || router_ad_iface.HomeAgentPreference != 0 || router_ad_iface.HomeAgentLifetime != router_ad_iface.AdvDefaultLifetime)) {
		home_agent_info_t ha_info;
		ha_info.type = ND_OPT_HOME_AGENT_INFO;
		ha_info.length = 1;
		ha_info.flags_reserved = (router_ad_iface.AdvMobRtrSupportFlag) ? ND_OPT_HAI_FLAG_SUPPORT_MR : 0;
		ha_info.preference = htons(router_ad_iface.HomeAgentPreference);
		ha_info.lifetime = htons(router_ad_iface.HomeAgentLifetime);
		memcpy(buff + len, &ha_info, sizeof(ha_info));
		len += sizeof(ha_info);
	}

	//Rdnss info
	rdnss = (struct rdnss_info *) (buff + len);
	inet_ntop(AF_INET6, &conf.PrimaryV6Dns, v6add_strprim, sizeof v6add_strprim);
	if (strncmp (v6add_strprim, "::", sizeof("::")))
	{
		dbg("Adding primary public RDNSS In RA success rdnss ip %s\n", v6add_strprim);
		haspridns = 1;

		rdnss->primarydns = conf.PrimaryV6Dns;
	}

	inet_ntop(AF_INET6, &conf.SecondaryV6Dns, v6add_strsec, sizeof v6add_strsec);
	if(strncmp (v6add_strsec, "::", sizeof("::")))
	{
		dbg("Adding Secondary public RDNSS In RA success rdnss ip %s\n", v6add_strsec);
		hassecdns = 1;
		if (haspridns) //have both dns server
		{
			rdnss->secondarydns = conf.SecondaryV6Dns;
		}
		else //have only secondary dns server
		{
			rdnss->primarydns = conf.SecondaryV6Dns;
		}
	}

	if (!haspridns && !hassecdns)
	{
		dbg("No DNS info available. RA will not have RDNSS\n");
	}
	else
	{
		dbg("Adding RDNSS in RA\n");
		rdnss->type = RDNSS_SERVER_TYPE;
		rdnss->reserved = 0;
		if (send_deprecate_ra == 1)
		{
			rdnss->lifetime= 0;
		}
		else
		{
			rdnss->lifetime= INFINITE_LIFE_TIME;
		}

		//rdnss lenth will determine the Numebr of DNS in RA
		if (haspridns && hassecdns)
		{
		rdnss->length = (sizeof(struct rdnss_info))/sizeof(uint64_t);
		len += sizeof(struct rdnss_info);
		}
		else
		{
			rdnss->length = (sizeof(struct rdnss_info) - sizeof(struct in6_addr))/sizeof(uint64_t);
			len += (sizeof(struct rdnss_info) - sizeof(struct in6_addr)) ;
		}
	}

	iov.iov_len = len;
	iov.iov_base = (caddr_t) buff;
	int err;
	err = icmp6_send(bce->link, 255, src, &bce->mn_link_local_addr, &iov, 1);
	if (err < 0) {
		dbg("Error: couldn't send a RA message ...\n");
	} else {
		dbg("RA LL ADDRESS sent\n");
	}
	return err;
}
//---------------------------------------------------------------------------------------------------------------------
int check_ip6_forwarding(void)
{
	int forw_sysctl[] = { SYSCTL_IP6_FORWARDING };
	int value;
	int rc;
	size_t size = sizeof(value);
	FILE *fp = NULL;
	if (conf.pmip_tunnel_mode == IP_FAMILY_V4_V01)
	{
		dbg("Ipv6 forwading check is not required in ipv4 tunnel Mode\n" );
		return 0;
	}
#ifdef __linux__
	fp = fopen(PROC_SYS_IP6_FORWARDING, "r");
	if (fp) {
		rc = fscanf(fp, "%d", &value);
		fclose(fp);
		if (rc <= 0) {
			dbg("ERROR reading %s\n" ,PROC_SYS_IP6_FORWARDING);
		}
	} else
		dbg("Correct IPv6 forwarding procfs entry not found, " "perhaps the procfs is disabled, " "or the kernel interface has changed?\n");
#endif              /* __linux__ */
	if (!fp && sysctl(forw_sysctl, sizeof(forw_sysctl) / sizeof(forw_sysctl[0]), &value, &size, NULL, 0) < 0) {
		dbg("Correct IPv6 forwarding sysctl branch not found, " "perhaps the kernel interface has changed?\n");
		return (0);     /* this is of advisory value only */
	}
	if (value < 1) {
		dbg("IPv6 forwarding setting is: %u, should be 1\n", value);
		return (-1);
	}
	return (0);
}
//---------------------------------------------------------------------------------------------------------------------
int mag_get_ingress_info(int *if_index, char *dev_name_mn_link)
{
	FILE *fp;
	char str_addr[INET6_ADDRSTRLEN];
	unsigned int plen, scope, dad_status, if_idx;
	struct in6_addr addr;
	unsigned int ap;
	int i;

	char devname[32];
	if ((fp = fopen("/proc/net/if_inet6", "r")) == NULL) {
		dbg("you don't have root previleges, please logon as root, can't open %s:\n", "/proc/net/if_inet6");
		return -1;
	}
	// first find the device name
	while (fscanf(fp, "%32s %x %02x %02x %02x %15s\n", str_addr, &if_idx, &plen, &scope, &dad_status, devname) != EOF) {
		for (i = 0; i < 16; i++) {
			sscanf(str_addr + i * 2, "%02x", &ap);
			addr.s6_addr[i] = (unsigned char) ap;
		}
		if (memcmp(&conf.MagAddressIngress, &addr, sizeof(struct in6_addr)) == 0) {
			strncpy(dev_name_mn_link, devname, 32);
			*if_index = if_idx;
			dbg("The interface name of the device that is used for communicate with MNs is %s\n", dev_name_mn_link);
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	dbg("No interface name of the device that is used for communicate with MNs found\n");
	return -1;
}
//---------------------------------------------------------------------------------------------------------------------
int setup_linklocal_addr(struct in6_addr *src)
{
	FILE *fp;
	char str_addr[INET6_ADDRSTRLEN];
	unsigned int plen, scope, dad_status, if_idx;
	struct in6_addr addr;
	unsigned int ap;
	int i;
	int flagy = 0;
	char devname[32];
	char dev_name_mn_link[32];
	if ((fp = fopen("/proc/net/if_inet6", "r")) == NULL) {
		dbg("you don't have root previleges, please logon as root, can't open %s:", "/proc/net/if_inet6");
		return -1;
	}
	// first find the device name
	while (fscanf(fp, "%32s %x %02x %02x %02x %15s\n", str_addr, &if_idx, &plen, &scope, &dad_status, devname) != EOF) {
		for (i = 0; i < 16; i++) {
			sscanf(str_addr + i * 2, "%02x", &ap);
			addr.s6_addr[i] = (unsigned char) ap;
		}
		if (memcmp(&conf.MagAddressIngress, &addr, sizeof(struct in6_addr)) == 0) {
			strncpy(dev_name_mn_link, devname, 32);
			flagy = 1;
			dbg("The interface name of the device that is used for communicate with MNs is %s\n", dev_name_mn_link);
			break;
		}
	}
	fclose(fp);
	if ((fp = fopen("/proc/net/if_inet6", "r")) == NULL) {
		dbg("can't open %s:", "/proc/net/if_inet6");
		return -1;
	}
	while (fscanf(fp, "%32s %x %02x %02x %02x %15s\n", str_addr, &if_idx, &plen, &scope, &dad_status, devname) != EOF) {
		if (scope == IPV6_ADDR_LINKLOCAL && strcmp(devname, dev_name_mn_link) == 0) //we have to store the interface name from which we get the router solicitation
		{
			dbg("entered the if to get %s iface ll address ", dev_name_mn_link);
			flagy = 1;
			for (i = 0; i < 16; i++) {
				sscanf(str_addr + i * 2, "%02x", &ap);
				addr.s6_addr[i] = (unsigned char) ap;
			}
			dbg("PMIP cache entry is found for: %x:%x:%x:%x:%x:%x:%x:%x \n", NIP6ADDR(&addr));
			*src = addr;
		}
	}
	if (flagy == 0) {
		dbg("no link local address configured ");
		fclose(fp);
		return -1;
	} else {
		fclose(fp);
		return 1;
	}
}
//---------------------------------------------------------------------------------------------------------------------
int mag_update_binding_entry(pmip_entry_t * bce, msg_info_t * info)
{
	dbg("Store binding entry\n");
	bce->our_addr = conf.OurAddress;
	bce->mn_suffix = info->mn_iid;
	bce->mn_hw_address = info->mn_hw_address;
	bce->mn_prefix = info->mn_prefix;
	bce->mn_addr = info->mn_addr;
	dbg("mag_update_binding_entry v6 address: %x:%x:%x:%x:%x:%x:%x:%x, \n", NIP6ADDR(&bce->mn_addr));
	bce->mn_link_local_addr = info->mn_link_local_addr;
	bce->mn_serv_mag_addr = info->src;
	bce->mn_serv_mag_v4_addr = info->srcv4;
	bce->lifetime = info->lifetime;
	bce->n_rets_counter = conf.MaxMessageRetransmissions;
	bce->seqno_in = info->seqno;
	bce->link = info->iif;
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int mag_pmip_md(msg_info_t * info, pmip_entry_t * bce, int is_v4_pmip_mode)
{
	if (bce != NULL) {
		bce->our_addr = conf.OurAddress;
		bce->mn_suffix = info->mn_iid;
		if (is_v4_pmip_mode == 1)
		{
			dbg("mag_pmip_md v4 ...\n");

		}
		if (!is_v4_pmip_mode)
		{
			dbg("mag_pmip_md v6 ...\n");
			bce->mn_prefix = info->mn_prefix;
			dbg("Making BCE entry in MAG with HN prefix  %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&bce->mn_prefix));
		}
		if (conf.pmip_tunnel_mode == IP_FAMILY_V6_V01)
		{
		bce->mn_serv_mag_addr = conf.OurAddress;
		bce->mn_serv_lma_addr = conf.LmaAddress;
		}
		else
		{
			bce->mn_serv_mag_v4_addr = conf.Magv4AddressEgress;
			bce->mn_serv_lma_v4_addr = conf.LmaV4Address;
		}
		bce->seqno_out = 0;
		bce->PBU_flags = IP6_MH_BU_ACK | IP6_MH_BU_PR | IP6_MH_BU_HOME;
		bce->link = info->iif;
		struct in6_addr *link_local = link_local_addr(&bce->mn_suffix);
		bce->mn_link_local_addr = *link_local;  // link local address of MN
		dbg("New attachment detected! Start Location Registration procedure...\n");
		bce->type = BCE_TEMP;
		mag_start_registration(bce, is_v4_pmip_mode);
	}
	return 0;
}

int mag_update_binding_entry_with_DMNP_Home_addr(pmip_entry_t * bce, msg_info_t * info)
{
	dbg("mag_update_binding_entry_with_DMNP_Home_addr enter \n");
	if (info->pmipv4_hnp_reply_valid)
	{
		bce->pmipv4_hnp_reply_valid = 1;
		bce->v4_home_prefix = info->v4_home_prefix;
		bce->v4_home_prefixlen= info->v4_home_prefixlen;
		dbg("has Home_addr\n");

	}
	if (info->hasipv4_dmnp_prefix_option)
	{
		bce->hasipv4_dmnp_prefix_option = 1;
		bce->dmnp_prefix= info->dmnp_prefix;
		bce->dmnp_prefix_len= info->dmnp_prefix_len;
		dbg("has DMNP \n");
	}
}


