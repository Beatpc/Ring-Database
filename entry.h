#ifndef ENTRY_H
#define ENTRY_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

//Estrutura que armazena dos dados importantes sobre o nó de cada terminal----------------------------------------------------------------------------------------------
typedef struct server_node 
{
    int key;
    char ip[16];
    char port[16];
    struct sockaddr *addr;
    socklen_t addrlen;
    int welcoming_socket; //file descriptor da socket TCP welcoming
    //---------------------------------------------------------------------------
    int key_pred;
    char ip_pred[16];
    char port_pred[16];
    int tcp_pred_socket;
    //---------------------------------------------------------------------------
    int key_suc;
    char ip_suc[16];
    char port_suc[16];
    int tcp_suc_socket;
    //----------------------------------------------------------------------------
    int udp_socket; // servidor e cliente de conexões UDP
    //----------------------------------------------------------------------------
    int key_chord;
    char ip_chord[16];
    char port_chord[16];

} server_node;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//Verificações iniciais do programa-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void verify_arguments(int argc, char *argv[], int *key, char ip[], char port[]);
int distance(int b, int a);
int verify_key(int key);
int verify_port(char port[]);

//Iinterface do utilizador---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void print_user_interface();

//Cálculo do file descriptor máximo para o select---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int find_max_fd(int stdin, int welcoming_socket, int suc_socket, int pred_socket, int udp_socket);

//Criação dos nós e inicialização dos seus valores de sucessor, predecessor e atalho----------------------------------------------------------------------------------------------------------------------------------------------
void create_node(server_node *myself, int KEY, char IP[], char PORT[]);
void put_pred_in_node(server_node *myself, int KEY_PRED, char IP_PRED[], char PORT_PRED[]);
void put_suc_in_node(server_node *myself, int KEY_SUC, char IP_SUC[], char PORT_SUC[]);
void put_chord_in_node(server_node *myself, int KEY_CHORD, char IP_CHORD[], char PORT_CHORD[]);

//Funções envolvidas no processo de leitura, escrita e processamento de mensagens
int client_TCP(char IP[], char PORT[]);
struct addrinfo *client_UDP(char IP[], char PORT[]);
int read_message(int fd, server_node *myself, struct addrinfo *res_udp, int sequence_number_find_table[], int sequence_number_bentry_table[], char ip_bentry[], char port_bentry[]);
int read_message_udp(int fd, server_node *myself, struct addrinfo *res_udp, int sequence_number_find_table[], int sequence_number_bentry_table[], char ip_bentry[], char port_bentry[], int sequence_number_bentry);
void write_in_socket(int new_socket, char buffer[]);
void write_in_udp(int new_socket, char buffer[], struct addrinfo *res);

//Função que faz parte da funcionalidade que não conseguimos implementar corretamente -> INVPENTRY
int verify_pred_conditions(int myself_key, int suc, int entry_key);

//Verifica se quando enviamos uma mensagem via UDP recebemos um ACK de volta
int waiting_ack(server_node *myself, char buffer[], char ip[], char port[]);

#endif