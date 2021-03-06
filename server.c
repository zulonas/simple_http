#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT "8080"
#define MAX_CONN 1000
#define BUF_SIZE 65535

typedef struct {
	  char *name, *value;
} header_t;

static header_t reqhdr[17] = {{"\0", "\0"}};

int server_open()
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd;
	int enable = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	 /* IPv4 */
	hints.ai_socktype = SOCK_STREAM; /* TCP */
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;		 /* Any protocol */

	if (getaddrinfo(NULL, PORT, &hints, &result) != 0) {
		perror("getaddrinfo");
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (!bind(sfd, rp->ai_addr, rp->ai_addrlen))
			break;	/* Success */

		close(sfd);
	}

	/* Reuse last socket address */
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt");

	#warning remove me
	printf("Starting server at: 127.0.0.1:%s\n", PORT);

	freeaddrinfo(result);
	if (!rp) {
		perror("bind");
		return -1;
	}

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

void accept_connections(int listen_fd)
{
	struct sockaddr_in client_addr;
	socklen_t len;
	int clients[MAX_CONN];
	int slot = 0;

	memset(&clients, -1, sizeof(clients));

	while (1) {
		len = sizeof(client_addr);
		clients[slot] = accept(listen_fd, (struct sockaddr *) &client_addr, &len);

		if (clients[slot] == -1) {
			perror("accept");
			exit(1);
		}

		if (fork() == 0) { /* child */
			close(listen_fd);
			respond(clients[slot]);
			close(clients[slot]);
			exit(0);
		} else { /* parent */
			close(clients[slot]);
		}

		while (clients[slot] != -1)
			slot = (slot + 1) % MAX_CONN;
	}
}


int main()
{
	int sfd;

	sfd = server_open();
	if (sfd == -1)
		exit(1);

	if (listen(sfd, SOMAXCONN) == -1)
	{
		perror("listen");
		exit(1);
	}

	/* Ignore SIGCHILD to avoid zombie threads */
	/* TODO: change me, when threads */
	signal(SIGCHLD, SIG_IGN);
	accept_connections(sfd);

	close(sfd);
	return 0;
}
