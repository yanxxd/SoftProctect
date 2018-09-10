/*
 * kmp.h
 *
 *  Created on: May 8, 2018
 *      Author: yan
 */

#ifndef KMP_H_143ef87b_3aec_47f6_a33a_ed51e3e965ac
#define KMP_H_143ef87b_3aec_47f6_a33a_ed51e3e965ac

#include <sys/time.h>
#include <string.h>
#include <string>

bool            g_is_server = false;		//服务器端为true 客户端为false
int             g_server_port = 8443;       //两端用来同步的一个参数，kmp1中用这个作为新的端口，kmp2中只是做比较
char            g_cha_input[256];
unsigned int   g_seed = 0;                 //种子
unsigned int   g_timestamp = 0;            //时间戳
//std::mt19937    g_mt;						//伪随机数生成
struct timespec g_offset;					//客户端和服务器的时间差 慢多少
struct timespec g_delay;					//线路延迟
time_t    		 g_timeslice = 200 * 1000 * 1000; //200ms一个时间片,变为ns表示

struct timeval 	g_time_consume_org = {0};	//原函数花费时间
struct timeval 	g_time_consume_sock = {0};	//端口重连花费时间
struct timeval 	g_time_consume = {0};		//加保护后花费时间

char rand_str(char str[], const int len);
int  getIndexOf(std::string s, std::string m);
char rand_str_org(char str[], const int len);
int  getIndexOf_org(std::string s, std::string m);
int  connect_serv(char *ip, int port);
unsigned int easy_hash(unsigned char *buf, unsigned int len);


#endif /* KMP_H_ */
