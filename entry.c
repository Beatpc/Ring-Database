#include "entry.h"
//
#define max(A, B) ((A) >= (B) ? (A) : (B))

// Verifica se quando enviamos uma mensagem via UDP recebemos um ACK de volta, caso contrário retorna -1----------------------------------------------------------------------------
int waiting_ack(server_node *myself, char buffer[], char ip[], char port[])
{
    int i = -1;
    struct sockaddr addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    struct addrinfo *res_udp;

    struct timeval timeout;
    timeout.tv_sec = 3;  // segundos
    timeout.tv_usec = 0; // microsegundos
    int counter = 0;     // Vê quantas vezes já reenviámos a mensagem

    char buffer_ACK[128];
    memset(&buffer_ACK, '\0', 128);

    fd_set socket_set_ACK;

    while (counter < 3)
    {
        FD_ZERO(&socket_set_ACK);

        FD_SET(myself->udp_socket, &socket_set_ACK);

        if (select(myself->udp_socket + 1, &socket_set_ACK, NULL, NULL, &timeout) == 0) // Se ao fim de 3 segundos não receber nada entra no if
        {
            res_udp = client_UDP(ip, port); // Vamos voltar a enviar a mensagem pois ainda não recebemos o ACK
            write_in_udp(myself->udp_socket, buffer, res_udp);
            if (counter == 2)
            {
                return i;
            }
            counter++;
        }
        else
        {
            if ((recvfrom(myself->udp_socket, buffer_ACK, 128, 0, &addr, &addrlen) < 1))
            {
                printf("ERRO READING MESSAGE FROM SOCKET UDP\n");
                perror("error");
                exit(1);
            }

            if (strcmp(buffer_ACK, "ACK") == 0)
            {
                break;
            }
        }

        FD_CLR(myself->udp_socket, &socket_set_ACK);
    }

    return 0;
}

// verifica se os argumentos inseridos na linha de comandos estão todos corretos
void verify_arguments(int argc, char *argv[], int *key, char ip[], char port[])
{
    if (argc != 4)
    {
        printf("Bad program call\n");
        exit(1);
    }

    // Assim que a aplicação é invocada recolhemos os dados do nó criado
    if (sscanf(argv[1], "%d", key) <= 0) // leitura incorreta
        exit(1);
    if (sscanf(argv[2], "%s", ip) <= 0)
        exit(1);
    if (sscanf(argv[3], "%s", port) <= 0)
        exit(1);

    verify_key(*key);
    verify_port(port);
}

// Cálcula a distância entre 2 nós no anel------------------------------------------------------------------------------------------------------------------------------------------
int distance(int b, int a)
{
    int i = 0;
    i = (a - b) - (32 * floor((a - b) / (float)32));
    return i;
}

// Verifica se o valor da chave (key) é válido--------------------------------------------------------------------------------------------------------------------------------------
int verify_key(int key)
{
    if (key < 0 || key > 31)
    {
        printf("Invalid key value\n");
        exit(1);
    }
    return 0;
}

// Verifica se o valor da port é válido---------------------------------------------------------------------------------------------------------------------------------------------
int verify_port(char port[])
{
    if (atoi(port) <= 58000)
    {
        printf("Invalid port value\n");
        exit(1);
    }
    return 0;
}

// Impressão na linha de comandos da interface do utilizador------------------------------------------------------------------------------------------------------------------------
void print_user_interface()
{
    printf("-----------------INTERFACE DE UTILIZADOR-----------------\n");
    printf("n - Criação de um anel contendo apenas o nó\n\n");
    printf("b boot boot.IP boot.port - Entrada do nó no anel ao qual pertence o nó boot com endereço IP boot.IP e porto boot.port\n\n");
    printf("p pred pred.IP pred.port - Entrada do nó no anel sabendo que o seu predecessor será o nó pred com endereço IP pred.IP e porto pred.port\n\n");
    printf("c i i.IP i.port - Criação de um atalho para o nó i com endereço IP i.IP e porto i.port\n\n");
    printf("s - Mostra do estado do nó, consistindo em: (i) a sua chave, endereço IP e porto; (ii) a chave, endereço IP e porto do seu sucessor; (iii) a chave, endereço IP e porto do seu predecessor; e, por fim, (iv) a chave, endereço IP e porto do seu atalho, se houver\n\n");
    printf("f k - Procura da chave k, retornando a chave, o endereço IP e o porto do nó à qual a chave pertence\n\n");
    printf("leave - Saída do nó do anel\n\n");
    printf("e - Fecho da aplicação\n");
}

