#!/bin/bash
count=0 
while true; do
    cat input.txt | radamsa | tee last_input.txt | ./insert
    test $? -gt 127 && break
    count=$((count + 1))  # Incrementamos el contador
    if [ "$count" -ge 100 ]; then  
        echo "Se Alcanzó el límite de 100 iteraciones."
        break
    fi
done
