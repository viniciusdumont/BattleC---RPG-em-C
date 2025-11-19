#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define IP_SERVIDOR "127.0.0.1"
#define PORTA 8080
#define TAMANHO_BUFFER 512

int minha_classe = 0;
int classe_oponente = 0;
int meu_hp_max = 1;
int oponente_hp_max = 1;
int minha_defesa_base = 10;
int oponente_defesa_base = 10;
int meu_ac_bonus = 0;
int oponente_ac_bonus = 0;


void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// --- MUDANÇA: Tamanho da Janela ---
void configurar_janela() {
    system("mode con: cols=80 lines=28"); // Mais estreito e mais baixo
    HWND consoleWindow = GetConsoleWindow();
    LONG estilo = GetWindowLong(consoleWindow, GWL_STYLE);
    estilo = estilo & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX;
    SetWindowLong(consoleWindow, GWL_STYLE, estilo);
}

void esconder_cursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;    // O tamanho não importa muito, mas...
    info.bVisible = FALSE; // A mágica acontece aqui!
    SetConsoleCursorInfo(consoleHandle, &info);
}

void desenhar_boneco(int classe_id, int x, int y) {
    gotoxy(x, y);
    if (classe_id == 1) { // Cavaleiro
        printf("      o  /       "); 
        gotoxy(x, y + 1); printf("    ()|\\/       "); 
        gotoxy(x, y + 2); printf("     / \\        "); 
    } else if (classe_id == 2) { // Mago
        printf("      o +         "); 
        gotoxy(x, y + 1); printf("     /|\\|         "); 
        gotoxy(x, y + 2); printf("     / \\|         "); 
    } else if (classe_id == 3) { // Arqueiro
        printf("       'o        "); 
        gotoxy(x, y + 1); printf("       /|\\|)      "); 
        gotoxy(x, y + 2); printf("       / \\        "); 
    } else if (classe_id == 4) { // Bárbaro
        printf("       o(|)       ");
        gotoxy(x, y + 1); printf("      /|\\|      ");
        gotoxy(x, y + 2); printf("      / \\* ");
    } else { 
        printf("      ( ? )      ");
        gotoxy(x, y + 1); printf("      / | \\      ");
        gotoxy(x, y + 2); printf("      / \\        ");
    }
}

void criar_barra_hp(char* buffer_barra, int hp_atual, int hp_max, int tamanho_barra) {
    if (hp_atual < 0) hp_atual = 0;
    if (hp_max <= 0) hp_max = 1; 
    
    float porcentagem = (float)hp_atual / (float)hp_max;
    int blocos_cheios = (int)(porcentagem * tamanho_barra);
    
    int i = 0;
    buffer_barra[i++] = '[';
    for (int j = 0; j < tamanho_barra; j++) {
        if (j < blocos_cheios) { buffer_barra[i++] = '#'; }
        else { buffer_barra[i++] = ' '; }
    }
    buffer_barra[i++] = ']';
    
    char texto_hp[15];
    sprintf(texto_hp, " %d/%d", hp_atual, hp_max);
    strcpy(&buffer_barra[i], texto_hp);
}

