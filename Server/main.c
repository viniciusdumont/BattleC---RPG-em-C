#include <pthread.h> 
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORTA 8080
#define TAMANHO_BUFFER 512

__thread unsigned int seed_thread; 

struct Jogador {
    int id_classe; 
    int hp_atual;
    int hp_max;
    int defesa_base; 
    int dano_dado;
    int esta_defendendo; 
    int esta_esquivando; 
    int bonus_ac_temp; 
};

void inicializar_jogador(struct Jogador *jogador, int classe_id) {
    jogador->id_classe = classe_id;
    jogador->esta_defendendo = 0;
    jogador->esta_esquivando = 0;
    jogador->bonus_ac_temp = 0;
    
    switch (classe_id) {
        case 1: // Cavaleiro
            jogador->hp_max = 30; 
            jogador->defesa_base = 14; 
            jogador->dano_dado = 6;
            break;
        case 2: // Mago
            jogador->hp_max = 20; 
            jogador->defesa_base = 11; 
            jogador->dano_dado = 10;
            break;
        case 3: // Arqueiro
            jogador->hp_max = 25; 
            jogador->defesa_base = 12; 
            jogador->dano_dado = 6;
            break;
        case 4: // Bárbaro
            jogador->hp_max = 35; 
            jogador->defesa_base = 11; 
            jogador->dano_dado = 8;
            break;
        default: 
            jogador->hp_max = 10; 
            jogador->defesa_base = 10; 
            jogador->dano_dado = 4;
            break;
    }
    jogador->hp_atual = jogador->hp_max;
}

int rolar_dado(int lados) {
    if (lados <= 0) return 1;
    return (rand() % lados) + 1;
}

struct Jogador jogador1;
struct Jogador jogador2;
SOCKET socket_jogador1;
SOCKET socket_jogador2;
int vez_do_jogador = 1;
int jogadores_prontos = 0; 
pthread_mutex_t trava_do_jogo;

void enviar_status_ac(SOCKET s1, SOCKET s2, struct Jogador* p1, struct Jogador* p2) {
    char buffer_envio[TAMANHO_BUFFER];
    sprintf(buffer_envio, "STATUS_AC;%d;%d;%d;%d\n", p1->defesa_base, p1->bonus_ac_temp, p2->defesa_base, p2->bonus_ac_temp);
    send(s1, buffer_envio, strlen(buffer_envio), 0);
    sprintf(buffer_envio, "STATUS_AC;%d;%d;%d;%d\n", p2->defesa_base, p2->bonus_ac_temp, p1->defesa_base, p1->bonus_ac_temp);
    send(s2, buffer_envio, strlen(buffer_envio), 0);
}


