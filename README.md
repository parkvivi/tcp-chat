# Chat TCP
Atividade da disciplina Redes de Computadores ministrada em 2s2026 no ICT Unifesp.

## Grupo
* [Luiza de Souza Ferreira](https://github.com/souza-luiza) - 170453
* [Viviane Flor Park](https://github.com/parkvivi) - 169259

## Requisitos
* Sistema operacional ou subsistema equivalente baseado em POSIX (Linux/macOS ou WSL)
* GCC

## Compilar e Executar

* Servidor

```bash
gcc -Wall -Wextra -g src/chat_server.c -o build/chat_server -pthread && ./build/chat_server 5000
```

* Cliente

```bash
gcc -Wall -Wextra -g src/chat_client.c -o build/chat_client -pthread && ./build/chat_client 127.0.0.1 5000
```