// Cálculo do file descriptor máximo para utilizar no select
int find_max_fd(int stdin, int welcoming_socket, int suc_socket, int pred_socket, int udp_socket)
{
    int i = max(stdin, welcoming_socket);
    int j = max(suc_socket, pred_socket);
    int k = max(i, udp_socket);
    int l = max(k, j);
    return l;
}

// Tentativa de criar uma funcionalidade na qual é feita a verificação se o pentry tem o predecessor correto
int verify_pred_conditions(int myself_key, int suc, int entry_key)
{
    int i = -1;
    if ((distance(myself_key, entry_key) >= distance(myself_key, suc)) || (myself_key == entry_key))
        return i;
    return 0;
}

// Criação das sockets welcoming TCP e UDP do nó
void create_node(server_node *myself, int KEY, char IP[], char PORT[])
{
    struct addrinfo hints_tcp, *res_tcp, hints_udp, *res_udp;
    int errcode = 0;

    myself->key = KEY;
    strcpy(myself->ip, IP);
    strcpy(myself->port, PORT);

    // CRIAÇÃO DA WELCOMING SOCKET QUE OUVE TODOS OS PEDIDOS DE CONEXÃO
    if ((myself->welcoming_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("ERRO CRIAÇÃO WELCOMING SOCKET");
        perror("error");
        exit(1); // error
    }

    // DEFINIR TODAS AS VARIÁVEIS DA ESTUTURA A 0, E DEPOIS SÓ MUDAR AS IMPORTANTES
    memset(&hints_tcp, 0, sizeof hints_tcp);
    hints_tcp.ai_family = AF_INET;       // IPv4
    hints_tcp.ai_socktype = SOCK_STREAM; // TCP socket
    hints_tcp.ai_flags = AI_PASSIVE;

    if ((errcode = getaddrinfo(IP, PORT, &hints_tcp, &res_tcp)) != 0) /*error*/
        exit(1);

    // ASSOCIAR A SOCKET COM O ENDEREÇO IP E PORTA DO SERVIDOR
    if (bind(myself->welcoming_socket, res_tcp->ai_addr, res_tcp->ai_addrlen) == -1) /*error*/
    {
        printf("ERRO NO BIND DO SERVER\n");
        perror("error");
        exit(1);
    }

    // O SERVER VAI FICAR A "OUVIR" PEDIDOS DE CONEXÃO (neste caso só deixamos acumular 5) ATÉ OS ACEITAR
    if (listen(myself->welcoming_socket, 5) == -1) /*error*/
    {
        printf("ERRO NO LISTEN DO SERVER\n");
        perror("error");
        exit(1);
    }

    //---------------------------------------------------- UDP SOCKET---------------------------------------------------------------------------

    // CRIAÇÃO DA UDP SOCKET
    if ((myself->udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("ERRO CRIAÇÃO UDP SOCKET");
        perror("error");
        exit(1); // error
    }

    // DEFINIR TODAS AS VARIÁVEIS DA ESTUTURA A 0, E DEPOIS SÓ MUDAR AS IMPORTANTES
    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_INET;      // IPv4
    hints_udp.ai_socktype = SOCK_DGRAM; // TCP socket
    hints_udp.ai_flags = AI_PASSIVE;

    if ((errcode = getaddrinfo(NULL, PORT, &hints_udp, &res_udp)) != 0) /*error*/
        exit(1);

    if (bind(myself->udp_socket, res_udp->ai_addr, res_udp->ai_addrlen) == -1)
    {
        printf("ERRO NO BIND DO SERVER\n");
        perror("error");
        exit(1);
    }

    return;
}

// Atualização dos valores do predecessor de um nó
void put_pred_in_node(server_node *myself, int KEY_PRED, char IP_PRED[], char PORT_PRED[]) // função que atualiza as informações do nó acerca do seu predecessor
{
    myself->key_pred = KEY_PRED;
    strcpy(myself->ip_pred, IP_PRED);
    strcpy(myself->port_pred, PORT_PRED);
    return;
}

// Atualização dos valores do sucessor de um nó
void put_suc_in_node(server_node *myself, int KEY_SUC, char IP_SUC[], char PORT_SUC[])
{
    myself->key_suc = KEY_SUC;
    strcpy(myself->ip_suc, IP_SUC);
    strcpy(myself->port_suc, PORT_SUC);
    return;
}

// Atualização dos valores do atalho de um nó
void put_chord_in_node(server_node *myself, int KEY_CHORD, char IP_CHORD[], char PORT_CHORD[])
{
    myself->key_chord = KEY_CHORD;
    strcpy(myself->ip_chord, IP_CHORD);
    strcpy(myself->port_chord, PORT_CHORD);
    return;
}

// Escrita de uma mensagem enviada via TCP
void write_in_socket(int new_socket, char buffer[])
{
    ssize_t n_bytes_left_to_read = 0, n_bytes_written = 0;
    char *ptr = buffer;

    n_bytes_left_to_read = strlen(buffer); // número de bytes por ler

    while (n_bytes_left_to_read > 0) // garantimos que toda a mensagem foi escrita
    {
        n_bytes_written = write(new_socket, buffer, n_bytes_left_to_read); // ir escrevendo a mensagem do buffer na new_socket

        if (n_bytes_written <= 0)
        {
            printf("ERRO NO WRITE IN SOCKET\n");
            return;
        }

        n_bytes_left_to_read -= n_bytes_written; // quando "n_bytes_left_to_read" chegar a 0 significa que já escrevemos todos os bytes da mensagem
        ptr += n_bytes_written;                  // avança para a posição seguinte da mensagem para escrever na socket
    }
}

// Escrita de uma mensagem enviada via UDP
void write_in_udp(int new_socket, char buffer[], struct addrinfo *res)
{
    ssize_t n_bytes_left_to_read = 0, n_bytes_written = 0;
    char *ptr = buffer;

    n_bytes_left_to_read = strlen(buffer); // número de bytes por ler

    // ESCREVER A MENSAGEM NA SOCKET
    while (n_bytes_left_to_read > 0) // garantimos que toda a mensagem foi escrita
    {
        n_bytes_written = sendto(new_socket, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen); // ir escrevendo a mensagem do buffer na new_socket

        if (n_bytes_written <= 0)
        {
            printf("ERRO NO WRITE IN UDP\n");
            return;
        }

        n_bytes_left_to_read -= n_bytes_written; // quando "n_bytes_left_to_read" chegar a 0 significa que já escrevemos todos os bytes da mensagem
        ptr += n_bytes_written;                  // avança para a posição seguinte da mensagem para escrever na socket
    }
}

// Criação de uma socket TCP para comunicação ou com o sucessor ou com o predecessor
int client_TCP(char IP[], char PORT[])
{
    struct addrinfo hints, *res;
    int new_socket = 0;

    // CRIAÇÃO DE SOCKET TCP -------------------------------------------------------------------------------------------------------
    new_socket = socket(AF_INET, SOCK_STREAM, 0); // TCP socket

    if (new_socket == -1)
    {
        printf("ERRO NA CRIAÇÃO DA SOCKET CLIENT_TCP\n");
        perror("error");
        exit(1); // error
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // IR BUSCAR O ENDEREÇO IP DO SERVER
    if (getaddrinfo(IP, PORT, &hints, &res) != 0)
    {
        printf("ERRO NO GETADDRINFO CLIENT_TCP\n");
        perror("error");
        exit(1);
    }

    // CONECTAR COM O SERVER
    if (connect(new_socket, res->ai_addr, res->ai_addrlen) != 0)
    {
        printf("ERRO NO CONNECT DO CLIENT_TCP\n");
        perror("error");
        exit(1);
    }

    return new_socket;
}

// Retorna o res, que contém informação sobre para onde enviar a mensagem via UDP
struct addrinfo *client_UDP(char IP[], char PORT[])
{
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;

    // OBTENÇÃO DO ENDEREÇO IP E PORTO
    if (getaddrinfo(IP, PORT, &hints, &res) != 0)
    {
        printf("ERRO NO GETADDRINFO CLIENT_TCP\n");
        perror("error");
        exit(1);
    }

    return res;
}

// Leitura da mensagem recebida via TCP, avaliação de que tipo de mensagem é, e correspondente tratamento de acordo com a informação recebida
int read_message(int fd, server_node *myself, struct addrinfo *res_udp, int sequence_number_find_table[], int sequence_number_bentry_table[], char ip_bentry[], char port_bentry[])
{
    char buffer[128], message_type[16], i_IP[16], i_port[16];
    memset(&buffer, '\0', 128);
    memset(&message_type, '\0', 16);
    memset(&i_IP, '\0', 16);
    memset(&i_port, '\0', 16);
    int i = 0;
    int k = 0, sequence_number = 0;

    if (read(fd, buffer, 128) < 1) // leitura da mensagem da socket, que vai para o buffer -> se for = 0 ou = -1 significa que a sessão foi abaixo
    {
        if (myself->tcp_suc_socket == fd) // conecção com o seu sucessor foi abaixo, significa que o mesmo deu leave, logo fechamos a conecção com ele
        {
            close(myself->tcp_suc_socket);
            myself->tcp_suc_socket = -2;
            return 0;
        }
        if (myself->tcp_pred_socket == fd) // conecção com o seu predecessor foi abaixo, significa que a sessão sofreu uma interrupção abrupta, fechamos todas as sockets e saímos do anel
        {
            close(myself->tcp_pred_socket);
            close(myself->tcp_suc_socket);
            close(myself->welcoming_socket);
            close(myself->udp_socket);
            myself->tcp_pred_socket = -1;
            myself->tcp_suc_socket = -1;
            myself->welcoming_socket = -1;
            myself->udp_socket = -1;
            put_pred_in_node(myself, -1, myself->ip, myself->port);
            put_suc_in_node(myself, -1, myself->ip, myself->port);
            put_chord_in_node(myself, -1, myself->ip, myself->port);
            return 0;
        }

        printf("ERRO READING MESSAGE FROM SOCKET\n");
        perror("error");
        exit(1);
    }

    if (sscanf(buffer, "%s", message_type) == -1) // a variável message_type diz de que tipo de mensagem se trata, p.e: se é de SELF, PRED...
    {
        printf("ERRO NA DETERMINAÇÃO DO MESSAGE TYPE!\n");
        perror("error");
        exit(1);
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // TRATAMENTO DA MENSAGEM DE SELF
    if (strcmp(message_type, "SELF") == 0)
    {

        if (sscanf(buffer, "%*s %d %s %s", &i, i_IP, i_port) == -1) // retira a informação do seu novo sucessor
        {
            printf("ERRO NA LEITURA DA MENSAGEM\n");
            perror("error");
            exit(1);
        }

        if (myself->tcp_suc_socket == -1) // a socket para quem vamos mandar o pred ainda não está criada
        {
            if (myself->key_suc == -1) // recebemos um self mas não queremos enviar um pred (caso nó entrante apenas)
            {
                put_suc_in_node(myself, i, i_IP, i_port);
                myself->tcp_suc_socket = fd;
                return 0;
            }

            myself->tcp_suc_socket = client_TCP(myself->ip_suc, myself->port_suc);
        }
        if ((distance(myself->key, myself->key_suc) < distance(myself->key, i)) && (myself->key != myself->key_suc)) // não enviar o pred no leave
        {
            close(myself->tcp_suc_socket);
            put_suc_in_node(myself, i, i_IP, i_port);
            myself->tcp_suc_socket = fd;
            return 0;
        }
        if ((distance(myself->key, myself->key_pred) == 0) && (myself->key != myself->key_suc)) // não enviar o pred no leave, caso específico do nó que fica sozinho no anel
        {
            close(myself->tcp_suc_socket);
            put_suc_in_node(myself, i, i_IP, i_port);
            myself->tcp_suc_socket = fd;
            return 0;
        }

        //------------------------------------------------------------------------------------------------------------------------------------------
        // ENVIO DA MENSAGEM PRED
        sprintf(buffer, "PRED %d %s %s\n", i, i_IP, i_port); // colocação da mensagem de PRED num buffer
        write_in_socket(myself->tcp_suc_socket, buffer);
        // atualização da info do sucessor na estrutura do servidor que recebeu o SELF, que só poderá ser feita após o envio de PRED
        put_suc_in_node(myself, i, i_IP, i_port);
        myself->tcp_suc_socket = fd;
        return 0;
    }
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // TRATAMENTO DA MENSAGEM DE PRED
    else if (strcmp(message_type, "PRED") == 0)
    {

        if (sscanf(buffer, "%*s %d %s %s", &i, i_IP, i_port) == -1) // retira a informação do seu novo predecessor
        {
            printf("ERRO NA LEITURA DA MENSAGEM\n");
            perror("error");
            exit(1);
        }
        close(myself->tcp_pred_socket);
        put_pred_in_node(myself, i, i_IP, i_port);

        // criação da socket client que irá comunicar com o seu novo predecessor
        myself->tcp_pred_socket = client_TCP(myself->ip_pred, myself->port_pred);

        sprintf(buffer, "SELF %d %s %s\n", myself->key, myself->ip, myself->port); // colocação da mensagem de SELF num buffer
        write_in_socket(myself->tcp_pred_socket, buffer);
        return 0;
    }
    else if (strcmp(message_type, "FND") == 0)
    {

        if (sscanf(buffer, "%*s %d %d %d %s %s\n", &k, &sequence_number, &i, i_IP, i_port) == -1)
        {
            printf("ERRO NA LEITURA DA MENSAGEM\n");
            perror("error");
            exit(1);
        }
        if (distance(myself->key, k) < distance(myself->key_suc, k)) // a chave pertence-me
        {
            if ((myself->key_chord != myself->key) && (distance(myself->key_chord, k) < distance(myself->key_suc, k)))
            {
                res_udp = client_UDP(myself->ip_chord, myself->port_chord);
                sprintf(buffer, "RSP %d %d %d %s %s", i, sequence_number, myself->key, myself->ip, myself->port);
                write_in_udp(myself->udp_socket, buffer, res_udp); // escrita da msg na socket do atalho
                if (waiting_ack(myself, buffer, myself->ip_chord, myself->port_chord) == -1)
                {
                    sprintf(buffer, "RSP %d %d %d %s %s\n", i, sequence_number, myself->key, myself->ip, myself->port);
                    write_in_socket(myself->tcp_suc_socket, buffer);
                    put_chord_in_node(myself, myself->key, myself->ip, myself->port);
                }
                return 0;
            }
            sprintf(buffer, "RSP %d %d %d %s %s\n", i, sequence_number, myself->key, myself->ip, myself->port); // envia a mensagem de resposta ao nó procurante
            write_in_socket(myself->tcp_suc_socket, buffer);
        }
        else if ((myself->key_chord != myself->key) && (distance(myself->key_chord, k) < distance(myself->key_suc, k)))
        {
            res_udp = client_UDP(myself->ip_chord, myself->port_chord);
            sprintf(buffer, "FND %d %d %d %s %s", k, sequence_number, i, i_IP, i_port);  // colocação da msg num buffer
            write_in_udp(myself->udp_socket, buffer, res_udp);                           // escrita da msg na socket do atalho
            if (waiting_ack(myself, buffer, myself->ip_chord, myself->port_chord) == -1) // caso não recebamos o ACK, enviamos a mensagem para o sucessor e eliminamos o atalho
            {
                sprintf(buffer, "FND %d %d %d %s %s\n", k, sequence_number, i, i_IP, i_port);
                write_in_socket(myself->tcp_suc_socket, buffer);
                put_chord_in_node(myself, myself->key, myself->ip, myself->port);
            }
            return 0;
        }
        else if (distance(myself->key, k) > distance(myself->key_suc, k)) // a chave não me pertence
        {
            write_in_socket(myself->tcp_suc_socket, buffer); // reencaminho a mensagem para o meu sucessor
        }
        return 0;
    }
    if (strcmp(message_type, "RSP") == 0)
    {
        if (sscanf(buffer, "%*s %d %d %d %s %s\n", &k /*nó de destino*/, &sequence_number, &i, i_IP, i_port) == -1)
        {
            printf("ERRO NA LEITURA DA MENSAGEM\n");
            perror("error");
            exit(1);
        }
        if (sequence_number_find_table[sequence_number] == 1) // fui eu que iniciei a procura
        {
            printf("key: %d, ip_adress: %s, port: %s\n", i, i_IP, i_port);
            sequence_number_find_table[sequence_number] = 0; // já lemos a resposta, logo tiramos o sequence_number da tabela
            return 0;
        }

        else if (sequence_number_bentry_table[sequence_number] == 1) // fui eu que iniciei a procura
        {
            res_udp = client_UDP(ip_bentry, port_bentry);
            sprintf(buffer, "EPRED %d %s %s", i, i_IP, i_port); // colocação da msg num buffer
            write_in_udp(myself->udp_socket, buffer, res_udp);
            if (waiting_ack(myself, buffer, ip_bentry, port_bentry) == -1)
            {
                printf("Error sending EPRED in bentry\n");
            }
            sequence_number_bentry_table[sequence_number] = 0;
            return 0;
        }

        else if ((myself->key_chord != myself->key) && (distance(myself->key_chord, k) < distance(myself->key_suc, k)))
        {
            res_udp = client_UDP(myself->ip_chord, myself->port_chord);
            sprintf(buffer, "RSP %d %d %d %s %s", k, sequence_number, i, i_IP, i_port); // colocação da msg num buffer
            write_in_udp(myself->udp_socket, buffer, res_udp);                          // escrita da msg na socket do atalho
            if (waiting_ack(myself, buffer, myself->ip_chord, myself->port_chord) == -1)
            {
                sprintf(buffer, "RSP %d %d %d %s %s\n", k, sequence_number, i, i_IP, i_port); // colocação da msg num buffer
                write_in_socket(myself->tcp_suc_socket, buffer);
                put_chord_in_node(myself, myself->key, myself->ip, myself->port);
            }
            return 0;
        }

        else
        {
            write_in_socket(myself->tcp_suc_socket, buffer); // reencaminho a mensagem para o nó seguinte
        }
    }

    return 0;
}

// Leitura da mensagem recebida via UDP, avaliação de que tipo de mensagem é, e correspondente tratamento de acordo com a informação recebida
int read_message_udp(int fd, server_node *myself, struct addrinfo *res_udp, int sequence_number_find_table[], int sequence_number_bentry_table[], char ip_bentry[], char port_bentry[], int sequence_number_bentry)
{
    char buffer[128], message_type[16], i_IP[16], i_port[16], buffer_ACK[16];
    memset(&buffer, '\0', 128);
    memset(&message_type, '\0', 16);
    memset(&i_IP, '\0', 16);
    memset(&i_port, '\0', 16);
    memset(&buffer_ACK, '\0', 16);
    int k = 0, i = 0, sequence_number = 0;
    struct sockaddr addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    char ip_ack[NI_MAXHOST], port_ack[NI_MAXSERV];
    memset(&ip_ack, '\0', NI_MAXHOST);
    memset(&port_ack, '\0', NI_MAXSERV);

    sprintf(buffer_ACK, "ACK");

    if (recvfrom(fd, buffer, 128, 0, &addr, &addrlen) < 1) // retorna o número de bytes recebidos
    {
        printf("ERRO READING MESSAGE FROM SOCKET UDP\n");
        perror("error");
        exit(1);
    }

    if ((getnameinfo(&addr, addrlen, ip_ack, NI_MAXHOST, port_ack, NI_MAXSERV, 0)) != 0) // guardar a informação para enviar um ACK
    {
        printf("ERRO NO GETNAMEINFO\n");
        perror("error");
        exit(1);
    }

    printf("%s\n", buffer);
    if (sscanf(buffer, "%s", message_type) == -1) // a variável message_type diz de que tipo de mensagem se trata, p.e: se é de SELF, PRED... e as outras todas que irão surgir
    {
        printf("ERRO NA DETERMINAÇÃO DO MESSAGE TYPE UDP!\n");
        perror("error");
        exit(1);
    }

    if (strcmp(message_type, "FND") == 0)
    {
        // Envio do ACK
        res_udp = client_UDP(ip_ack, port_ack);
        write_in_udp(myself->udp_socket, buffer_ACK, res_udp);

        if (sscanf(buffer, "%*s %d %d %d %s %s\n", &k, &sequence_number, &i, i_IP, i_port) == -1)
        {
            printf("ERRO NA LEITURA DA MENSAGEM UDP\n");
            perror("error");
            exit(1);
        }

        if (distance(myself->key, k) < distance(myself->key_suc, k)) // a chave pertence-me
        {
            if ((myself->key_chord != myself->key) && (distance(myself->key_chord, k) < distance(myself->key_suc, k)))
            {
                res_udp = client_UDP(myself->ip_chord, myself->port_chord);
                sprintf(buffer, "RSP %d %d %d %s %s", i, sequence_number, myself->key, myself->ip, myself->port); // colocação da msg num buffer
                write_in_udp(myself->udp_socket, buffer, res_udp);                                                // escrita da msg na socket do atalho
                if (waiting_ack(myself, buffer, myself->ip_chord, myself->port_chord) == -1)
                {
                    sprintf(buffer, "RSP %d %d %d %s %s\n", i, sequence_number, myself->key, myself->ip, myself->port); // envia a mensagem de resposta ao nó procurante
                    write_in_socket(myself->tcp_suc_socket, buffer);
                    put_chord_in_node(myself, myself->key, myself->ip, myself->port);
                }
                return 0;
            }
            sprintf(buffer, "RSP %d %d %d %s %s\n", i, sequence_number, myself->key, myself->ip, myself->port); // envia a mensagem de resposta ao nó procurante
            write_in_socket(myself->tcp_suc_socket, buffer);
            return 0;
        }
        else if ((myself->key_chord != myself->key) && (distance(myself->key_chord, k) < distance(myself->key_suc, k)))
        {
            res_udp = client_UDP(myself->ip_chord, myself->port_chord);
            sprintf(buffer, "FND %d %d %d %s %s", k, sequence_number, i, i_IP, i_port); // colocação da msg num buffer
            write_in_udp(myself->udp_socket, buffer, res_udp);                          // escrita da msg na socket do atalho
            if (waiting_ack(myself, buffer, myself->ip_chord, myself->port_chord) == -1)
            {
                sprintf(buffer, "FND %d %d %d %s %s\n", k, sequence_number, i, i_IP, i_port); // colocação da msg num buffer
                write_in_socket(myself->tcp_suc_socket, buffer);
                put_chord_in_node(myself, myself->key, myself->ip, myself->port);
            }
            return 0;
        }
        else if (distance(myself->key, k) > distance(myself->key_suc, k)) // a chave não me pertence
        {
            write_in_socket(myself->tcp_suc_socket, buffer); // reencaminho a mensagem para o meu sucessor
        }
        return 0;
    }
    else if (strcmp(message_type, "RSP") == 0)
    {
        // Envio do ACK
        res_udp = client_UDP(ip_ack, port_ack);
        write_in_udp(myself->udp_socket, buffer_ACK, res_udp);

        if (sscanf(buffer, "%*s %d %d %d %s %s\n", &k /*nó de destino*/, &sequence_number, &i, i_IP, i_port) == -1)
        {
            printf("ERRO NA LEITURA DA MENSAGEM UDP\n");
            perror("error");
            exit(1);
        }
        if (sequence_number_find_table[sequence_number] == 1) // fui eu que iniciei a procura
        {
            printf("key: %d, ip_adress: %s, port: %s\n", i, i_IP, i_port);
            sequence_number_find_table[sequence_number] = 0;
            return 0;
        }
        else if (sequence_number_bentry_table[sequence_number] == 1) // fui eu que iniciei a procura mas para o bentry
        {
            res_udp = client_UDP(ip_bentry, port_bentry);
            sprintf(buffer, "EPRED %d %s %s\n", i, i_IP, i_port);
            write_in_udp(myself->udp_socket, buffer, res_udp);
            if (waiting_ack(myself, buffer, ip_bentry, port_bentry) == -1)
            {
                printf("Error sending EPRED in bentry\n");
            }
            sequence_number_bentry_table[sequence_number] = 0;
            return 0;
        }
        else if ((myself->key_chord != myself->key) && (distance(myself->key_chord, k) < distance(myself->key_suc, k)))
        {
            res_udp = client_UDP(myself->ip_chord, myself->port_chord);
            sprintf(buffer, "RSP %d %d %d %s %s", k, sequence_number, i, i_IP, i_port); // colocação da msg num buffer
            write_in_udp(myself->udp_socket, buffer, res_udp);                          // escrita da msg na socket do atalho
            if (waiting_ack(myself, buffer, myself->ip_chord, myself->port_chord) == -1)
            {
                sprintf(buffer, "RSP %d %d %d %s %s\n", k, sequence_number, i, i_IP, i_port); // colocação da msg num buffer
                write_in_socket(myself->tcp_suc_socket, buffer);
                put_chord_in_node(myself, myself->key, myself->ip, myself->port);
            }
            return 0;
        }
        else // if (distance(myself->key, k) > distance(myself->key_suc, k))// não fui eu que iniciei a procura
        {
            write_in_socket(myself->tcp_suc_socket, buffer); // reencaminho a mensagem para o nó seguinte
            return 0;
        }
    }
    else if (strcmp(message_type, "EFND") == 0)
    {
        // Envio do ACK
        res_udp = client_UDP(ip_ack, port_ack);
        write_in_udp(myself->udp_socket, buffer_ACK, res_udp);

        if (sscanf(buffer, "%*s %d\n", &i) == -1) // i guarda a key do nó que quer entrar no anel
        {
            printf("ERRO NA LEITURA DA MENSAGEM UDP\n");
            perror("error");
            exit(1);
        }
        if ((sequence_number_bentry >= 0) && (sequence_number_bentry < 99)) // incrementar 1 sempre que fazemos um novo find, mas antes verificar se está dentro dos parâmetros
        {
            sequence_number_bentry++;
        }
        else // se for 99, voltamos ao 0
        {
            sequence_number_bentry = 0;
        }
        sequence_number_bentry_table[sequence_number_bentry] = 1;

        if ((getnameinfo(&addr, addrlen, ip_bentry, NI_MAXHOST, port_bentry, NI_MAXSERV, 0)) != 0)
        {
            printf("ERRO NO GETNAMEINFO\n");
            perror("error");
            exit(1);
        }

        if (distance(myself->key, i) <= distance(myself->key_suc, i)) // primeiro verificamos se somos o predecessor do nó entrante
        {
            sequence_number_bentry_table[sequence_number_bentry] = 0;
            sprintf(buffer, "EPRED %d %s %s", myself->key, myself->ip, myself->port);
            res_udp = client_UDP(ip_bentry, port_bentry);
            write_in_udp(myself->udp_socket, buffer, res_udp);
            if (waiting_ack(myself, buffer, ip_bentry, port_bentry) == -1)
            {
                printf("Error sending EPRED in bentry\n");
            }

            return 0;
        }

        // condição que verifica se a corda está mais próxima da key a ser procurada
        else if ((myself->key_chord != myself->key) && (distance(myself->key_chord, i) < distance(myself->key_suc, i)))
        {
            res_udp = client_UDP(myself->ip_chord, myself->port_chord);
            sprintf(buffer, "FND %d %d %d %s %s", i, sequence_number_bentry, myself->key, myself->ip, myself->port); // colocação da msg num buffer
            write_in_udp(myself->udp_socket, buffer, res_udp);                                                       // escrita da msg na socket do atalho
            if (waiting_ack(myself, buffer, myself->ip_chord, myself->port_chord) == -1)
            {
                sprintf(buffer, "FND %d %d %d %s %s\n", i, sequence_number_bentry, myself->key, myself->ip, myself->port);
                write_in_socket(myself->tcp_suc_socket, buffer);
                put_chord_in_node(myself, myself->key, myself->ip, myself->port);
            }
            return 0;
        }
        else
        {
            sprintf(buffer, "FND %d %d %d %s %s\n", i, sequence_number_bentry, myself->key, myself->ip, myself->port); // colocação da msg num buffer
            write_in_socket(myself->tcp_suc_socket, buffer);
            return 0;
        }
    }

    else if (strcmp(message_type, "EPRED") == 0)
    {
        if (sscanf(buffer, "%*s %d %s %s\n", &i, i_IP, i_port) == -1) // guardamos a informação do nosso predecessor
        {
            printf("ERRO NA LEITURA DA MENSAGEM UDP\n");
            perror("error");
            exit(1);
        }

        // Envio do ACK
        res_udp = client_UDP(ip_ack, port_ack);
        write_in_udp(myself->udp_socket, buffer_ACK, res_udp);

        put_pred_in_node(myself, i, i_IP, i_port);
        sprintf(buffer, "SELF %d %s %s\n", myself->key, myself->ip, myself->port); // colocar na string self a informação a enviar [dar-se a conhecer ao predecessor]
        myself->tcp_pred_socket = client_TCP(myself->ip_pred, myself->port_pred);  // retorna a socket que vai passar a mensagem
        write_in_socket(myself->tcp_pred_socket, buffer);
        return 0;
    }

    return 0;
}