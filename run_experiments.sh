#!/bin/bash

IP="127.0.0.1"
PORT=8080
RODADAS=5
MAX_CLIENTES=10
LOG_DIR="build/bench_logs"
CSV_FILE="build/resultados_benchmark.csv"
SERVER_LIVE_LOG="build/server_live.log"

gcc -Wall -Wextra -O2 -pthread src/chat_client.c -o build/chat_client

if [ $? -ne 0 ]; then
    echo "Erro na compilação do cliente!"
    exit 1
fi

# Verifica se o servidor está rodando e gerando o log
if [ ! -f "$SERVER_LIVE_LOG" ]; then
    echo "Erro: Arquivo $SERVER_LIVE_LOG não encontrado."
    echo "Inicie o servidor no outro terminal usando: ./build/chat_server $PORT | tee $SERVER_LIVE_LOG"
    exit 1
fi

echo "Num_Clientes,Rodada,Bytes_Recebidos,Throughput_Servidor_Mbps,Tempo_Total,Vazao_Media_Por_Cliente_Mbps" > "$CSV_FILE"
rm -rf "$LOG_DIR"
mkdir -p "$LOG_DIR"

for c in $(seq 1 $MAX_CLIENTES); do
    echo "=== Testando com $c cliente(s) ==="
    
    for r in $(seq 1 $RODADAS); do
        CURRENT_DIR="$LOG_DIR/c${c}_r${r}"
        mkdir -p "$CURRENT_DIR"

        LINHAS_ANTES=$(grep -c "Recebidos:" "$SERVER_LIVE_LOG" 2>/dev/null || echo 0)
        
        for i in $(seq 0 $((c - 1))); do
            ./build/chat_client $IP $PORT "Cliente_$i" > "$CURRENT_DIR/client_$i.log" 2>&1 &
        done
        
        wait

        while [ "$(grep -c "Recebidos:" "$SERVER_LIVE_LOG" 2>/dev/null || echo 0)" -le "$LINHAS_ANTES" ]; do
            sleep 0.05
        done

        read BYTES TEMPO THROUGHPUT_SERVER <<< "$(grep "Recebidos:" "$SERVER_LIVE_LOG" | tail -n 1 | awk '{printf "%.0f %.3f %.3f", $2, $6, $9}')"
        
        TOTAL_MBPS=$(cat "$CURRENT_DIR"/client_*.log | grep "Taxa:" | awk '{sum += $(NF-1)} END {printf "%.3f", sum}')
        MEDIA_CLIENTE_MBPS=$(awk -v total="$TOTAL_MBPS" -v count="$c" 'BEGIN {printf "%.3f", total / count}')
        
        echo "$c,$r,$BYTES,$THROUGHPUT_SERVER,$TEMPO,$MEDIA_CLIENTE_MBPS" >> "$CSV_FILE"
        echo "  [$r/$RODADAS] Servidor: $THROUGHPUT_SERVER Mbps (${TEMPO}s) | Clientes: $TOTAL_MBPS Mbps (Média: $MEDIA_CLIENTE_MBPS Mbps)"

        sleep 1
    done
    
    echo ""
done

echo "================================================================================"