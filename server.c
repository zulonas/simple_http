#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 8081
#define MAX_CONN 1000
#define BUF_SIZE 65535

#define EEXIT(s) {\
        perror((s));\
        exit(1); }

typedef struct {
	  char *name, *value;
} header_t;

static header_t reqhdr[17] = {{"\0", "\0"}};

int server_open(uint16_t port)
{
	struct sockaddr_in srv_addr;
	int sfd;
	int enable = 1;

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		EEXIT("socket");

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Reuse last socket if possible*/
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		EEXIT("setsockopt");

	/* Bind socket to fd */
	if (bind(sfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1)
		EEXIT("bind");

	/* Set the server socket non-blocking */
	fcntl(sfd, F_SETFL, O_NONBLOCK);

	printf("Starting server at: 127.0.0.1:%d\n", port);

	/* Start listening */	
	if (listen(sfd, SOMAXCONN) == -1)
		EEXIT("listen");
	
	return sfd;
}

/* Get request header by name */
char *request_header(const char *name) {
	header_t *h = reqhdr;
	while (h->name) {
		if (strcmp(h->name, name) == 0)
			return h->value;
		h++;
	}
	return NULL;
}

void respond(int clientfd)
{
	int recfd;
	char *buf;

	char *method; // "GET" or "POST"
        char *uri;    // "/index.html" things before '?'
	char *qs;     // "a=1&b=2"     things after  '?'
	char *prot;   // "HTTP/1.1"

	/* WTF: resolve this code */
	header_t *h = reqhdr;
	char *t, *t2;
	char *k, *v;

	char *payload; // for POST
	int payload_size;

	buf = malloc(BUF_SIZE);
	memset(buf, 0, sizeof(buf));
	recfd = recv(clientfd, buf, BUF_SIZE, 0);

	if (recfd <= 0) {
		perror("recv");
	} else {
		buf[recfd] = '\0';

		method = strtok(buf, " \t\r\n");
		uri = strtok(NULL, " \t");
		prot = strtok(NULL, " \t\r\n");

		fprintf(stdout, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);

		/* Check if uri contains get params */
		qs = strchr(uri, '?');

		if (strchr(uri, '?')) {
			*qs++ = '\0'; /* split URI */
		} else {
			qs = uri - 1; /* use an empty string. Cut last symbol(?) */
		}

		/* have no clue how qf this code works */
		while (h < reqhdr + 16) {
			k = strtok(NULL, "\r\n: \t");
			if (!k)
				break;

			v = strtok(NULL, "\r\n");
			while (*v && *v == ' ')
				v++;

			h->name = k;
			h->value = v;
			h++;

			fprintf(stdout, "[H] %s: %s\n", k, v);
			t = v + 1 + strlen(v);

			if (t[1] == '\r' && t[2] == '\n')
				break;
		}
		t++; // now the *t shall be the beginning of user payload
		t2 = request_header("Content-Length"); // and the related header if there is
		payload = t;
		payload_size = t2 ? atol(t2) : (recfd - (t - buf));

		dup2(clientfd, STDOUT_FILENO);
		close(clientfd);

		// call router
		printf("HTTP/1.1 200 OK\r\n\r\n");
		printf("Hello! You are using %s", request_header("User-Agent"));

		// tidy up
		fflush(stdout);
		shutdown(STDOUT_FILENO, SHUT_WR);
		close(STDOUT_FILENO);
	}

	shutdown(clientfd, SHUT_WR);
	close(clientfd);

	free(buf);
}

void accept_connections(int sfd)
{
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	int slot = 0;
	int client;

	while (1) {
		client = accept(sfd, (struct sockaddr *) &client_addr, &len);
		if (client == -1) {
			perror("accept");
			continue;
		}
		printf("http request received");
	}
	/*while (1) {*/
		/*[>len = sizeof(client_addr);<]*/

		/*if (clients[slot] == -1) {*/
			/*perror("accept");*/
			/*exit(1);*/
		/*}*/

		/*if (fork() == 0) { [> child <]*/
			/*close(sfd);*/
			/*respond(clients[slot]);*/
			/*close(clients[slot]);*/
			/*exit(0);*/
		/*} else { [> parent <]*/
			/*close(clients[slot]);*/
		/*}*/

		/*while (clients[slot] != -1)*/
			/*slot = (slot + 1) % MAX_CONN;*/
	/*}*/
}


int main()
{
	int sfd;

	sfd = server_open(PORT);

	/* Ignore SIGCHILD to avoid zombie threads */
	/* TODO: change me, when threads */
	/*signal(SIGCHLD, SIG_IGN);*/
	accept_connections(sfd);

	close(sfd);
	return 0;
}
