#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define BUF_SIZE 1024
int sockfd, clnt_cn, fileDescriptor, pid, write_descriptor, read_descriptor;
int char_count = 0U;
int ret_bytes = 0U;
int packet_end = 0;
struct in_addr address;
struct sockaddr_in serv_addr, client_addr;
socklen_t client_sock_size = sizeof(client_addr);
ssize_t clnt_read_char = 0;
ssize_t clnt_ret_char = 0;
char* recv_buffer = NULL;
char* ret_buff = NULL;
int isDeamon = 0U;
int hasNewLine(char* string_stream, int size);
void sig_handler(int signo){
    if(signo == SIGINT){
        syslog(LOG_DEBUG,"Caught SIGINT signal! \n");
    }
    else if(signo == SIGTERM){
        syslog(LOG_DEBUG,"Caught SIGTERM signal! \n");
    }
    else{
        syslog(LOG_DEBUG,"Unexpected signal \n");
    }
    remove("/var/tmp/aesdsocketdata.txt");
    free(recv_buffer);
    free(ret_buff);
    close(sockfd);
    close(write_descriptor);
    close(read_descriptor);
    close(clnt_cn);
    closelog();
    exit (EXIT_SUCCESS);
}

void fault_handler(){
    free(recv_buffer);
    free(ret_buff);
    close(write_descriptor);
    close(read_descriptor);
    close(sockfd);
    close(clnt_cn);
    closelog();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{

    if(argc > 1)
    {
        if(strcmp(argv[1],"-d") == 0)
        {
            isDeamon = 1U;
        }
    }

    openlog("aesd", LOG_CONS | LOG_ERR, LOG_USER );
    
    if(signal(SIGINT,sig_handler) == SIG_ERR){
        syslog(LOG_DEBUG, "Cannot handle SIGINT signal \n");
    }
    if(signal(SIGTERM,sig_handler) == SIG_ERR){
        syslog(LOG_DEBUG, "Cannot handle SIGTERM signal \n");
    }

    /* Create file where received data shall be stored */
    syslog(LOG_DEBUG, "Opening file.. ");
    fileDescriptor = creat("/var/tmp/aesdsocketdata.txt", 0644);
    if(fileDescriptor == -1){
        syslog(LOG_ERR, "ERROR: File could not be created");
        exit(EXIT_FAILURE);
    }

    /* Create socket*/
    sockfd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    if(sockfd == -1)
    {
        syslog(LOG_ERR, "Socket not created ");
        fault_handler();
    }

    int yes = 1;
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) == -1){
        syslog(LOG_ERR, "ERROR: socket options failed");
        fault_handler();
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(9000);

    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1 ){
        syslog(LOG_DEBUG, "Binding unsuccessful");
        fault_handler();
    }
    
    if(isDeamon == 1U){
        int ret = daemon(0U,0U);
        if(ret != 0){ syslog(LOG_DEBUG, "Daemon creation failed"); exit(EXIT_FAILURE); }
        syslog(LOG_DEBUG, "Running as deamon");
    }else{
        syslog(LOG_DEBUG, "Not running as daemon");
    }

    if(listen(sockfd, 20U) == -1){
        syslog(LOG_DEBUG, "Listening unsuccessful");
        fault_handler();
    }

    recv_buffer   = (char*) malloc(BUF_SIZE);
    ret_buff      = (char*) malloc(BUF_SIZE);

    if( (recv_buffer == NULL) | (ret_buff == NULL)){
        syslog(LOG_DEBUG, "Not enough heap memory");
        fault_handler();
    }

    close(fileDescriptor);
    int write_descriptor = open("/var/tmp/aesdsocketdata.txt", O_WRONLY | O_APPEND);
    int read_descriptor = open("/var/tmp/aesdsocketdata.txt", O_RDONLY);

    while( 1 ) {
        clnt_cn = accept(sockfd, (struct sockaddr *) &client_addr, &client_sock_size);
        if(clnt_cn > 0){
            syslog(LOG_DEBUG,"Accepted connection from %s",inet_ntoa(client_addr.sin_addr));
            while((clnt_read_char = read(clnt_cn, recv_buffer, BUF_SIZE)) > 0 ){
                packet_end = hasNewLine(recv_buffer,clnt_read_char);
                char_count += clnt_read_char;
                if( (write(write_descriptor,recv_buffer,clnt_read_char)) == -1 ){
                    syslog(LOG_DEBUG, "Write error");
                    fault_handler();
                }
                if(packet_end){
                    if(char_count > BUF_SIZE){
                        while((ret_bytes = read(read_descriptor, ret_buff, BUF_SIZE)) > 0 ){
                            if((write(clnt_cn, ret_buff, ret_bytes)) == -1){
                                syslog(LOG_DEBUG, "Write error");
                                fault_handler();
                            }
                        }
                    }else{
                        int ret_bytes = pread(read_descriptor, ret_buff, BUF_SIZE,0);
                        if((write(clnt_cn, ret_buff, ret_bytes)) == -1){
                        syslog(LOG_DEBUG, "Write error");
                        fault_handler();
                        }
                    }
                }
            }
            close(clnt_cn);
            syslog(LOG_DEBUG,"Closed connection from %s",inet_ntoa(client_addr.sin_addr));
        }
        else{
            syslog(LOG_DEBUG, "Connection not accepted");
            close(clnt_cn);
            break;
        }
    }
    return 0;
}

int hasNewLine(char* string_stream, int size){
    for(int letter = 0; letter< size ; letter++){
        if((*(string_stream+letter)) == '\n'){
            return 1;
        }
    }
    return 0;
}