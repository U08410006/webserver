#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>

#define MAX_LINE 1024
#define MAX_USERNAME 25

void print_board();
void *server_connect(int socket_fd, struct sockaddr_in *address);
void *receive_data(void *data_ptr);
void send_mesg(char *cmd_line, int socket_fd, struct sockaddr_in *address, char *user_name);

char usage[] = "\Command List:\n"
               "/q, /quit: quit\n"
	           "/list : list all user\n"
	           "/chat : chat to specific user\n"
	           "/chess : play tic tac toe to specific user\n";
typedef struct thread_data thread_data;
struct thread_data
{
    int socket_fd;
    char cmd_line[MAX_USERNAME + 1];
};

int board[9];

int main(int argc, char **argv)
{
    long port = strtol(argv[2], NULL, 10);
    struct sockaddr_in address, client_addr;
    char *server_addr;
    char user_name[MAX_USERNAME];
    char password[MAX_USERNAME];
    char cmd_line[MAX_USERNAME + 1];

    if(argc < 3)
    {
        printf("usage: .\\server.exe <IP> <PORT>\n");
        exit(0);
    }

    printf("Please enter your Username : ");
    scanf("%s", user_name);
    printf("Please enter your password : ");
    scanf("%s", password);
    strcpy(cmd_line, user_name);
    strcat(cmd_line, ">");

	bzero(&address, sizeof(address));
    	server_addr = argv[1];
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_addr);
	address.sin_port = htons(port);
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	server_connect(socket_fd, &address);

    thread_data data;
    strcpy(data.cmd_line, cmd_line);
    data.socket_fd = socket_fd;


    pthread_t thread;
	pthread_create(&thread, NULL, (void *)receive_data, (void *)&data);

    send_mesg(cmd_line, socket_fd, &address, user_name);

    printf("closed client\n");
    close(socket_fd);
    pthread_exit(NULL);
    
    exit(0);
}

void print_board()
{
	char char_board[9];

	for (int i = 0; i < 9; i++)
	{
		if (board[i] == -1) char_board[i] = ' ';
		else if (board[i] == 0) char_board[i] = 'O';
		else char_board[i] = 'X';
	}

	printf("\n");
    printf("O:Your chess, X: opponent's chess\n");
	printf("┌───┬───┬───┐        ┌───┬───┬───┐\n");
	printf("│ 0 │ 1 │ 2 │        │ %c │ %c │ %c │\n", char_board[0], char_board[1], char_board[2]);
	printf("├───┼───┼───┤        ├───┼───┼───┤\n");
	printf("│ 3 │ 4 │ 5 │        │ %c │ %c │ %c │\n", char_board[3], char_board[4], char_board[5]);
	printf("├───┼───┼───┤        ├───┼───┼───┤\n");
	printf("│ 6 │ 7 │ 8 │        │ %c │ %c │ %c │\n", char_board[6], char_board[7], char_board[8]);
	printf("└───┴───┴───┘        └───┴───┴───┘\n\n");
}
void *server_connect(int socket_fd, struct sockaddr_in *address)
{
	if(connect(socket_fd, (struct sockaddr *)address, sizeof *address) < 0) 
    {
        printf("connect failed");
        exit(0);
    }
    else printf("Successful connecting\n");
}

void *receive_data(void *data_ptr)
{
    thread_data *data = ((thread_data *)data_ptr);
    int in_game = 0;
    int board_init = 1;
    int socket_fd = data->socket_fd;
    char server_mesg[MAX_LINE];
    char *cmd_line = data->cmd_line;

    while(1)
    {

        memset(server_mesg, 0, sizeof(server_mesg));
        if(board_init) for(int i = 0;i < 9;i++) board[i] = -1;

        int response = recv(socket_fd, server_mesg, MAX_LINE, 0);
        if(response < 0)
        {
            printf("\nrecv failed\n");
            break;
        }
        else if(response == 0)
        {
            printf("\ndisconnected\n");
            break;
        }
        else
        {
            server_mesg[response] = 0;
            if(!in_game)
            {
                printf("%s", server_mesg);
                if(strncmp("<Game>", server_mesg, 6) == 0)
                {
                    in_game = 1;
                    board_init = 0;
                    printf("%s", cmd_line);
                }
                else if (strncmp(">>", server_mesg, 2) != 0)
				{ 
                    //private message
					printf("%s", cmd_line);
				}
            }
            else
            {
                if(strncmp("<", server_mesg, 1) == 0)
                {
                    printf("%s", server_mesg);
                    print_board();
                    if(strncmp("<Turn>", server_mesg, 6) == 0)
                        printf("Please input a integer between 0 ~ 8\n>");
                }
                else 
					sscanf(server_mesg, "%d %d %d %d %d %d %d %d %d", &board[0], &board[1], &board[2], &board[3], &board[4], &board[5], &board[6], &board[7], &board[8]);
                
                if(strncmp("<Game>", server_mesg, 6) == 0)
                {
                    in_game = 0;
                    board_init = 1;
                }
            }   
            fflush(stdout);
        }   
    }
}
void send_mesg(char *cmd_line, int socket_fd, struct sockaddr_in *address, char *user_name)
{
	char client_mesg[MAX_LINE];
	char buff[MAX_LINE];
	memset(client_mesg, '\0', sizeof(client_mesg));
	memset(buff, '\0', sizeof(buff));

	send(socket_fd, user_name, strlen(user_name), 0);

	while (fgets(client_mesg, MAX_LINE, stdin) != NULL)
	{
		printf("%s", cmd_line);
		if ((strncmp(client_mesg, "/quit", 5) == 0) || (strncmp(client_mesg, "/q", 2) == 0))
		{
			printf("Close connection...\n");
			exit(0);
		}
		else if (strncmp(client_mesg, "/help", 5) == 0)
		{
			printf("%s", usage);
			printf("%s", cmd_line);
			memset(client_mesg, '\0', sizeof(client_mesg));
			continue;
		}
		send(socket_fd, client_mesg, strlen(client_mesg), 0);
		memset(client_mesg, '\0', sizeof(client_mesg));
	}
}
