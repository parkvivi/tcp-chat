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
gcc -Wall -Wextra -O2 -pthread src/chat_server.c -o build/chat_server && ./build/chat_server 8080
```

* Script de automação dos clientes

```bash
chmod +x run_experiments.sh && ./run_experiments.sh
```
