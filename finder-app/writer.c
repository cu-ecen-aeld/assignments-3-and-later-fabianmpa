#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    openlog("aesd", LOG_CONS | LOG_ERR, LOG_USER );

    if(argc != 3){
        syslog(LOG_ERR, "Not enough arguments");
        closelog();
        return 1;
    }

    int fileDescriptor;
    ssize_t cnt;

    syslog(LOG_DEBUG, "Opening file.. ");
    fileDescriptor = creat(argv[1], 0644);
    if(fileDescriptor == -1){
        syslog(LOG_ERR, "ERROR: File could not be created");
        closelog();
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s ...", argv[2], argv[1]);

    cnt = write(fileDescriptor, argv[2], strlen(argv[2]));
    if(cnt == -1){
        syslog(LOG_ERR, "ERROR: File could not be written");
        closelog();
        return 1;
    }else{
        syslog(LOG_DEBUG, "Write successful ");
    }
    
    if(close(fileDescriptor) == -1){
        syslog(LOG_ERR, "ERROR: File could not be closed");
        closelog();
        return 1;
    }else{
        syslog(LOG_DEBUG, "Close file successful");
        closelog();
    }

    return 0;
}