void desenhar_icone_classe(int classe_id) {
    // Ponto de início (y=2) para caber na tela de 28 linhas
    int y_topo = 2;
    
    switch (classe_id) {
        case 1: // Cavaleiro (Espada)
            gotoxy(15, y_topo+6); printf("        />__________________________________");
            gotoxy(15, y_topo+7); printf("[########[]_________________________________/");
            gotoxy(15, y_topo+8); printf("        \\>                                 "); // Escape na barra
            break;
            
        case 2: // Mago (Livro Aberto)
            gotoxy(17, y_topo+3); printf("         ,..........   ...........,");
            gotoxy(17, y_topo+4); printf("      ,..,'         '.'         ',..,");
            gotoxy(17, y_topo+5); printf("     ,' ,'           :           ', ',");
            gotoxy(17, y_topo+6); printf("   ,' ,'             :             ', ',");
            gotoxy(17, y_topo+7); printf("   ,' ,'             :             ', ',");
            gotoxy(17, y_topo+8); printf(" ,' ,'............., : ,.............', ',");
            gotoxy(17, y_topo+9); printf(",'  '............   '.'   ............'  ',");
            gotoxy(17, y_topo+10); printf(" '''''''''''''''''';''';''''''''''''''''''");
            gotoxy(17, y_topo+11); printf("                    '''");
            break;
            
        case 3: // Arqueiro (Arco)
            gotoxy(21, y_topo+7); printf(">>>>>>>_____________________\\`-._"); // Escape na barra
            gotoxy(21, y_topo+8); printf(">>>>>>>                     /.-'");
            break;
            
        case 4: // Bárbaro (Machado)
            gotoxy(33, y_topo+2); printf("  ,  /\\  .  "); // Escape na barra
            gotoxy(33, y_topo+3); printf(" //`-||-'\\\\ "); // Escape nas barras
            gotoxy(33, y_topo+4); printf("(| -=||=- |)");
            gotoxy(33, y_topo+5); printf(" \\\\,-||-.// "); // Escape nas barras
            gotoxy(33, y_topo+6); printf("  `  ||  '  ");
            gotoxy(33, y_topo+7); printf("     ||     ");
            gotoxy(33, y_topo+8); printf("     ||     ");
            gotoxy(33, y_topo+9); printf("     ||     ");
            gotoxy(33, y_topo+10); printf("     ||     ");
            gotoxy(33, y_topo+11); printf("     ||     ");
            gotoxy(33, y_topo+12);printf("     ()     ");
            break;
            
        default:
            desenhar_boneco(0, 21, 5); 
            break;
    }
}

void desenhar_tela_de_espera(int classe_que_eu_escolhi) {
    system("cls");
    
    // 1. Desenha o ÍCONE da classe (em vez do boneco)
    desenhar_icone_classe(classe_que_eu_escolhi); 
    
    // 2. Prepara o texto
    char* titulo;
    char* flavortext; 

    switch (classe_que_eu_escolhi) {
        case 1:
            titulo = "CAVALEIRO"; 
            flavortext = "Polindo a armadura... Aguardando oponente."; 
            break;
        case 2:
            titulo = "MAGO"; 
            flavortext = "Folheando seu grimorio... Aguardando oponente."; 
            break;
        case 3:
            titulo = "ARQUEIRO"; 
            flavortext = "Afiando suas flechas... Aguardando oponente."; 
            break;
        case 4:
            titulo = "BARBARO"; 
            flavortext = "Amolando seu machado... Aguardando oponente."; 
            break;
        default:
            titulo = "Voce escolheu sua classe."; 
            flavortext = "Aguardando o outro jogador..."; 
            break;
    }

    // 3. Desenha a caixa (Ajustada para ficar EMBAIXO da arte)
    int caixa_x = 5; 
    gotoxy(caixa_x, 18); printf("        +----------------------------------------------------+");
    gotoxy(caixa_x, 19); printf("        |                                                    |");
    gotoxy(caixa_x, 20); printf("        |                                                    |");
    gotoxy(caixa_x, 21); printf("        |                                                    |");
    gotoxy(caixa_x, 22); printf("        +----------------------------------------------------+");

    // 4. Desenha o texto centralizado DENTRO da caixa
    int titulo_x = caixa_x + 1 + (65 - strlen(titulo)) / 2;
    int flavor_x = caixa_x + 1 + (68 - strlen(flavortext)) / 2;

    gotoxy(titulo_x, 19);
    printf("%s", titulo);
    
    gotoxy(flavor_x, 21);
    printf("%s", flavortext);
    
    gotoxy(0, 27);
}

void desenhar_ac(int x, int y, char* titulo, int base_ac, int bonus_ac) {
    gotoxy(x, y);
    printf("                                "); 
    gotoxy(x, y);
    printf("%s (AC: %d", titulo, base_ac);
    if (bonus_ac > 0) {
        printf(" +%d", bonus_ac);
    }
    printf(")");
}

