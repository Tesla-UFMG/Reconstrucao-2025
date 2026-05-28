import time
import math
import random

TTY_PATH = "/dev/ttyACM50"
TAXA_ATUALIZACAO = 0.1  # 10Hz (atualiza a cada 0.1 segundos)

print(f"Injetando sinais em {TTY_PATH}...")

try:
    with open(TTY_PATH, "w") as f:
        start_time = time.time()
        
        while True:
            t = time.time() - start_time
            id_sensor = 32

            val1 = math.sin(t/4) * 180
            val2 = math.cos(t/4) * 180
            
            val3 = int(t // 2) % 2
            
            val4 = (t % 5) * (255.0 / 5.0)
            
            linha = f"{id_sensor},{val1:.4f},{val2:.4f},{val3},{val4:.1f},0,0,0,0"
            
            f.write(linha + "\n")
            f.flush()
            print(linha)
            time.sleep(TAXA_ATUALIZACAO)

except KeyboardInterrupt:
    print("\nSimulação encerrada.")
except FileNotFoundError:
    print(f"\nErro: Arquivo {TTY_PATH} não existe. Certifique-se de que o comando 'socat' está rodando.")
