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
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <errno.h>

#define BUFF_SIZE (1024)   /* for client read/write */
#define BACKLOG     (5)   /* maximum number of concurrent clients */
enum values {false, true};        /* 0 and 1, respectively */
typedef unsigned bool;     /* bool aliases unsigned int */
int sockfd, fileDescriptor, write_descriptor, read_descriptor;
struct in_addr address;
struct sockaddr_in serv_addr, client_addr;
socklen_t client_sock_size = sizeof(struct sockaddr_in);
int isDaemon = 0U;

int hasNewLine(char* string_stream, int size);
void error_msg(const char* msg, bool halt_flag);
void closeAll(void);
int create_server_socket(void);
void* handle_client(void*);
void announce_client(int client);
void write_timestamp(void);
static void setup_signal(int signum);
void join_threads(void);

struct client_entry {
    int client_fd;
    pthread_t threadID;
    STAILQ_ENTRY(client_entry) next;
};
//Queue variables
STAILQ_HEAD(client_queue_head, client_entry) client_queue = STAILQ_HEAD_INITIALIZER(client_queue);
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void join_threads(){
    struct client_entry *client_iter;
    STAILQ_FOREACH(client_iter,&client_queue,next) {
        int ret = pthread_join(client_iter->threadID,NULL);
        if (ret)error_msg("Not joinable",false);
    }
}

void sig_handler(int signum){
    if(signum == SIGINT){
        join_threads();
        syslog(LOG_DEBUG,"Caught SIGINT signal! \n");
        closeAll();
        exit (EXIT_SUCCESS);
    }
    if(signum == SIGTERM){
        join_threads();
        syslog(LOG_DEBUG,"Caught SIGTERM signal! \n");
        closeAll();
        exit (EXIT_SUCCESS);
    }
    if (signum == SIGALRM){
        write_timestamp();
    }
}

void fault_handler(){
    closeAll();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    if(argc > 1)
    {
        if(strcmp(argv[1],"-d") == 0)
        {
            isDaemon = 1U;
        }
    }

    openlog("aesd", LOG_CONS | LOG_ERR, LOG_USER );

    /* Create file where received data shall be stored */
    syslog(LOG_DEBUG, "Opening file.. ");
    fileDescriptor = creat("/var/tmp/aesdsocketdata.txt", 0644);
    if(fileDescriptor == -1){
        syslog(LOG_ERR, "ERROR: File could not be created");
        exit(EXIT_FAILURE);
    }

    sockfd = create_server_socket();
    close(fileDescriptor);
    write_descriptor = open("/var/tmp/aesdsocketdata.txt", O_WRONLY | O_APPEND);
    read_descriptor = open("/var/tmp/aesdsocketdata.txt", O_RDONLY);
    if (write_descriptor == -1 || read_descriptor == -1) error_msg("Descriptor error", true);
    setup_signal(SIGALRM);
    setup_signal(SIGINT);
    setup_signal(SIGTERM);

    struct itimerval itv;
    itv.it_value.tv_sec = 10;
    itv.it_value.tv_usec = 0;
    itv.it_interval.tv_sec = 10;
    itv.it_interval.tv_usec = 0;
    int startTimerOnce = 0;
    int thread_count = 0;
    while( true ) {
        int client = accept(sockfd, (struct sockaddr *) &client_addr, &client_sock_size);
        if(client < 0) error_msg("Problem accepting a client request", true);
        if (thread_count == 1 && startTimerOnce == 0) {
            if (setitimer(ITIMER_REAL,&itv,NULL) == -1)error_msg("Init Timer Error", true);
            startTimerOnce = 1;
        }
        if (thread_count >= BACKLOG) {
            join_threads();
            struct client_entry *client_iter;
            while ((client_iter = STAILQ_FIRST(&client_queue)) != NULL) {
                STAILQ_REMOVE_HEAD(&client_queue,next);
                free(client_iter);
            }
            thread_count=0;
            STAILQ_INIT(&client_queue);
        }
        announce_client(client);
        struct client_entry* new_client = malloc(sizeof(struct client_entry));
        pthread_mutex_lock(&queue_mutex);
        new_client->client_fd = client;
        STAILQ_INSERT_TAIL(&client_queue,new_client,next);
        pthread_mutex_unlock(&queue_mutex);
        pthread_create(&(new_client->threadID),NULL,handle_client,new_client);
        thread_count++;
    }
}

