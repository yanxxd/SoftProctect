#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> //for gethostbyname
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>//pid_t fork(void);
#include <signal.h>
#include <string>
#include <assert.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <dlfcn.h>

#include "kmp.h"

#include "kmp_client_check.cpp"
#include "libkmp.h"
#include "md5.h"
#include "ptp.h"

using namespace std;

#define MAX_BUF 4096

char *g_ip = NULL;

int recv_printf(SSL *ssl);
char kmp_rand_str(char str[], const int len, SSL *ssl, bool protect=true);
int  kmp_getIndexOf(std::string s, std::string m, SSL *ssl, bool protect=true);
void compute_time(struct timeval *start, struct timeval *end, struct timeval *result);
SSL_CTX* InitCTX(void);
int  connect_serv(char *ip, int port);

int main(int argc,char *argv[]) {
	int sockfd;          //socket
	char sendBuf[MAX_BUF];
	int sendSize, recvSize;          //用于记录记录发送和接收到数据的大小
	int pid;
	void *handle = NULL;
	char lib_name[128] = "";
	bool is_protected = false;
	struct timeval start, end, exec_module_time;

	if(3 != argc){
		printf("\nUsage:\tkmp interface ip\n\n");
		return 0;
	}

	g_ip = argv[2];
	bool is_loop = true;

	//FILE *flog = fopen("time.log", "w");

	SSL_CTX *ctx = InitCTX();
	g_ssl = SSL_new(ctx);      /* create new SSL connection state */

	while (is_loop) {
		//每次都要重连新端口
		sockfd = connect_serv(g_ip, g_checksum);
		if (sockfd <= 0) {
			fprintf(stderr, "connect server port %d error\n", g_checksum);
			break;
		}
		//连接成功后先做时间同步
//		if( ptp_client(argv[1], g_ip, g_offset, g_delay) ){
//			printf("\n[!] Time Sync failed!\n\n");
//			break;
//		}
		SSL_set_fd(g_ssl, sockfd);	  /* attach the socket descriptor */
		if (1 != SSL_connect(g_ssl)) /* perform the connection */
		{
			ERR_print_errors_fp(stderr);
			break;
		}
		int len;
		while (printf("\n*************************client input command*************************\n")
				&& fgets(sendBuf, 32, stdin)) { //一个指令,每个指令代表一个具体的功能
			len = strlen(sendBuf);
			is_protected = false;
			if('p' == sendBuf[len-2] && 'p' == sendBuf[len-3]) { //尾部爲"pp"表示運行被保護程序
				is_protected = true;
				sendBuf[len - 1] = 0; //去掉尾部的'\n'
				if (SSL_write(g_ssl, sendBuf, 32) == -1) {
					fprintf(stderr, "fail to send datas.");
					break;
				}
			}

			gettimeofday(&start, NULL);
			if (strncmp(sendBuf, "quit", 4) == 0) {
				is_loop = false;
				break;
			} else	if (strncmp(sendBuf, "sort", 4) == 0) { //对sort进行校验
				printf("\n*************************check sort start*************************\n");
				strcpy(lib_name, "libtest.so");
				if(is_protected)
					strcpy(lib_name, "libtestpp.so");
				handle = dlopen(lib_name, RTLD_LAZY);
				if (!handle) {
					fprintf(stderr, "load libtest.so error:%s\n", dlerror());
					return -1;
				}
				tkmp_Sort kmp_Sort = (tkmp_Sort)dlsym(handle, "kmp_Sort");
				if (!kmp_Sort) {
					fprintf(stderr, "dlsym kmp_Sort error:%s\n", dlerror());
					return -1;
				}
				kmp_Sort();
				dlclose(handle);
				printf("\n*************************check sort end*************************\n");

			} else	if (strncmp(sendBuf, "bzip2", 5) == 0) { //对bzip2进行校验
				printf("\n*************************check bzip2 start*************************\n");
				if( kmp_bz2(g_ssl, sendBuf) )
					break;	//断开
				printf("\n*************************check bzip2 end*************************\n");

			} else if ( !strncmp(sendBuf, "bpe", 3) ) { //对bzip2进行校验
				printf("\n*************************check bpe start*************************\n");
				if( kmp_bpe(g_ssl, sendBuf) )
					break;	//断开
				printf("\n*************************check bpe end*************************\n");
			} else if ( !strncmp(sendBuf, "bignum", 6) ) { //对bzip2进行校验
				printf("\n*************************check bignum start*************************\n");
				if( kmp_bignum(g_ssl, sendBuf) )
					break;	//断开
				printf("\n*************************check bignum end*************************\n");
			} else if ( !strncmp(sendBuf, "hmmer", 5) ) { //对bzip2进行校验
				printf("\n*************************check hmmer start*************************\n");
				if( kmp_hmmer(g_ssl, sendBuf) )
					break;	//断开
				printf("\n*************************check hmmer end*************************\n");
			} else if ( !strncmp(sendBuf, "sjeng", 5) ) { //对bzip2进行校验
				printf("\n*************************check sjeng start*************************\n");
				if( kmp_sjeng(g_ssl, sendBuf) )
					break;	//断开
				printf("\n*************************check sjeng end*************************\n");
			} else if ( !strncmp(sendBuf, "h264ref", 7) ) { //对bzip2进行校验
				printf("\n*************************check h264ref start*************************\n");
				if( kmp_h264ref(g_ssl, sendBuf) )
					break;	//断开
				printf("\n*************************check h264ref end*************************\n");
			}  else if ( !strncmp(sendBuf, "gene", 4) ) { //对bzip2进行校验
				printf("\n*************************check gene start*************************\n");
				if( kmp_gene(g_ssl, sendBuf) )
					break;	//断开
				printf("\n*************************check gene end*************************\n");
			} else { //非"@"或"quit",接收服务器端发回的
				if(recv_printf(g_ssl)) {
					fprintf(stderr, "fail to recv data.\n");
					break;
				}
			}
			gettimeofday(&end, NULL);
			timeval_us_sub(&start, &end, &exec_module_time);
			printf("\n该功能总执行时间：%lds %ldus", exec_module_time.tv_sec, exec_module_time.tv_usec);
			printf("\n");

			//验证完成后发包告诉服务器端，服务器端接受其他指令
			if ( is_protected ) {
				if (SSL_write(g_ssl, "check ok", 8) <= 0) {
					break;
				}
			}

			memset(sendBuf, 0, sizeof(sendBuf));
		} // end of while (fgets(sendBuf, MAX_BUF, stdin))
		close(sockfd); //需要重新连接
		printf("\n*************************close socket*************************\n");
	} //end of 大循环

	//fclose(flog);
	close(sockfd);
	SSL_free(g_ssl);
	SSL_CTX_free(ctx);
	return 0;
}

