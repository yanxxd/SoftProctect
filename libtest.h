/*
 * libtest.h
 *
 *  Created on: Sep 26, 2018
 *      Author: yan
 *      要保护的模块，测试用
 */

#ifndef LIBTEST_H_
#define LIBTEST_H_


#include <sys/time.h>
#include <string.h>
#include <string>

int             g_server_port = 8443;       //两端用来同步的一个参数，kmp1中用这个作为新的端口，kmp2中只是做比较
char            g_cha_input[256];			//x作为种子
//unsigned int   g_seed = 0;                 //种子
unsigned int   g_timestamp = 0;            //时间戳
//std::mt19937    g_mt;						//伪随机数生成

struct timeval 	g_time_consume_org = {0};	//原函数花费时间
struct timeval 	g_time_consume_sock = {0};	//端口重连花费时间
struct timeval 	g_time_consume = {0};		//加保护后花费时间

char rand_str(char str[], const int len);
int  getIndexOf(std::string s, std::string m);
char rand_str_org(char str[], const int len);
int  getIndexOf_org(std::string s, std::string m);


#endif /* LIBTEST_H_ */