int hasNewLine(char* string_stream, int size){
    for(int letter = 0; letter< size ; letter++){
        if((*(string_stream+letter)) == '\n'){
            return 1;
        }
    }
    return 0;
}

void error_msg(const char* msg, bool halt_flag) {
    perror(msg);
    syslog(LOG_DEBUG,"%s",msg);
    if (halt_flag) fault_handler();
}

/* listening socket */
int create_server_socket(void) {
    /* Modify as needed. */
    const int port = 9000;

    /* create, bind, listen */
    sockfd = socket(AF_INET,     /* family */
              SOCK_STREAM, /* TCP */
              IPPROTO_TCP);
    if (sockfd < 0) error_msg("Problem with socket call", true);
    int y = 1;
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&y,sizeof(y)) == -1) error_msg("ERROR: socket options failed", true);

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port); /* host to network endian */

    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error_msg("Problem with bind call", true);

    if(isDaemon == 1U){
        int ret = daemon(0U,0U);
        if(ret != 0){ syslog(LOG_DEBUG, "Daemon creation failed"); exit(EXIT_FAILURE); }
        syslog(LOG_DEBUG, "Running as deamon");
    }else{
        syslog(LOG_DEBUG, "Not running as daemon");
    }

    if (listen(sockfd, 20) < 0)
        error_msg("Problem with listen call", true);

    return sockfd;
}

void* handle_client(void* argument) {
        struct client_entry* client = argument;
        int clnt_cn = client->client_fd;
        int packet_end = 0;
        char recv_buffer[BUFF_SIZE];
        char ret_buff[BUFF_SIZE];
        int char_count = 0U;
        int ret_bytes = 0U;
        ssize_t clnt_read_char = 0;
        pthread_t tid = pthread_self();
        syslog(LOG_DEBUG, "Th id %lu working with %i",(unsigned long)tid,clnt_cn);
        pthread_mutex_lock(&file_mutex);
        while ((clnt_read_char = read(clnt_cn, recv_buffer, BUFF_SIZE)) > 0) {
            packet_end = hasNewLine(recv_buffer, clnt_read_char);
            char_count += clnt_read_char;
            if ((write(write_descriptor, recv_buffer, clnt_read_char)) == -1)error_msg("Write error", true);
            if (packet_end) {
                if (char_count > BUFF_SIZE) {
                    while ((ret_bytes = read(read_descriptor, ret_buff, BUFF_SIZE)) > 0) {
                        if ((write(clnt_cn, ret_buff, ret_bytes)) == -1) error_msg("Write error", true);
                    }
                } else {
                    ret_bytes = pread(read_descriptor, ret_buff, BUFF_SIZE, 0);
                    if ((write(clnt_cn, ret_buff, ret_bytes)) == -1) error_msg("Write error", true);
                }
            }
        }
        pthread_mutex_unlock(&file_mutex);
        pthread_mutex_lock(&queue_mutex);
        close(clnt_cn);
        pthread_mutex_unlock(&queue_mutex);
        pthread_exit(NULL);
}

void write_timestamp(void) {
    time_t now;
    struct tm* timeinfo;
    char head[11U] = "timestamp: ";
    char buffer[26];
    char endline[1] = "\n";
    now = time(NULL);
    timeinfo = localtime(&now);
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %T %z", timeinfo);
    if (write(write_descriptor, head, sizeof(head)) <0)error_msg("Write error", true);
    if (write(write_descriptor, buffer, sizeof(buffer))<0) error_msg("Write error",true);
    if (write(write_descriptor, endline, 1U)<0) error_msg("Write error",true);
}

void announce_client(int client) {
    syslog(LOG_DEBUG,"Accepted connection from %s, client %i",inet_ntoa(client_addr.sin_addr),client);
}

void closeAll(void) {
    remove("/var/tmp/aesdsocketdata.txt");
    close(sockfd);
    close(write_descriptor);
    close(read_descriptor);
    closelog();
}

static void setup_signal(int signum) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(signum, &sa, NULL);
}