// --- MUDANÇA: Arena "Espremida" ---
void desenhar_arena() {
    system("cls");
    
    char barra_oponente_inicial[50];
    char barra_jogador_inicial[50];
    
    criar_barra_hp(barra_oponente_inicial, oponente_hp_max, oponente_hp_max, 20);
    criar_barra_hp(barra_jogador_inicial, meu_hp_max, meu_hp_max, 20);

    // Info Oponente (Movido para x=43)
    desenhar_boneco(classe_oponente, 43, 5);
    desenhar_ac(40, 9, "OPONENTE", oponente_defesa_base, oponente_ac_bonus);
    gotoxy(40, 10); printf("HP: %s", barra_oponente_inicial);

    // Info Jogador (Mantido em x=8)
    desenhar_boneco(minha_classe, 8, 14);
    desenhar_ac(5, 18, "JOGADOR ", minha_defesa_base, meu_ac_bonus);
    gotoxy(5, 19); printf("HP: %s", barra_jogador_inicial);

    // Linhas encurtadas (60 '=' de largura)
    gotoxy(5, 21); printf("======================================================================");
    gotoxy(5, 22); printf("| Acoes:                                                               ");
    gotoxy(5, 23); printf("| Log:                                                                 ");
    gotoxy(5, 24); printf("======================================================================");
    gotoxy(0, 27); // Posição final
}


