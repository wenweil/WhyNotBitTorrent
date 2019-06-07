#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

struct sock_struct {
	int mtu;
	int sock;
};


void *proc_request(void* arguments){
	puts("processing requests\n");
	struct sock_struct *args = arguments;
	int Mtu = args -> mtu;
	int sock = args -> sock;
	char msg[65535],response[1500];
	if(recv(sock,msg,65535,0) < 0){
		return NULL;	
	}
	int fsize;

	FILE *file;
	file = fopen(msg, "r");
	if(file == NULL){
		sprintf(response, "COULD NOT OPEN REQUESTED FILE\n");
		send(sock,response,strlen(response),0);
		puts("exiting handler\n");
		close(sock);
		return NULL;	
	}else {
		struct stat st;
		fstat(fileno(file), &st);
		fsize = st.st_size;
		sprintf(response,"FILE SIZE IS %d bytes",fsize);
		puts(response);
		send(sock,response,strlen(response),0);
	} 

	char buff[Mtu];
	memset(buff,0,sizeof(buff));
	char s;
	
	puts("starting file transmission\n");
	do{
		s = fgets(buff,Mtu,file);
		if(s != NULL &&buff[0] != EOF){
			int c = send(sock,buff,strlen(buff),0);
			if(c == -1 || c < strlen(buff)){
				puts("error transmitting file\n");
				break;
			}
		}
		sleep(1);//waits 1 second as per requirement
		memset(buff,0,sizeof(buff));
	}while(s != NULL);
	fclose(file);
	close(sock);
	puts("exiting handler\n");
	pthread_exit(NULL);
}

void *server(void* arguments){
	struct sock_struct *args = arguments;
	int Mtu = args -> mtu;
	int sock = args -> sock;
	struct sockaddr_in client;
	struct sock_struct sock_mtu;
	socklen_t socksize = sizeof(struct sockaddr_in);
	listen(sock , 5);	
	int new_socket;
	while( (new_socket = accept(sock, (struct sockaddr *)&client, &socksize)) ){
		puts("connection found, starting handler\n");
		pthread_t handler;
		sock_mtu.mtu = Mtu;
		sock_mtu.sock = new_socket;
		
		if( pthread_create( &handler , NULL ,  proc_request , (void*) &sock_mtu) < 0){
           		printf("could not create handler thread\n");
          	  	exit(-1);
		}
		
		
	}
	return NULL;
	
}

void serverInit(int ip4port,int ip6port,int MTU){
	int socketv4 = socket(AF_INET, SOCK_STREAM, 0);
	int socketv6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);	
	struct sockaddr_in server4;
	struct sockaddr_in6 server6;

	if(socketv6 == -1 && socketv4 == -1){
		printf("Error: cannot create socket\n");
		
		exit(-1);
	}
	
	server4.sin_family = AF_INET;
	server4.sin_addr.s_addr = htonl (INADDR_ANY);
	server4.sin_port = htons(ip4port);
	
	int flag = 1;
	int ret = setsockopt(socketv6, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(ret == -1) {
		perror("setsockopt()");
		exit(-1);
	}
	server6.sin6_family = AF_INET6;
	server6.sin6_addr= in6addr_any;
	server6.sin6_port = htons(ip6port);

	int stat = bind(socketv4, (struct sockaddr*) &server4 , sizeof(server4));
	int stat6 = bind(socketv6, (struct sockaddr*) &server6 , sizeof(server6));

	if(stat < 0 && stat6 <0){
		printf("Error: cannot bind socket to port\n");
		exit(-1);
	}

	struct sock_struct serv4;
	serv4.mtu = MTU;
	serv4.sock = socketv4;


	struct sock_struct serv6;
	serv6.mtu = MTU;
	serv6.sock = socketv6;

	pthread_t ip4serv,ip6serv;
	

	if(pthread_create(&ip6serv, NULL , server, (void*)&serv6) < 0){
		printf("Error: cannot create ipv6 server thread");
		close(socketv4);
		close(socketv6);
		exit(-1);
	}

	if(pthread_create(&ip4serv, NULL , server, (void*)&serv4) < 0){
		printf("Error: cannot create ipv4 server thread");
		close(socketv4);
		close(socketv6);
		exit(-1);
	}


	pthread_join(ip4serv,NULL);
	pthread_join(ip6serv,NULL);

	close(socketv4);
	close(socketv6);

}

int main (int argc , char** argv){
	if ( argc != 4){
		printf("Error: Wrong number of arguments\n");
		printf(" To use this server you need excatly 3 arguments:\n");
		printf("The port number for ipv4 (0-65535)\n");
		printf("The port number for ipv6 (0-65535)\n");
		printf("and the maximum transmission unit (MTU)\n");
		printf("there are no resticion set within this software for MTU\n");
		return -1;
	}
	serverInit(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]));
	return 0;
}

