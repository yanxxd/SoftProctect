
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <iostream>
#include <map>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>
//#include <mutex> C++11

#include "kmp.h"

#define LISTEN_PORT 8443

bool g_bdisconnect = false;	//校验不过时，是否断开连接

void *thread_work(void *param);

int printf_current_time();

SSL_CTX* InitServerCTX(const char *CertFile, const char *KeyFile);

int main(int argc, char* argv[])
{
	if(2 != argc){
		printf("usage:%s 0|1\n", argv[0]);
		return 0;
	}
	//signal(SIGCHLD, SIG_IGN);
	if(strcmp(argv[1], "1"))
		g_bdisconnect = true;

	int server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in server_sockaddr;
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(LISTEN_PORT);
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);


	if (bind(server_sockfd, (struct sockaddr *) &server_sockaddr, sizeof(server_sockaddr)) == -1) {
		perror("bind err\n");
		exit(1);
	}

	if (listen(server_sockfd, 10) == -1) {
		perror("listen err\n");
		exit(1);
	}

	int conn;
	struct sockaddr_in client_addr;
	int length;

    SSL_CTX *ctx = InitServerCTX("./certname_cert.pem", "./certname_key.pem");  /* init, load certs */

	while (1) {
		length = sizeof(client_addr);
		conn = accept(server_sockfd, (struct sockaddr*) &client_addr, (socklen_t*)&length);
		if (conn < 0) {
			perror("accept error\n");
			break;
		}
		if(inet_addr("127.0.0.1") == client_addr.sin_addr.s_addr){
			printf("127.0.0.1 connect\n");
		}
		else{
			printf("%s connect\n", inet_ntoa(client_addr.sin_addr));
			if(0 == fork())	{
				SSL *ssl = SSL_new(ctx); /* get new SSL state with context */
				SSL_set_fd(ssl, conn);   /* set connection socket to SSL state */
				thread_work(ssl);
				break;
			}
			else {
				close(conn);
			}
		    //pthread_t pt;
			//pthread_create(&pt, NULL, thread_work, (void*)(long long)conn);
		}
	}

	close(server_sockfd);
	SSL_CTX_free(ctx);
	return 0;
}

int printf_current_time() {
	time_t timep;
	time(&timep);
	printf("%s\n", ctime(&timep));
	return 0;
}

int gen_random_str(char *buf, int len)
{
	for(int i=0; i < len; ++i)
	{
		buf[i] = rand()%26 + 'a';
	}
	return len;
}

//根据函数名调用收保护函数
//4B checksum + fun_name + param1 + param2 + ...
int call_protect_fun(SSL *ssl){
	char buf[1024];
	//生成x
	int len = gen_random_str(g_cha_input, rand() % 12 + 8);
	g_cha_input[len] = 0;
	printf("x:%s\n", g_cha_input);

	//发送x
	len = sprintf(buf, "OK %s", g_cha_input);
	if (SSL_write(ssl, buf, len + 1) <= 0) {
		return -2;
	}

	//接收函数名 参数
	//调用函数，函数中自动设置g_server_port
	len = SSL_read(ssl, buf, sizeof(buf) - 20);
	if (len <= 0) {
		return -1;
	}
	//头部4B为两端校验数，不相等，说明客服端程序被篡改了
	if( g_bdisconnect && *(unsigned int*)buf != g_server_port ){
		printf("error port:%d != %d\n", *(unsigned int*)buf, g_server_port);
		return -3;
	} else {
		printf("g_server_port = %u\n", g_server_port);
	}
	char *fun = (char*)buf + 4;
	if (!strncmp(fun, "rand_str", 8)) {
		//rand_str + n
		int n = atoi(fun + 9);
		char *str = new char[n];
		printf("\nrand_str(%d)", n);
		rand_str(str, n);
		printf("\n====%s====\n", str);
		delete str;

	} else if (!strncmp(fun, "getIndexOf", 10)) {
		//getIndexOf + str + mstr
		char *p = strtok(fun, " ");
		char *str = strtok(NULL, " ");
		char *mstr = strtok(NULL, " ");
		printf("getIndexOf(%s, %s)\n", str, mstr);
		int mi = getIndexOf(str, mstr);
		printf("The index of %s in %s is %d\n", mstr, str, mi);
	}
	
	return 0;
}

void *thread_work(void *pParam)
{
	SSL *ssl = (SSL *)pParam;
	char buf[1024];
	char buf_send[1024];
	int len;
	int conn = SSL_get_fd(ssl);

    if(1 != SSL_accept(ssl)){
		ERR_print_errors_fp(stderr);
		close(conn);
		SSL_free(ssl);
		return NULL;
	}

	while (true) {
		//1. 设置伪随机数种子
	    srand(time(NULL));

		//2. 接受命令
		len = SSL_read(ssl, buf, 32);
		if (len <= 0) {
			break;
		}
		buf[len] = 0;
		printf("recv:%s\n", buf);

		if (!strncmp("quit", buf, 4)) {
			break;
		} else if (!strncmp("@", buf, 1)) {
			//调用3个函数，需要循环3次
			for(int i=0; i<3; ++i){
				if(call_protect_fun(ssl))
					break;
			}
		} else {
			len = sprintf(buf_send, "Hello, %s", buf);
			if (SSL_write(ssl, buf_send, len + 1) <= 0) {
				break;
			}
		}

	}//end of while (true)

	struct linger ling = {1, 0};
	setsockopt(conn, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
	close(conn);
	SSL_free(ssl);
	return NULL;
}

SSL_CTX* InitServerCTX(const char *CertFile, const char *KeyFile)
{
	SSL_CTX *ctx = NULL;
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
	const SSL_METHOD *method;
#else
	SSL_METHOD *method;
#endif
	SSL_library_init();
    SSL_load_error_strings();   /* load all error messages */
    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    method = TLSv1_2_server_method(); /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
	//New lines
	if (SSL_CTX_load_verify_locations(ctx, CertFile, KeyFile) != 1)
		ERR_print_errors_fp(stderr);
	if (SSL_CTX_set_default_verify_paths(ctx) != 1)
		ERR_print_errors_fp(stderr);
	//End new lines
	/* set the local certificate from CertFile */
	if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		abort();
	}
	/* set the private key from KeyFile (may be the same as CertFile) */
	if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		abort();
	}
	/* verify private key */
	if (!SSL_CTX_check_private_key(ctx))
	{
		fprintf(stderr, "Private key does not match the public certificaten");
		abort();
	}
	printf("LoadCertificates Compleate Successfully.....\n");
    return ctx;
}