void* rotina_do_jogador(void* arg) {
    
    intptr_t meu_id = (intptr_t)arg; 
    seed_thread = (unsigned int)meu_id * GetTickCount(); 
    srand(seed_thread);

    int oponente_id = (meu_id == 1) ? 2 : 1;
    SOCKET meu_socket = (meu_id == 1) ? socket_jogador1 : socket_jogador2;
    SOCKET oponente_socket = (meu_id == 1) ? socket_jogador2 : socket_jogador1;
    
    char buffer_envio[TAMANHO_BUFFER];
    char buffer_recebimento[TAMANHO_BUFFER];
    int bytes_recebidos;

    printf("Thread (pthread) do Jogador %d iniciada.\n", (int)meu_id);
    
    sprintf(buffer_envio, "TITULO;Batalha de RPG em C\n");
    send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
    
    bytes_recebidos = recv(meu_socket, buffer_recebimento, TAMANHO_BUFFER, 0);
    if (bytes_recebidos <= 0 || strncmp(buffer_recebimento, "PRONTO", 6) != 0) {
        printf("Jogador %d falhou no handshake do título ou desconectou.\n", (int)meu_id);
        pthread_mutex_lock(&trava_do_jogo);
        vez_do_jogador = -1; 
        jogadores_prontos = 2; 
        pthread_mutex_unlock(&trava_do_jogo);
        closesocket(meu_socket);
        return (void*)1;
    }
    printf("Jogador %d pronto.\n", (int)meu_id);

    int classe_escolhida = 0;
    int classe_valida = 0; 
    char* mensagem_de_escolha = "ESCOLHA;Escolha sua classe:";

    while (classe_valida == 0) {
        sprintf(buffer_envio, "%s\n", mensagem_de_escolha);
        send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
        bytes_recebidos = recv(meu_socket, buffer_recebimento, TAMANHO_BUFFER, 0);
    
        if (bytes_recebidos <= 0) {
            printf("Jogador %d desconectou durante a selecao.\n", (int)meu_id);
            pthread_mutex_lock(&trava_do_jogo);
            vez_do_jogador = -1; 
            jogadores_prontos = 2; 
            pthread_mutex_unlock(&trava_do_jogo);
            closesocket(meu_socket);
            return (void*)1;
        }
    
        buffer_recebimento[bytes_recebidos] = '\0';
        classe_escolhida = atoi(buffer_recebimento); 

        switch (classe_escolhida) {
            case 1: case 2: case 3: case 4:
                classe_valida = 1; 
                break;
            default:
                mensagem_de_escolha = "ESCOLHA;Escolha invalida.";
        }
    }

    pthread_mutex_lock(&trava_do_jogo);
    if (meu_id == 1) {
        inicializar_jogador(&jogador1, classe_escolhida);
    } else {
        inicializar_jogador(&jogador2, classe_escolhida);
    }
    jogadores_prontos++; 
    pthread_mutex_unlock(&trava_do_jogo); 

    while (jogadores_prontos < 2) {
        Sleep(100); 
        if (vez_do_jogador == -1) {
            closesocket(meu_socket);
            return (void*)1; 
        }
    }

    struct Jogador *meu_jogador = (meu_id == 1) ? &jogador1 : &jogador2;
    struct Jogador *oponente_jogador = (meu_id == 1) ? &jogador2 : &jogador1;

    sprintf(buffer_envio, "CLASSES;%d;%d;%d;%d;%d;%d\n", 
        meu_jogador->id_classe, oponente_jogador->id_classe, 
        meu_jogador->hp_max, oponente_jogador->hp_max, 
        meu_jogador->defesa_base, oponente_jogador->defesa_base);
    send(meu_socket, buffer_envio, strlen(buffer_envio), 0);

    sprintf(buffer_envio, "STATUS;%d;%d\n", meu_jogador->hp_atual, oponente_jogador->hp_atual);
    send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
    
    enviar_status_ac(socket_jogador1, socket_jogador2, &jogador1, &jogador2);

    pthread_mutex_lock(&trava_do_jogo);
    if (vez_do_jogador == meu_id) {
        sprintf(buffer_envio, "TURNO;Sua vez de jogar!\n");
        send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
    } else {
        sprintf(buffer_envio, "TEXTO;Aguarde o oponente...\n");
        send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
    }
    pthread_mutex_unlock(&trava_do_jogo);

    while (1) {
        bytes_recebidos = recv(meu_socket, buffer_recebimento, TAMANHO_BUFFER, 0);
        pthread_mutex_lock(&trava_do_jogo);

        if (bytes_recebidos <= 0) {
            printf("Jogador %d desconectou.\n", (int)meu_id);
            vez_do_jogador = -1; 
            sprintf(buffer_envio, "FIM;O oponente desconectou. Voce venceu!\n");
            send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
            pthread_mutex_unlock(&trava_do_jogo); 
            break; 
        }
        
        if (vez_do_jogador != meu_id) {
            sprintf(buffer_envio, "TEXTO;Nao e sua vez de jogar!\n");
            send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
            pthread_mutex_unlock(&trava_do_jogo);
            continue; 
        }
        
        buffer_recebimento[bytes_recebidos] = '\0';
        char acao = buffer_recebimento[0]; 

        struct Jogador *atacante = (meu_id == 1) ? &jogador1 : &jogador2;
        struct Jogador *defensor = (meu_id == 1) ? &jogador2 : &jogador1;

        printf("Jogador %d (%d) usou a acao: %c\n", (int)meu_id, atacante->id_classe, acao);
        
        int jogo_terminou = 0; 
        int acao_foi_processada = 0; 
        int bonus_ac_resetado = 0; 

        switch (acao) {
            case 'A': 
            {
                atacante->esta_defendendo = 0;
                atacante->esta_esquivando = 0;
                atacante->bonus_ac_temp = 0; 
                bonus_ac_resetado = 1; 

                int defesa_real_oponente = defensor->defesa_base + defensor->bonus_ac_temp;
                int esquiva_ativa = defensor->esta_esquivando; 
                
                defensor->esta_esquivando = 0; 
                defensor->bonus_ac_temp = 0; 

                int bonus_ataque = 0;
                if (atacante->id_classe == 4) { 
                    bonus_ataque = 5;
                }

                int rolagem_base = rolar_dado(20);
                int rolagem_ataque = rolagem_base + bonus_ataque;

                if (rolagem_ataque > defesa_real_oponente) {
                    // ACERTOU!
                    int dano_base = rolar_dado(atacante->dano_dado);
                    int dano_bonus = 0; 
                    if (atacante->id_classe == 2) { 
                        dano_bonus = rolar_dado(4); 
                    }
                    int dano_total = dano_base + dano_bonus;

                    if (defensor->esta_defendendo) {
                        if (defensor->id_classe == 1) { 
                            dano_total = 0; 
                            sprintf(buffer_envio, "TEXTO;O Cavaleiro bloqueou seu ataque! Causou 0 de dano.\n");
                            send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                            sprintf(buffer_envio, "TEXTO;Voce bloqueou TODO o dano com o escudo!\n");
                            send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                        } else { 
                            dano_total = dano_total / 2; 
                            sprintf(buffer_envio, "TEXTO;O oponente defendeu! Causou %d de dano. (-50%%)\n", dano_total);
                            send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                            sprintf(buffer_envio, "TEXTO;Voce se defendeu! Tomou %d de dano. (-50%%)\n", dano_total);
                            send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                        }
                    } else {
                        sprintf(buffer_envio, "TEXTO;Voce ataca! (Rolou %d+%d=%d). ACERTOU e causou %d de dano!\n", rolagem_base, bonus_ataque, rolagem_ataque, dano_total);
                        send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                        sprintf(buffer_envio, "TEXTO;Oponente ataca! (Rolou %d). ACERTOU e voce tomou %d de dano!\n", rolagem_ataque, dano_total);
                        send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                    }

                    defensor->esta_defendendo = 0; 
                    defensor->hp_atual -= dano_total;
                
                } else {
                    // ERROU!
                    defensor->esta_defendendo = 0; 
                    
                    if (esquiva_ativa) {
                        sprintf(buffer_envio, "TEXTO;Voce ataca! (Rolou %d+%d=%d). O oponente ESQUIVOU!\n", rolagem_base, bonus_ataque, rolagem_ataque);
                        send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                        sprintf(buffer_envio, "TEXTO;Oponente ataca! (Rolou %d). Voce ESQUIVOU!\n", rolagem_ataque);
                        send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                    } else {
                        sprintf(buffer_envio, "TEXTO;Voce ataca! (Rolou %d+%d=%d). ERROU!\n", rolagem_base, bonus_ataque, rolagem_ataque);
                        send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                        sprintf(buffer_envio, "TEXTO;Oponente ataca! (Rolou %d). ERROU!\n", rolagem_ataque);
                        send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                    }
                }
                
                if (defensor->hp_atual <= 0) {
                    jogo_terminou = 1;
                }
                acao_foi_processada = 1; 
                break;
            } 
            
            case 'D':
            {
                atacante->esta_defendendo = 1;
                atacante->esta_esquivando = 0;
                atacante->bonus_ac_temp = 0; 
                bonus_ac_resetado = 1; 
                
                if (atacante->id_classe == 1) { // Cavaleiro
                    sprintf(buffer_envio, "TEXTO;BLOQUEIO TOTAL! (Dano 100%% reduzido no proximo turno)\n");
                    send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                    sprintf(buffer_envio, "TEXTO;O Cavaleiro levanta o escudo em postura defensiva!\n");
                    send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                } else { // Outros
                    sprintf(buffer_envio, "TEXTO;Voce se prepara para defender (Dano -50%%)...\n");
                    send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                    sprintf(buffer_envio, "TEXTO;Oponente assume uma postura defensiva...\n");
                    send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                }
                acao_foi_processada = 1;
                break;
            }
            
            case 'E':
            {
                atacante->esta_defendendo = 0;
                atacante->esta_esquivando = 1;
                
                if (atacante->id_classe == 3) { // Arqueiro
                    atacante->bonus_ac_temp = 6;
                    sprintf(buffer_envio, "TEXTO;Voce se prepara para esquivar (AC +6)!\n");
                    send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                    sprintf(buffer_envio, "TEXTO;O Arqueiro se prepara para esquivar!\n");
                    send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                } else { // Padrão
                    atacante->bonus_ac_temp = 3;
                    sprintf(buffer_envio, "TEXTO;Voce se prepara para esquivar (AC +3)...\n");
                    send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                    sprintf(buffer_envio, "TEXTO;Oponente se prepara para esquivar...\n");
                    send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
                }
                
                enviar_status_ac(socket_jogador1, socket_jogador2, &jogador1, &jogador2);
                
                acao_foi_processada = 1;
                break;
            }
            default:
                printf("Jogador %d enviou acao invalida: %c (%d)\n", (int)meu_id, acao, acao);
                sprintf(buffer_envio, "TEXTO;Acao invalida '%c'. Voce perdeu o turno.\n", acao);
                send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
                acao_foi_processada = 1; 
                break;
        } 
        
        sprintf(buffer_envio, "STATUS;%d;%d\n", atacante->hp_atual, defensor->hp_atual);
        send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
        sprintf(buffer_envio, "STATUS;%d;%d\n", defensor->hp_atual, atacante->hp_atual);
        send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);

        if (bonus_ac_resetado) {
            enviar_status_ac(socket_jogador1, socket_jogador2, &jogador1, &jogador2);
        }

        if (jogo_terminou) {
            vez_do_jogador = -1; 
            sprintf(buffer_envio, "FIM;Voce VENCEU! O oponente foi derrotado.\n");
            send(meu_socket, buffer_envio, strlen(buffer_envio), 0);
            sprintf(buffer_envio, "FIM;Voce foi DERROTADO!\n");
            send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
        } else if (acao_foi_processada) {
            vez_do_jogador = oponente_id;
            sprintf(buffer_envio, "TURNO;Sua vez de jogar!\n");
            send(oponente_socket, buffer_envio, strlen(buffer_envio), 0);
        }
                     
        pthread_mutex_unlock(&trava_do_jogo); 
        
        if (vez_do_jogador == -1) break; 
    } 
    
    closesocket(meu_socket);
    return (void*)0; 
}

