
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

#include "ptp.h"
#include "libkmp.h"
#include "libtest.h"
#include "kmp_server_check.cpp"
//#include <mutex> C++11

#define LISTEN_PORT 8443

bool g_bdisconnect = false;	//校验不过时，是否断开连接

int gen_random_str(char *buf, int len);

void *thread_work(void *param);

int printf_current_time();

SSL_CTX* InitServerCTX(const char *CertFile, const char *KeyFile);

int main(int argc, char* argv[])
{
	if(3 != argc){
		printf("usage:%s interface 0|1\n", argv[0]);
		return 0;
	}

	g_is_server = true;

	//signal(SIGCHLD, SIG_IGN);
	if(!strcmp(argv[2], "1"))
		g_bdisconnect = true;

	int sock_time;
	if( ptp_server_bind(argv[1], sock_time) ){
		perror("\n[!] bind udp port err\n");
		return 0;
	}

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
				//先做时间同步
//				if( ptp_server(sock_time, g_offset, g_delay) ){
//					printf("\n[!] Time Sync failed!\n\n");
//					break;
//				}
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

void *thread_work(void *pParam)
{
	SSL *ssl = (SSL *)pParam;
	char buf[1024];
	char buf_send[1024];
	int len;
	int conn = SSL_get_fd(ssl);
	int iRet = 0;
	void *handle = NULL;

	g_ssl = ssl;
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
		printf("\n*************************server recv command*************************\n");
		len = SSL_read(ssl, buf, 32);
		if (len <= 0) {
			break;
		}
		buf[len] = 0;
		printf("recv:%s\n", buf);

		if (!strncmp("quit", buf, 4)) {
			break;
		} else if (!strncmp("bzip2", buf, 5)) {
			handle = dlopen("libbzip2pp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libbzip2.so error:%s\n", dlerror());
				break;
			}
			iRet = check_bzip2(ssl, handle);
		} else if (!strncmp("bpe", buf, 3)) {
			handle = dlopen("libbpepp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libbpepp.so error:%s\n", dlerror());
				break;
			}
			iRet = check_bpe(ssl, handle);
		} else if (!strncmp("sort", buf, 4)) {
			handle = dlopen("libtestpp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libtestpp.so error:%s\n", dlerror());
				break;
			}
			iRet = check_sort(ssl, handle);
		} else if (!strncmp("bignum", buf, 6)) {
			handle = dlopen("libbnpp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libbnpp.so error:%s\n", dlerror());
				break;
			}
			iRet = check_bignum(ssl, handle);
		} else if (!strncmp("hmmer", buf, 5)) {
			handle = dlopen("libhmmerpp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libhmmerpp.so error:%s\n", dlerror());
				break;
			}
			iRet = check_hmmer(ssl, handle);
		} else if (!strncmp("sjeng", buf, 5)) {
			handle = dlopen("libsjengpp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libsjengpp.so error:%s\n", dlerror());
				break;
			}
			iRet = check_sjeng(ssl, handle);
		} else if (!strncmp("sjeng", buf, 5)) {
			handle = dlopen("libh264refpp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libh264refpp.so error:%s\n", dlerror());
				break;
			}
			iRet = check_h264ref(ssl, handle);
		} else if (!strncmp("gene", buf, 4)) {
			handle = dlopen("libgenepp.so", RTLD_LAZY);
			if (!handle) {
				fprintf(stderr, "load libgenepp.so error:%s\n", dlerror());
				break;
			}
			iRet = check_gene(ssl, handle);
		} else if (!strncmp("check ok", buf, 8)) {
			continue;
		} else {
			len = sprintf(buf_send, "Hello, %s", buf);
			if (SSL_write(ssl, buf_send, len + 1) <= 0) {
				break;
			}
		}
		if(handle)		dlclose(handle);
		if(iRet)		break;

	}//end of while (true)

    fprintf(stderr, "\n*************************disconnect!*************************\n");
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
