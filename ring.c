#include "entry.h"

int main(int argc, char *argv[])
{

    // DECLARAÇÃO E INICIALIZAÇÃO DE VARIÁVEIS-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    // Buffers que vão guardar as mensagens enviadas------------------------------------------------------------------------------------------
    char self_buffer[128];
    memset(&self_buffer, '\0', 128);
    char pred_buffer[128];
    memset(&pred_buffer, '\0', 128);
    char fnd_buffer[128];
    memset(&fnd_buffer, '\0', 128);
    char efnd_buffer[128];
    memset(&efnd_buffer, '\0', 128);
    
    // Inicialização dos valores de myself -> inicializamos tudo?
    server_node myself;
    myself.welcoming_socket = -1;
    myself.tcp_pred_socket = -1;
    myself.tcp_suc_socket = -1;
    myself.udp_socket = -1;

    myself.key_pred = 0;
    memset(myself.ip_pred, '\0', 16);
    memset(myself.port_pred, '\0', 16);

    myself.key_suc = 0;
    memset(myself.ip_suc, '\0', 16);
    memset(myself.port_suc, '\0', 16);

    memset(myself.ip_chord, '\0', 16);
    memset(myself.port_chord, '\0', 16);
    myself.key_chord = -1; // se o nó ainda não tiver sido criado, a sua key será -1

    // Inicialização de vetores utilizados para armazenar temporariamente informação obtida através da linha de comandos--------------------------------------------------------------------------------------------------------------
    char ip[16], ip_pred[16], port[16], port_pred[16];
    memset(&ip, '\0', 16);
    memset(&ip_pred, '\0', 16);
    memset(&port, '\0', 16);
    memset(&port_pred, '\0', 16);
    
    // Gerar um número de sequência entre 0 e 99----------------------------------------------------------------------------------------------
    srand(time(0));
    int sequence_number = rand() % 100;
   
    // Vetores que contêm números de sequência para sabermos se a RSP teve origem num find normal ou num bentry, cujas posições apenas estão a '1' se o index correspondente for um dos números de seuqência criados pelo nó
    int sequence_number_find_table[100]; // quando fazemos um FND pelo comando find, colocamos o número de sequência aqui
    memset(&sequence_number_find_table, 0, sizeof(sequence_number_find_table));
    int sequence_number_bentry_table[100]; // quando fazemos um FND pelo comando bentry, colocamos o número de sequência aqui
    memset(&sequence_number_bentry_table, 0, sizeof(sequence_number_bentry_table));
   
    // Flag que guarda o último comando executado de entre 'n', 'p', 'l', e 'b' para controlar o estado do nó, e consequentemente verificarmos se os comandos que tentamos fazer são válidos
    char command_type_flag[2];
    memset(command_type_flag, '\0', 2);
    strcpy(command_type_flag, "z");
    
    // Variáveis que guardam os valores do endereço ip e port para o qual enviamos o EPRED
    char ip_bentry[NI_MAXHOST] = {0}, port_bentry[NI_MAXSERV] = {0};
    
    // Sigaction----------------------------------------------------------------------------------------------------------------------------------
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1)
        exit(1);
   
    // Variáveis auxiliares ao desenvolvimento do programa-------------------------------------------------------------------------------------------------------------------------------------
    int key = 0, key_pred = 0, k = -1;
    int maxfd = 0;
    char *command_ptr = NULL;
    char command[128] = {0};
    fd_set socket_set; // lista de posições que guarda se vêm informações do stdin, e das sockets tcp e udp
    struct sockaddr addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);
    struct addrinfo *res_udp;
    int socket_accept;
   
    // FIM DA DECLARAÇÃO E INICIALIZAÇÃO DE VARIÁVEIS-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    // Verificar se a aplicação foi bem invocada---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    verify_arguments(argc, argv, &key, ip, port);
    
    // Imprimir a interface do utilizador----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    print_user_interface();
   
    // Ciclo While do Select-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    while (1)
    {
        FD_ZERO(&socket_set); // Inicialização da lista socket_set toda a zeros

        FD_SET(0, &socket_set); // Colocação de stdin no set de sockets a ler no Select

        // Colocação das restantes sockets no set de sockets a ler pelo Select caso existam---------------------------------------------------------------------------------------------------------------------------------------------

        if (myself.welcoming_socket != -1) // Se a socket estiver a -1 quer dizer que ou ainda não foi criada ou foi fechada
        {
            FD_SET(myself.welcoming_socket, &socket_set);
        }
        if ((myself.tcp_pred_socket != -1) && (myself.tcp_pred_socket != -2))
        {
            FD_SET(myself.tcp_pred_socket, &socket_set);
        }
        if ((myself.tcp_suc_socket != -1) && (myself.tcp_suc_socket != -2))
        {
            FD_SET(myself.tcp_suc_socket, &socket_set);
        }
        if (myself.udp_socket != -1)
        {
            FD_SET(myself.udp_socket, &socket_set);
        }
        
        // Cálculo do fd máximo para utilizar no Select
        maxfd = find_max_fd(0, myself.welcoming_socket, myself.tcp_suc_socket, myself.tcp_pred_socket, myself.udp_socket);

        // Verificação se ocorreu algum erro no Select
        if (select(maxfd + 1, &socket_set, NULL, NULL, NULL) < 0)
        {
            printf("ERROR NO SELECT\n");
            perror("error");
            exit(1);
        }

        // Verificação se stdin está pronto para leitura (teclado pressionado)
        if (FD_ISSET(0, &socket_set))
        {
            command_ptr = fgets(command, 25, stdin); // Apontador para a primeira posição de command, que contém a informação da linha de comandos

            switch (*command_ptr) // Primeira letra escrita na linha de comandos
            {
            case 'n': // New

                // Caso esta flag indique que foi realizado previamente um pentry, bentry ou new sem ocorrer leave, não se pode efetura este comando
                if ((strcmp(command_type_flag, "p") == 0) || (strcmp(command_type_flag, "b") == 0) || (strcmp(command_type_flag, "n") == 0))
                {
                    printf("You can't make command 'n' after command '%s'\n", command_type_flag);
                    break;
                }
                strcpy(command_type_flag, "n");      // Atualização da flag de controlo do estado do nó
                create_node(&myself, key, ip, port); // Criação de um servidor TCP e UDP

                // Atualização como seu próprio predecessor e sucessor
                put_pred_in_node(&myself, key, ip, port);
                put_suc_in_node(&myself, key, ip, port);
                put_chord_in_node(&myself, key, ip, port);

                break;

            case 'p': // Pentry

                // Verificação se este comando é válido tendo em conta o estado do nó
                if ((strcmp(command_type_flag, "p") == 0) || (strcmp(command_type_flag, "b") == 0) || (strcmp(command_type_flag, "n") == 0))
                {
                    printf("You can't make command 'p' after command '%s'\n", command_type_flag);
                    break;
                }
                // Atualização do estado do nó
                strcpy(command_type_flag, "p");

                // Armazenamento em variáveis temporárias da informação do comando inserido
                if (sscanf(command, "%*s %d %s %s", &key_pred, ip_pred, port_pred) < 3)
                {
                    printf("ERRO PENTRY\n");
                    exit(1);
                }
                if (verify_key(key_pred) == -1) // Verificação se a chave do nó que quer entrar no anel pertence ao range 0-31
                    break;

                if (key == key_pred) // Se o nó estiver a tentar entrar como seu próprio sucessor, não permitimos
                {
                    printf("The pred key inserted is not valid\n");
                    break;
                }

                create_node(&myself, key, ip, port);
                put_pred_in_node(&myself, key_pred, ip_pred, port_pred);
                put_suc_in_node(&myself, -1, ip, port); // Colocamos o seu sucessor como si próprio mas com key = -1, para distinguir o comando pentry no envio de SELF's e PRED's (condição de paragem)
                put_chord_in_node(&myself, key, ip, port);
                sprintf(self_buffer, "SELF %d %s %s\n", myself.key, myself.ip, myself.port); // Colocação na string self a informação a enviar [dar-se a conhecer ao predecessor]
                myself.tcp_pred_socket = client_TCP(myself.ip_pred, myself.port_pred);       // Criação de uma socket TCP para comunicar com o seu predecessor e enviar o SELF
                write_in_socket(myself.tcp_pred_socket, self_buffer);
                break;

            case 's':                       // Show
                if (myself.key_chord == -1) // Caso o nó ainda não tenha sido criado ou tenha dado 'leave', dizemos que o nó não pertence ao anel
                {
                    printf("This key does not belong to the ring\n");
                    break;
                }

                printf("myself: %d %s %s\n", myself.key, myself.ip, myself.port);
                printf("sucessor: %d %s %s\n", myself.key_suc, myself.ip_suc, myself.port_suc);
                printf("predecessor: %d %s %s\n", myself.key_pred, myself.ip_pred, myself.port_pred);
                if ((myself.key_chord != myself.key) && (strcmp(myself.port_chord, myself.port) != 0)) // Só imprimimos o atalho caso o nó o tenha
                    printf("chord: %d %s %s\n", myself.key_chord, myself.ip_chord, myself.port_chord);
                break;

            case 'l': // Leave

                // Verificação da flag que controla o estado do anel
                if (strcmp(command_type_flag, "l") == 0)
                {
                    printf("You can't make command 'l' after command '%s'\n", command_type_flag);
                    break;
                }
                if (strcmp(command_type_flag, "z") == 0) // A flag conter a letra 'z' equivale a dizer que o nó ainda não entrou no anel
                {
                    printf("You can't leave because the key is not in the ring yet\n");
                    break;
                }
                strcpy(command_type_flag, "l"); // Atualização da flag

                // Só para o caso de apenas termos um anel no nó, não enviamos o pred
                if ((myself.key == key) && (myself.key_suc == key) && (myself.key_pred == key))
                {
                    // Fecho das sockets
                    close(myself.tcp_pred_socket);
                    close(myself.tcp_suc_socket);
                    close(myself.welcoming_socket);
                    close(myself.udp_socket);
                    // Atualização dos valores dos file descriptors
                    myself.tcp_pred_socket = -1;
                    myself.tcp_suc_socket = -1;
                    myself.welcoming_socket = -1;
                    myself.udp_socket = -1;
                    // Atualização da informação sobre o seu predecessor, sucessor e atalho
                    put_pred_in_node(&myself, -1, ip, port);
                    put_suc_in_node(&myself, -1, ip, port);
                    put_chord_in_node(&myself, -1, ip, port);
                    break;
                }

                sprintf(pred_buffer, "PRED %d %s %s\n", myself.key_pred, myself.ip_pred, myself.port_pred);
                write_in_socket(myself.tcp_suc_socket, pred_buffer); // Envia um PRED com a informação do seu predecessor
                // Fecho das sockets
                close(myself.tcp_pred_socket);
                close(myself.tcp_suc_socket);
                close(myself.welcoming_socket);
                close(myself.udp_socket);
                // Atualização dos valores dos file descriptors
                myself.tcp_pred_socket = -1;
                myself.tcp_suc_socket = -1;
                myself.welcoming_socket = -1;
                myself.udp_socket = -1;
                // Atualização da informação sobre o seu predecessor, sucessor e atalho
                put_pred_in_node(&myself, -1, ip, port);
                put_suc_in_node(&myself, -1, ip, port);
                put_chord_in_node(&myself, -1, ip, port);
                break;

            case 'e': // Exit
                // Só podemos dar exit se tivermos feito 'leave' previamente ou o nó não tiver entrado no anel
                if ((strcmp(command_type_flag, "l") == 0) || (strcmp(command_type_flag, "z") == 0))
                {
                    exit(0);
                }
                printf("You can't make command 'e' without command 'l' before\n");
                break;

            case 'f':                                  // Find
                if (sscanf(command, "%*s %d", &k) < 1) // k é a chave que procuramos
                {
                    printf("ERRO FIND\n");
                    exit(1);
                }
                if (distance(myself.key, k) < distance(myself.key_suc, k)) // Verificação se somos nós o predecessor da chave procurada
                {
                    printf("Eu sou o nó %d e a chave %d pertence-me\n", myself.key, k);
                    printf("key: %d, ip_adress: %s, port: %s\n", myself.key, myself.ip, myself.port);
                    break;
                }

                // Incrementar 1 sempre que fazemos um novo find, mas antes verificar se está dentro dos parâmetros 0-99
                if ((sequence_number >= 0) && (sequence_number < 99))
                {
                    sequence_number++;
                }
                else // se for 99, voltamos ao 0
                {
                    sequence_number = 0;
                }

                sequence_number_find_table[sequence_number] = 1; // Número de sequência ativo

                // Verificação se a corda está mais próxima da key a ser procurada
                if ((myself.key_chord != myself.key) && (distance(myself.key_chord, k) < distance(myself.key_suc, k)))
                {
                    printf("A procura vai pelo atalho\n");
                    res_udp = client_UDP(myself.ip_chord, myself.port_chord);
                    sprintf(fnd_buffer, "FND %d %d %d %s %s", k, sequence_number, myself.key, myself.ip, myself.port);
                    write_in_udp(myself.udp_socket, fnd_buffer, res_udp); // Escrita da mensagem na socket do atalho

                    break;
                }
                // Caso contrário, envio para o sucessor
                sprintf(fnd_buffer, "FND %d %d %d %s %s\n", k, sequence_number, myself.key, myself.ip, myself.port);
                write_in_socket(myself.tcp_suc_socket, fnd_buffer);

                break;

            case 'c': // Chord
                if (sscanf(command, "%*s %d %s %s", &key, ip, port) < 3)
                {
                    printf("Invalid arguments\n");
                    break;
                }
                // Caso o atalho seja criado para um dos nós adjacentes, para si próprio, ou estejamos a tentar fazer 'c' sem termos feito 'n', 'p', ou 'b' antes, não criamos o atalho
                if ((myself.key_pred == key) || (myself.key_suc == key) || (myself.key == key) || (myself.key_chord == -1))
                {
                    printf("You're not allowed to create this chord\n");
                    break;
                }
                put_chord_in_node(&myself, key, ip, port); // Atualização dos valores do atalho
                break;

            case 'd': // Echord

                // Verificação se existe um atalho para remover
                if (myself.key_chord == myself.key)
                {
                    printf("There is no chord to be removed\n");
                    break;
                }
                // Remoção do valor do atalho da estrutura do nó
                put_chord_in_node(&myself, myself.key, myself.ip, myself.port);
                break;

            case 'b': // Bentry

                // Verificação do valor da flag que controla o estado do nó
                if ((strcmp(command_type_flag, "p") == 0) || (strcmp(command_type_flag, "b") == 0) || (strcmp(command_type_flag, "n") == 0))
                {
                    printf("You can't make command 'b' after command '%s'\n", command_type_flag);
                    break;
                }

                strcpy(command_type_flag, "b");

                if (sscanf(command, "%*s %d %s %s", &key_pred, ip_pred, port_pred) < 3) // A variável chama-se key_pred mas não é para o predecessor, é para o nó boot
                {
                    break;
                }
                create_node(&myself, key, ip, port);
                put_pred_in_node(&myself, key, ip, port);
                put_suc_in_node(&myself, -1, ip, port);
                put_chord_in_node(&myself, key, ip, port);

                sprintf(efnd_buffer, "EFND %d", key);     // colocar na string efnd a informação a enviar
                res_udp = client_UDP(ip_pred, port_pred); // vamos enviar o EFND para o nó boot, por UDP
                write_in_udp(myself.udp_socket, efnd_buffer, res_udp);

                // Verificação do retorno do ACK
                if (waiting_ack(&myself, efnd_buffer, ip_pred, port_pred) == -1)
                {
                    printf("Invalid bentry key\n");

                    break;
                }

                break;
            }

            FD_CLR(0, &socket_set); // Retira-se a linha de comandos do socket_set pois já foi lida
        }

        // Verificação de welcoming socket TCP no set
        if (FD_ISSET(myself.welcoming_socket, &socket_set))
        {
            addrlen = sizeof(addr);

            if ((socket_accept = accept(myself.welcoming_socket, &addr, &addrlen)) == -1)
            {
                printf("ERRO NO ACCEPT DO SERVIDOR");
                perror("error:  ");
                exit(1);
            }
            read_message(socket_accept, &myself, res_udp, sequence_number_find_table, sequence_number_bentry_table, ip_bentry, port_bentry); // função encarregue de ler e tratar de TODOS os tipos de mensagens recebidas pelos servidores
            FD_CLR(myself.welcoming_socket, &socket_set);
        }

        if (FD_ISSET(myself.tcp_pred_socket, &socket_set)) // SOCKET PREDECESSOR
        {
            read_message(myself.tcp_pred_socket, &myself, res_udp, sequence_number_find_table, sequence_number_bentry_table, ip_bentry, port_bentry); // função encarregue de ler e tratar de TODOS os tipos de mensagens recebidas pelos servidores
            FD_CLR(myself.tcp_pred_socket, &socket_set);
        }
        if (FD_ISSET(myself.tcp_suc_socket, &socket_set)) // SOCKET SUCESSOR
        {
            read_message(myself.tcp_suc_socket, &myself, res_udp, sequence_number_find_table, sequence_number_bentry_table, ip_bentry, port_bentry); // função encarregue de ler e tratar de TODOS os tipos de mensagens recebidas pelos servidores
            FD_CLR(myself.tcp_suc_socket, &socket_set);
        }
        if (FD_ISSET(myself.udp_socket, &socket_set)) // SOCKET UDP
        {
            read_message_udp(myself.udp_socket, &myself, res_udp, sequence_number_find_table, sequence_number_bentry_table, ip_bentry, port_bentry, sequence_number); // função encarregue de ler e tratar de TODOS os tipos de mensagens recebidas pelos servidores
            FD_CLR(myself.udp_socket, &socket_set);
        }
    }
    return 0;
}