void processar_mensagem(char* buffer_original, SOCKET socket) {
    char buffer[TAMANHO_BUFFER];
    strcpy(buffer, buffer_original);

    char* codigo = strtok(buffer, ";");
    char* mensagem = strtok(NULL, "\0");

    if (codigo == NULL) {
        gotoxy(12, 23); printf("                                                                    ");
        gotoxy(12, 23); printf("%s", buffer_original);
        gotoxy(0, 27); return;
    }

    if (strcmp(codigo, "TEXTO") == 0) {
        gotoxy(12, 23); printf("                                                                    ");
        gotoxy(12, 23); printf("%s", mensagem);
    }
    else if (strcmp(codigo, "TITULO") == 0) {
        system("cls");
        
        gotoxy(14, 5); printf("        ____        _   _   _       _____\n");
        gotoxy(14, 6); printf("       |  _ \\      | | | | | |     / ____|\n");
        gotoxy(14, 7); printf("       | |_) | __ _| |_| |_| | ___| |    \n");
        gotoxy(14, 8); printf("       |  _ < / _` | __| __| |/ _ \\ |    \n");
        gotoxy(14, 9); printf("       | |_) | (_| | |_| |_| |  __/ |____ \n");
        gotoxy(14, 10);printf("       |____/ \\__,_|\\__|\\__|_|\\___|\\_____|\n");
        
        gotoxy(23, 13); printf("   === %s ===\n", mensagem); 
        
        gotoxy(20, 16); printf("   Pressione [ENTER] para iniciar...");
        
        gotoxy(8, 26); printf("Vinicius Dumont            Joao Ariza            Guilherme Sousa");
        
        while (getch() != 13) {}
        while (_kbhit()) { getch(); }
        
        send(socket, "PRONTO\n", 7, 0);
    }
    // --- MUDANÇA: Menu 2x2 "Espremido" ---
    else if (strcmp(codigo, "ESCOLHA") == 0) {
        char escolha_char;
        char escolha_string[2];
        int classe_id_numerico = 0;

        system("cls");
        
        gotoxy(19, 1); // Centralizado para 70
        printf("=== %s ===\n", mensagem);

        // --- (1) Cavaleiro (X=5) ---
        gotoxy(5, 3); printf("+------------------+");
        gotoxy(5, 4); printf("|  (1) CAVALEIRO   |");
        gotoxy(5, 5); printf("|------------------|");
        gotoxy(5, 6); printf("|        o  /      |"); 
        gotoxy(5, 7); printf("|      ()|\\/       |"); 
        gotoxy(5, 8); printf("|       / \\        |"); 
        gotoxy(5, 9); printf("|------------------|");
        gotoxy(5, 10);printf("| HP: 30 / DEF: 14 |");
        gotoxy(5, 11);printf("+------------------+");
        
        // --- (2) Mago (X=40) ---
        gotoxy(40, 3); printf("+------------------+");
        gotoxy(40, 4); printf("|     (2) MAGO     |");
        gotoxy(40, 5); printf("|------------------|");
        gotoxy(40, 6); printf("|        o +       |"); 
        gotoxy(40, 7); printf("|       /|\\|       |"); 
        gotoxy(40, 8); printf("|       / \\|       |"); 
        gotoxy(40, 9); printf("|------------------|");
        gotoxy(40, 10);printf("| HP: 20 / DEF: 11 |");
        gotoxy(40, 11);printf("+------------------+");

        // --- (3) Arqueiro (X=5) ---
        gotoxy(5, 13); printf("+------------------+");
        gotoxy(5, 14); printf("|   (3) ARQUEIRO   |");
        gotoxy(5, 15); printf("|------------------|");
        gotoxy(5, 16); printf("|       'o         |"); 
        gotoxy(5, 17); printf("|       /|\\|)      |"); 
        gotoxy(5, 18); printf("|       / \\        |"); 
        gotoxy(5, 19); printf("|------------------|");
        gotoxy(5, 20);printf("| HP: 25 / DEF: 12 |");
        gotoxy(5, 21);printf("+------------------+");
        
        // --- (4) Bárbaro (X=40) ---
        gotoxy(40, 13); printf("+------------------+");
        gotoxy(40, 14); printf("|    (4) BARBARO   |");
        gotoxy(40, 15); printf("|------------------|");
        gotoxy(40, 16); printf("|        o(|)      |");
        gotoxy(40, 17); printf("|       /|\\|       |");
        gotoxy(40, 18); printf("|       / \\*       |");
        gotoxy(40, 19); printf("|------------------|");
        gotoxy(40, 20);printf("| HP: 35 / DEF: 11 |");
        gotoxy(40, 21);printf("+------------------+");

        // --- Pede a escolha (x=5) ---
        gotoxy(5, 23);
        printf("======================================================="); // 55 '='
        gotoxy(5, 24);
        printf("Sua escolha: ");
        
        escolha_char = getch(); 
        while (_kbhit()) { getch(); }
        
        escolha_string[0] = escolha_char;
        escolha_string[1] = '\0';
        send(socket, escolha_string, strlen(escolha_string), 0);
        
        classe_id_numerico = atoi(escolha_string);
        if (classe_id_numerico < 1 || classe_id_numerico > 4) {
            classe_id_numerico = 0; 
        }
        desenhar_tela_de_espera(classe_id_numerico);
    }
    else if (strcmp(codigo, "CLASSES") == 0) {
        minha_classe = atoi(strtok(mensagem, ";"));
        classe_oponente = atoi(strtok(NULL, ";"));
        meu_hp_max = atoi(strtok(NULL, ";"));
        oponente_hp_max = atoi(strtok(NULL, ";"));
        minha_defesa_base = atoi(strtok(NULL, ";"));
        oponente_defesa_base = atoi(strtok(NULL, ";"));
        
        desenhar_arena();
    }
     else if (strcmp(codigo, "STATUS") == 0) {
        int meu_hp_atual = atoi(strtok(mensagem, ";"));
        int oponente_hp_atual = atoi(strtok(NULL, ";"));
        
        char barra_meu_hp[50];
        char barra_oponente_hp[50];
        
        criar_barra_hp(barra_meu_hp, meu_hp_atual, meu_hp_max, 20);
        criar_barra_hp(barra_oponente_hp, oponente_hp_atual, oponente_hp_max, 20);

        gotoxy(40, 10); printf("                                       ");
        gotoxy(40, 10); printf("HP: %s", barra_oponente_hp);

        gotoxy(5, 19); printf("                                       ");
        gotoxy(5, 19); printf("HP: %s", barra_meu_hp);
    }
    else if (strcmp(codigo, "STATUS_AC") == 0) {
        minha_defesa_base = atoi(strtok(mensagem, ";"));
        meu_ac_bonus = atoi(strtok(NULL, ";"));
        oponente_defesa_base = atoi(strtok(NULL, ";"));
        oponente_ac_bonus = atoi(strtok(NULL, ";"));
        
        desenhar_ac(40, 9, "OPONENTE", oponente_defesa_base, oponente_ac_bonus);
        desenhar_ac(5, 18, "JOGADOR ", minha_defesa_base, meu_ac_bonus);
    }
    else if (strcmp(codigo, "TURNO") == 0) {
        char jogada_char;
        char jogada_string[2];

        gotoxy(5, 26); printf("                                                          "); // Y=26
        gotoxy(14, 22); printf("                                                                  ");
        gotoxy(12, 23); printf("                                                                    ");

        gotoxy(14, 22); printf("[A]tacar    [D]efender    [E]squivar");
        gotoxy(12, 23); printf("%s", mensagem);

        while (1) {
            jogada_char = getch(); 
            jogada_char = toupper(jogada_char);
            while (_kbhit()) { getch(); }
            
            if (jogada_char == 'A' || jogada_char == 'D' || jogada_char == 'E') {
                break; 
            } else {
                gotoxy(5, 26); printf("!!! Tecla invalida. Use [A], [D] ou [E] !!!"); // Y=26
                Beep(200, 150);
            }
        }

        gotoxy(5, 26); printf("                                                          "); // Y=26

        jogada_string[0] = jogada_char;
        jogada_string[1] = '\0';
        send(socket, jogada_string, strlen(jogada_string), 0);

        gotoxy(14, 22); printf("                                                                  ");
    } 
    else if (strcmp(codigo, "FIM") == 0) {
        gotoxy(12, 23); printf("                                                                    ");
        gotoxy(12, 23); printf("!!! %s !!!", mensagem);
    }
    
    gotoxy(0, 27); // Posição final
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { printf("WSAStartup falhou!\n"); getch(); return 1; }
    
    configurar_janela();
    
    esconder_cursor();

    SOCKET socket_conexao = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_conexao == INVALID_SOCKET) { printf("Criacao do socket falhou!\n"); WSACleanup(); getch(); return 1; }
    
    struct sockaddr_in endereco_servidor;
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_addr.s_addr = inet_addr(IP_SERVIDOR);
    endereco_servidor.sin_port = htons(PORTA);

    printf("Conectando ao servidor %s na porta %d...\n", IP_SERVIDOR, PORTA);
    if (connect(socket_conexao, (struct sockaddr*)&endereco_servidor, sizeof(endereco_servidor)) == SOCKET_ERROR) { printf("Connect falhou! O servidor esta rodando?\n"); closesocket(socket_conexao); WSACleanup(); getch(); return 1; }
    printf("Conectado! Aguardando o jogo comecar...\n\n");

    static char buffer_acumulado[TAMANHO_BUFFER * 2] = {0}; 
    char buffer_recebimento_temp[TAMANHO_BUFFER];        
    int bytes_recebidos;
    int jogo_acabou = 0; 

    while (!jogo_acabou) {
        bytes_recebidos = recv(socket_conexao, buffer_recebimento_temp, TAMANHO_BUFFER - 1, 0);

        if (bytes_recebidos <= 0) { printf("Servidor desconectou.\n"); break; }

        buffer_recebimento_temp[bytes_recebidos] = '\0';
        strcat(buffer_acumulado, buffer_recebimento_temp);

        char* proxima_mensagem_ptr = buffer_acumulado;
        char* fim_da_mensagem = NULL;

        while ((fim_da_mensagem = strchr(proxima_mensagem_ptr, '\n')) != NULL) {
            *fim_da_mensagem = '\0';
            
            char mensagem_atual[TAMANHO_BUFFER];
            strcpy(mensagem_atual, proxima_mensagem_ptr); 
            
            processar_mensagem(mensagem_atual, socket_conexao);

            if (strncmp(mensagem_atual, "FIM;", 4) == 0) {
                jogo_acabou = 1;
                break; 
            }
            
            proxima_mensagem_ptr = fim_da_mensagem + 1;
        }
        
        if (jogo_acabou) break;

        if (strlen(proxima_mensagem_ptr) > 0) {
            strcpy(buffer_acumulado, proxima_mensagem_ptr);
        } else {
            buffer_acumulado[0] = '\0';
        }
    }

    closesocket(socket_conexao);
    WSACleanup();
    
    printf("\nPressione QUALQUER TECLA para sair...");
    getch();
    return 0;
}
