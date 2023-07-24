#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

struct Route {
	char* key;
	char* value;

	struct Route *left, *right;
};

typedef struct HTTP_Server {
	int socket;
	int port;	
} HTTP_Server;

struct Route * initRoute(char* key, char* value) {
	struct Route * temp = (struct Route *) malloc(sizeof(struct Route));

	temp->key = key;
	temp->value = value;

	temp->left = temp->right = NULL;
	return temp;
}

void inorder(struct Route* root)
{

    if (root != NULL) {
        inorder(root->left);
        printf("%s -> %s \n", root->key, root->value);
        inorder(root->right);
    }
}

void addRoute(struct Route ** root, char* key, char* value) {
	if (*root == NULL) {
		*root = initRoute(key, value);
	}
	else if (strcmp(key, (*root)->key) == 0) {
		printf("============ WARNING ============\n");
		printf("A Route For \"%s\" Already Exists\n", key);
	}else if (strcmp(key, (*root)->key) > 0) {
		addRoute(&(*root)->right, key, value);
	}else {
		addRoute(&(*root)->left, key, value);
	}
}

struct Route * search(struct Route * root, char* key) {
	if (root == NULL) {
		return NULL;
	} 

	if (strcmp(key, root->key) == 0){
		return root;
	}else if (strcmp(key, root->key) > 0) {
		return search(root->right, key);
	}else if (strcmp(key, root->key) < 0) {
		return search(root->left, key);
	}  

}


void init_server(HTTP_Server * http_server, int port) {
	http_server->port = port;

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;

	bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));

	listen(server_socket, 5);

	http_server->socket = server_socket;
	printf("HTTP Server Initialized\nPort: %d\n", http_server->port);
}

char * render_static_file(char * fileName) {
	FILE* file = fopen(fileName, "rb");

	if (file == NULL) {
		return NULL;
	}else {
		printf("%s is conjured \n", fileName);
	}

	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* temp = malloc(sizeof(char) * (fsize+1));
	char ch;
	int i = 0;
	while((ch = fgetc(file)) != EOF) {
		temp[i] = ch;
		i++;
	}
	fclose(file);
	return temp;
}



int main() {
	// initiate HTTP_Server
	int port = 8080;
	HTTP_Server http_server;
	init_server(&http_server, port);

	int client_socket;
	
	// registering Routes
	struct Route * route = initRoute("/", "home.html"); 
	addRoute(&route, "/about", "about.html");
	addRoute(&route, "/install", "install.html");

	// display all available routes
	printf("Visit localhost:%d to check out website\n",port);
	inorder(route);

	while (1) {
		char client_msg[4096] = "";

		client_socket = accept(http_server.socket, NULL, NULL);

		read(client_socket, client_msg, 4095);
		printf("%s\n", client_msg);

		// parsing client socket header to get HTTP method, route
		char *method = "";
		char *urlRoute = "";

		char *client_http_header = strtok(client_msg, "\n");
			
		printf("\n\n%s\n\n", client_http_header);

		char *header_token = strtok(client_http_header, " ");
		
		int header_parse_counter = 0;

		while (header_token != NULL) {

			switch (header_parse_counter) {
				case 0:
					method = header_token;
				case 1:
					urlRoute = header_token;
			}
			header_token = strtok(NULL, " ");
			header_parse_counter++;
		}

		printf("The method is %s\n", method);
		printf("The route is %s\n", urlRoute);


		char template[100] = "";
		
		if (strstr(urlRoute, "/style/") != NULL) {
			strcat(template, "style/index.css");
		} else if (strstr(urlRoute, "/imgs/") != NULL) {
			strcat(template, "imgs/image.png");
		} else {
			struct Route * destination = search(route, urlRoute);
			strcat(template, "templates/");

			if (destination == NULL) {
				strcat(template, "404.html");
			}else {
				strcat(template, destination->value);
			}
		}

		char * response_data = render_static_file(template);

		char http_header[4096] = "HTTP/1.1 200 OK\r\n\r\n";

		strcat(http_header, response_data);
		strcat(http_header, "\r\n\r\n");


		send(client_socket, http_header, sizeof(http_header), 0);
		close(client_socket);
		free(response_data);
	}
	return 0;
}