int main() {
    WSADATA wsaData;
    int resultado = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (resultado != 0) { printf("WSAStartup falhou! Erro: %d\n", resultado); getch(); return 1; }
    printf("Winsock inicializado.\n");

    SOCKET socket_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_listen == INVALID_SOCKET) { printf("Criacao do socket falhou! Erro: %d\n", WSAGetLastError()); WSACleanup(); getch(); return 1; }
    printf("Socket do servidor criado.\n");

    struct sockaddr_in endereco_servidor;
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_addr.s_addr = INADDR_ANY;
    endereco_servidor.sin_port = htons(PORTA);

    if (bind(socket_listen, (struct sockaddr*)&endereco_servidor, sizeof(endereco_servidor)) == SOCKET_ERROR) { printf("Bind falhou! Erro: %d\n", WSAGetLastError()); closesocket(socket_listen); WSACleanup(); getch(); return 1; }
    printf("Socket vinculado a porta %d.\n", PORTA);

    if (listen(socket_listen, 2) == SOCKET_ERROR) { printf("Listen falhou! Erro: %d\n", WSAGetLastError()); closesocket(socket_listen); WSACleanup(); getch(); return 1; }

    if (pthread_mutex_init(&trava_do_jogo, NULL) != 0) { printf("Falha ao criar o mutex (pthread)!\n"); getch(); return 1; }
    printf("Mutex (pthread) inicializado.\n");

    printf("Aguardando Jogador 1 conectar...\n");
    socket_jogador1 = accept(socket_listen, NULL, NULL);
    if (socket_jogador1 == INVALID_SOCKET) { printf("Accept falhou! Erro: %d\n", WSAGetLastError()); closesocket(socket_listen); WSACleanup(); getch(); return 1; }
    printf("Jogador 1 conectado!\n");

    printf("Aguardando Jogador 2 conectar...\n");
    socket_jogador2 = accept(socket_listen, NULL, NULL);
    if (socket_jogador2 == INVALID_SOCKET) { printf("Accept falhou! Erro: %d\n", WSAGetLastError()); closesocket(socket_listen); WSACleanup(); getch(); return 1; }
    printf("Jogador 2 conectado!\n");

    closesocket(socket_listen);
    printf("Lobby cheio. Iniciando o jogo...\n");

    pthread_t thread1, thread2;

    if (pthread_create(&thread1, NULL, rotina_do_jogador, (void*)(intptr_t)1) != 0) { printf("Falha ao criar thread 1 (pthread)!\n"); getch(); return 1; }
    if (pthread_create(&thread2, NULL, rotina_do_jogador, (void*)(intptr_t)2) != 0) { printf("Falha ao criar thread 2 (pthread)!\n"); getch(); return 1; }
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("Jogo terminado.\n");

    pthread_mutex_destroy(&trava_do_jogo);
    WSACleanup();
    
    printf("\nPressione QUALQUER TECLA para sair...");
    getch();
    return 0;
}
