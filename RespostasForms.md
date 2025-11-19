Respostas do Formulário:

ESOFT4S-D Alunos: Vinicius Dumont, Guilherme Sousa da Silva, João Emmanuel Nadur Ariza

QUAL O NOME DO SEU JOGO?

- BattleC


QUAL A LINGUAGEM FOI UTILIZADA?

- C


COMO E QUAIS INFORMAÇÕES SÃO TROCADAS ENTRE OS PROCESSOS?

- A comunicação entre os processos é implementada através de sockets TCP, utilizando a biblioteca winsock2.h, onde um processo servidor central gerencia as conexões de dois processos clientes. As informações são trocadas por meio de um protocolo de mensagens customizado, baseado em strings de texto. Cada mensagem é terminada com um caractere de nova linha (\n), o que permite ao cliente processar o fluxo (streaming) de dados corretamente, mesmo que múltiplas mensagens cheguem "coladas" em um único pacote TCP.  O fluxo de comunicação começa com o servidor enviando uma mensagem TITULO para o cliente exibir a tela inicial. O cliente responde com PRONTO assim que o jogador pressiona [ENTER]. Em seguida, o servidor envia o comando ESCOLHA, e o cliente responde com o ID da classe escolhida. Com as classes definidas, o servidor envia a mensagem CLASSES, que contém todos os stats iniciais (HP Máximo, Defesa Base) de ambos os jogadores. Durante o combate, o servidor gerencia o fluxo enviando TURNO ao jogador ativo. O cliente responde com uma ação (ex: A, D ou E), e o servidor processa a lógica, devolvendo mensagens TEXTO (para logs de combate), STATUS (para atualizar o HP) e STATUS_AC (para atualizar bônus de esquiva). A partida termina quando o servidor envia uma mensagem FIM.


COMO É FEITO O GERENCIAMENTO DA EXCLUSÃO MÚTUA?


- O gerenciamento da exclusão mútua é uma função crítica do processo servidor, implementada utilizando a biblioteca pthreads.h. Como o servidor dispara uma thread dedicada para cada jogador, e ambas as threads precisam acessar e modificar o mesmo conjunto de variáveis globais que definem o estado do jogo (especificamente, as estruturas jogador1, jogador2 e a variável vez_do_jogador), a proteção é essencial para evitar condições de corrida.  Para isso, uma trava global do tipo pthread_mutex_t é inicializada. Em qualquer seção crítica do código (por exemplo, quando um jogador ataca, defende ou quando o servidor precisa ler o HP do oponente para calcular o dano) a thread ativa deve primeiro adquirir a trava usando pthread_mutex_lock(). Somente após obter o bloqueio, ela pode ler ou modificar as variáveis globais com segurança. Ao concluir a operação, a thread libera a trava imediatamente com pthread_mutex_unlock(), permitindo que a outra thread (do outro jogador) possa executar sua própria seção crítica. Esse mecanismo garante que as ações do jogo sejam atômicas e que a lógica de turnos seja respeitada, impedindo que os dois jogadores modifiquem o estado do jogo simultaneamente.


