
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdlib.h>
#include <unistd.h>


#define MAX_ID 400
static mqd_t port_map[MAX_ID];

// Ensure all message queues are removed before we start so that we don't get any stale messages.
void message_init(void)
{
	int i;
	for(i=0; i < MAX_ID; i++){
		char *name;
		if(asprintf(&name, "/liftqueue-%u-%u", getuid(), i) == -1){
			fprintf(stderr, "Could not format port name\n");
			exit(1);
		}
		mq_unlink(name);
		free(name);
	}
}

static mqd_t open_or_create_port(unsigned int id)
{
	char *name;
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = 1024;
	attr.mq_curmsgs = 0;
	// Include the user id in the name so we are guaranteed to be able to
	// open the port again and avoid a situation where a port is opened by
	// another student and not deleted before logging off.
	if(asprintf(&name, "/liftqueue-%u-%u", getuid(), id) == -1){
		fprintf(stderr,"Could not format port name\n");
		exit(1);
	}
	mqd_t port = mq_open(name, O_RDWR | O_CREAT, 0600, &attr);
	free(name);
	if(port == -1){
		perror("mq_open");
		exit(1);
	}
	return port;
}

static void check_port(unsigned int portid)
{
	if(portid >= MAX_ID){
		fprintf(stderr,"Fatal Internal error: portid %u too large\n", portid);
		exit(1);
	}
	if(port_map[portid] == 0){
		port_map[portid] = open_or_create_port(portid);
	}
}

void message_send(char *msg, unsigned int len, unsigned int portid, unsigned int priority)
{
	check_port(portid);
	mq_send(port_map[portid], msg, len, priority);
}


ssize_t message_receive(char *msg, unsigned int max_len, unsigned int portid)
{
	check_port(portid);
	ssize_t length = mq_receive(port_map[portid], msg, max_len, NULL);
	if(length == -1){
		perror("mq_receive");
		fprintf(stderr,"    pid for mq_receive: %d, max_len is %d, portid is %d\n", getpid(), max_len, portid);
	}
	return length;
}
