#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <netdb.h>
#include <unistd.h>

#define IPV4_PORT 33455 //ipv4 default value
#define IPV6_PORT 33446 //ipv6 default value
//#define LOCAL_HOST "localhost"
//#define IPV4_MTU 1440
//#define IPV6_MTU 1280
//#define DEFAULT_FILENAME "fileToTransfer"

//argv[0]: program name
//argv[1]: server IP address
//argv[2]: server server port number
//argv[3]: the client IP address
//argv[4]: the filename of the requested file
//argv[5]: the size of the send/receive buffer



int validate_address(char* ip);
void test_address_validation();

int main(int argc, char *argv[]){
	
	char localhost[]="localhost";
	struct sockaddr_in server;
	struct hostent *ptr;
	char *host_addr = NULL;
	int port = 0;
	char *client_addr;
	int buf_size = 0;
	int ipv4_mtu=1440;
	int ipv6_mtu=1280;
	char* requested_file;
	int sock_desc;
	char* message =NULL;	
	//char buf[1024];
	char *recv_buf=NULL;
	int nread=0;
	FILE* outfile = NULL;
	int total_read = 0;
	int nwrite = 0;
	int file_size = 0;
	
	   /* Check for command-line arguments                                  */
   /* If there are not arguments print an information message           */
	if (argc <= 1) {   
		fprintf (stderr, "Following Command line arguments are required:\n");
		
		fprintf (stderr, "Server IP address:\n");
		fprintf (stderr, "	Default value localhost\n");
		fprintf (stderr, "Server port number:\n");
		fprintf (stderr, "	  Ipv4 default value 33455\n");
		fprintf (stderr, "	  Ipv6 default value 33446\n");
		fprintf (stderr, "IP address of local communication endpoint\n");
		fprintf (stderr, "	  Default value localhost\n");
		fprintf (stderr, "The filename of the requested file\n");
		fprintf (stderr, "	  Default value fileToTransfer\n");
		fprintf (stderr, "buffer size:\n");
		fprintf (stderr, "        Ipv4 default value 1440\n");
		fprintf (stderr, "        Ipv4 default value 1280\n");
		fprintf (stderr, "To accept any particular default replace\n");
		fprintf (stderr, "the variable with a . in the argument list\n");
		exit(0);
	}


	if(argc!=6){
		fprintf(stderr,"Invalid number of arguments\n");
		exit(0);
	}
	
	//get host address
	if(strncmp(argv[1],".",1)==0){
		ptr = gethostbyname(localhost);
		host_addr = inet_ntoa(*((struct in_addr*)ptr->h_addr_list[0]));
	}else{
		host_addr = argv[1];
	}
	//memcpy(&server.sin_addr, host->h_addr, host->h_length);

	
	//check if host address is ipv4 or ipv6
	//default port and mtu depends on which ip address is used
	if(validate_address(host_addr)==4){
		if(strncmp(argv[2],".",1)==0){
			port = IPV4_PORT;
		}else{
			port = atoi(argv[2]);
		}
		if(strncmp(argv[5],".",1)==0){
			buf_size = ipv4_mtu;
		}else{
			buf_size = atoi(argv[5]);
		}
		
	}else if(validate_address(host_addr)==6){
		if(strncmp(argv[2],".",1)==0){
			port = IPV6_PORT;
		}else{
			port = atoi(argv[2]);
		}
		if(strncmp(argv[5],".",1)==0){
			buf_size = ipv6_mtu;
		}else{
			buf_size = atoi(argv[5]);
		}
	}else{
		fprintf(stderr,"Invalid address\n");
		exit(0);
	}	
	
	//extract client address
	if(strncmp(argv[3],".",1) == 0){
		ptr = gethostbyname(localhost);
		client_addr = inet_ntoa(*((struct in_addr*)ptr->h_addr_list[0]));
		//client_addr = localhost;
	}else{
		client_addr = argv[3];
	}
	//validate address
	if((validate_address(client_addr)==0)){
		fprintf(stderr,"Invalid client address:%s\n",client_addr);
		exit(0);
	}

	if(strncmp(argv[4],".",1) == 0){
		requested_file = "fileToTransfer";
	}else{
		requested_file = argv[4];
	}
	


	printf("host address: %s\n",host_addr);
	printf("port: %d\n",port);
	printf("client address:%s\n",client_addr);
	printf("requested file:%s\n",requested_file);
	printf("buffer size:%d \n",buf_size);
	//allocate memory for recieve buffer
	recv_buf = malloc(buf_size*sizeof(*recv_buf));

	if(validate_address(host_addr)==4){
		//server address is ipv4
		printf("address is ipv4");
		server.sin_addr.s_addr = inet_addr(host_addr);
		server.sin_port = htons(port);
		server.sin_family = AF_INET;
		//create IPV4 socket
		sock_desc = socket(AF_INET, SOCK_STREAM, 0);
		if(sock_desc == -1){
			printf("Could not open IPV 4 socket\n");
		}
	}else if(validate_address(host_addr)==6){
		//server address is ipv6
		printf("address is ipv6");
		server.sin_addr.s_addr = inet_addr(host_addr);
		server.sin_port = htons(port);
		server.sin_family = AF_INET6;
		//create IPV6 socket
		sock_desc = socket(AF_INET6,SOCK_STREAM,0);
		if(sock_desc == -1){
			printf("Could not open IPV 6 socket\n");
		}
	}else{

		printf("Invaid address\n");
		exit(-1);
	}

	//connect to remote server	
	if(connect(sock_desc, (struct sockaddr *)&server, sizeof(server))<0){
		printf("connection failed");
		return -1;
	}
	message = requested_file;
	//request file
	//Check if the requested file exits on the server
	if(send(sock_desc,message,strlen(message), 0) < 0){
		printf("send fail\n");
		return -1;
	}else{
		if(recv(sock_desc,recv_buf,buf_size,0 ) < 0){
			fprintf(stderr,"error reading from the socket\n");
			exit(-1);
		}
		
		if(strcmp(recv_buf,"COULD NOT OPEN REQUESTED FILE\n")){
			fprintf(stderr,"COULD NOT OPEN REQUESTED FILE\n");
			close(sock_desc);
			free(recv_buf);
			exit(0);
		}else{
			//extract file size
			sscanf(recv_buf,"%d",&file_size);
			printf("file size:%d\n",file_size);			

			if((outfile = fopen(requested_file,"r"))==NULL){
				fprintf(stderr,"COULD NOT OPEN OUTPUT FILE\n");
				close(sock_desc);
				free(recv_buf)
				exit(0);
			}
		}	
	}

	while(1){
		if( ( nread = recv(sock_desc,recv_buf,buf_size,0 ) < 0) ){
			fprintf(stderr,"error reading from the socket");
			close(sock_desc);
			close(fileno(outfile));
			free(recv_buf);
			exit(-1);
		}else if(nread==0){
			
			printf("All data recieved: %d bytes\n",total_read);
			printf("data write to file: %d bytes\n",total_read);
			break;
		}else{
			
			total_read += nread;
			nwrite += write(fileno(outfile), recv_buf, nread);
			memset(recv_buf,"0",buf_size*sizeof(recv_buf));
		}	
	}
	close(sock_desc);
	close(fileno(outfile));
}


int validate_address(char* ip){
	unsigned char buf[sizeof(struct in6_addr)];
	if(inet_pton(AF_INET,ip,buf)){
		//printf("Ipv4:%s\n",buf);
		return 4;
	}else if(inet_pton(AF_INET6,ip,buf)){
		//printf("Ipv6:%s\n",buf);
		return 6;	
	}else{
		//printf("Invalid address\n");
		return 0;
	}
}

void test_address_validation(){
	char ip1[] = "2001:cdba:0000:0000:0000:0000:3257:9652";
    	char ip2[] = "2001:cdba:0:0:0:0:3257:9652";
    	char ip3[] = "2001:cdba::3257:9652";
    	char ip4[] = "2001:cdba::3257:965212323";
	validate_address(ip1);
	validate_address(ip2);
	validate_address(ip3);
	validate_address(ip4);
}




















