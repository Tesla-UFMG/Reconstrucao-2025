#!/bin/bash
echo "Rodando o aplicativo via GDB para capturar o erro..."
gdb -batch -ex "run" -ex "bt full" -ex "quit" ./build/app > crash_report.txt 2>&1
echo "Erro capturado! Verifique o arquivo crash_report.txt"