int recv_printf(SSL *ssl)
{
	char recvBuf[MAX_BUF];
	if (SSL_read(ssl, recvBuf, MAX_BUF) <= 0) {
		fprintf(stderr, "Server maybe shutdown!\n");
		return -1;
	}
	printf("Server:%s\n", recvBuf);
	return 0;
}

void compute_time(struct timeval *start, struct timeval *end, struct timeval *result){
	if(start->tv_usec < end->tv_usec){
		result->tv_sec = end->tv_sec - start->tv_sec;
		result->tv_usec = end->tv_usec - start->tv_usec;
	} else {
		result->tv_sec = end->tv_sec - start->tv_sec - 1;
		result->tv_usec = 1000000 + end->tv_usec - start->tv_usec;
	}
}

SSL_CTX* InitCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
	SSL_library_init();
    SSL_load_error_strings();   /* Bring in and register error messages */
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

int connect_serv(char *ip, int port) {
	int sockfd;          //socket
	char sendBuf[MAX_BUF], recvBuf[MAX_BUF];
	int sendSize, recvSize;          //用于记录记录发送和接收到数据的大小
	struct sockaddr_in servAddr;
	char * p;

	//分割线，表示每一轮的开始
	printf("----------------------------------------------------------------------\n");
	printf("connect %s:%d...\n", ip, g_checksum);
	//创建socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("fail to establish a socket");
		return -1;
	}

	/*init sockaddr_in*/
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = inet_addr(ip);
	bzero(&(servAddr.sin_zero), 8);

	/*connect the socket*/
	if (connect(sockfd, (struct sockaddr *) &servAddr,
			sizeof(struct sockaddr_in)) == -1) {
		fprintf(stderr, "fail to connect the %s:%d\n", ip, port);
		return -2;
	}
	printf("Success to connect the socket...\n");

	return sockfd;
}

int recv_data(int sockfd)
{
	char recvBuf[MAX_BUF];
	if (recv(sockfd, recvBuf, MAX_BUF, 0) <= 0) {
		fprintf(stderr, "Server maybe shutdown!\n");
		return -1;
	}
	printf("Server:%s\n", recvBuf);
	if (strncmp(recvBuf, "OK", 2) == 0) {
		strtok(recvBuf, " ");
		strcpy(g_cha_input, strtok(NULL, " "));
		printf("recv_data:x:%s\n", g_cha_input);
	}
	return 0;
}
