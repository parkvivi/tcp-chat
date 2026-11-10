#!/bin/bash

IP="127.0.0.1"
PORT=8080
RODADAS=5
MAX_CLIENTES=10
LOG_DIR="build/bench_logs"
CSV_FILE="build/resultados_benchmark.csv"

gcc -Wall -Wextra -O2 -pthread src/chat_client.c -o build/chat_client

if [ $? -ne 0 ]; then
    echo "Erro na compilação do cliente!"
    exit 1
fi

echo "Num_Clientes,Rodada,Vazao_Total_Mbps,Vazao_Media_Por_Cliente_Mbps" > "$CSV_FILE"
rm -rf "$LOG_DIR"
mkdir -p "$LOG_DIR"

for c in $(seq 1 $MAX_CLIENTES); do
    echo "=== Testando com $c cliente(s) ==="
    
    for r in $(seq 1 $RODADAS); do
        CURRENT_DIR="$LOG_DIR/c${c}_r${r}"
        mkdir -p "$CURRENT_DIR"
        
        for i in $(seq 0 $((c - 1))); do
            ./build/chat_client $IP $PORT "Cliente_$i" > "$CURRENT_DIR/client_$i.log" 2>&1 &
        done
        
        wait
        
        TOTAL_MBPS=$(cat "$CURRENT_DIR"/client_*.log | grep "Taxa:" | awk '{sum += $(NF-1)} END {printf "%.3f", sum}')
        MEDIA_CLIENTE_MBPS=$(awk -v total="$TOTAL_MBPS" -v count="$c" 'BEGIN {printf "%.3f", total / count}')
        
        echo "$c,$r,$TOTAL_MBPS,$MEDIA_CLIENTE_MBPS" >> "$CSV_FILE"
        echo "  [$r/$RODADAS] Vazão Total Agregada: $TOTAL_MBPS Mbps | Média por Cliente: $MEDIA_CLIENTE_MBPS Mbps"

        sleep 1
    done
    
    echo ""
done

echo "================================================================================"