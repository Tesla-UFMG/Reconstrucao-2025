import time
import math

TTY_PATH = "/dev/ttyACM50"
TAXA_ATUALIZACAO = 0.1  # 10Hz (atualiza a cada 0.1 segundos)

print(f"Injetando sinais em {TTY_PATH}...")

try:
    with open(TTY_PATH, "w") as f:
        start_time = time.time()
        
        while True:
            t = time.time() - start_time
            
            # ID 32: Lógica original (Sinais de base)
            val1_32 = math.sin(t/4) * 180
            val2_32 = math.cos(t/4) * 180
            val3_32 = int(t // 2) % 2
            val4_32 = (t % 5) * (255.0 / 5.0)
            linha_32 = f"32,{val1_32:.4f},{val2_32:.4f},{val3_32},{val4_32:.1f},0,0,0,0"
            
            # ID 33: Sinais mais rápidos e amplitude menor (metade)
            val1_33 = math.sin(t/2) * 90
            val2_33 = math.cos(t/2) * 90
            val3_33 = int(t // 1) % 2              # Alterna a cada 1 segundo
            val4_33 = (t % 3) * (255.0 / 3.0)      # Rampa zera a cada 3 segundos
            linha_33 = f"33,{val1_33:.4f},{val2_33:.4f},{val3_33},{val4_33:.1f},0,0,0,0"

            # ID 34: Sinais mais lentos e amplitude maior (dobro)
            val1_34 = math.sin(t/8) * 360
            val2_34 = math.cos(t/8) * 360
            val3_34 = int(t // 4) % 2              # Alterna a cada 4 segundos
            val4_34 = (t % 10) * (255.0 / 10.0)    # Rampa zera a cada 10 segundos
            linha_34 = f"34,{val1_34:.4f},{val2_34:.4f},{val3_34},{val4_34:.1f},0,0,0,0"

            # ID 35: Sinais mistos / comportamentos assimétricos
            val1_35 = math.sin(t) * 100            # Seno bem rápido
            val2_35 = math.cos(t/10) * 100         # Cosseno bem lento
            val3_35 = int(t // 0.5) % 2            # Alterna a cada meio segundo (0.5s)
            val4_35 = (t % 2) * (255.0 / 2.0)      # Rampa zera a cada 2 segundos
            linha_35 = f"35,{val1_35:.4f},{val2_35:.4f},{val3_35},{val4_35:.1f},0,0,0,0"
            
            # Agrupa os pacotes do ciclo atual
            linhas = [linha_32, linha_33, linha_34, linha_35]
            
            # Escreve todos os IDs no buffer de uma vez
            for linha in linhas:
                f.write(linha + "\n")
                print(linha)
                
            f.flush()
            time.sleep(TAXA_ATUALIZACAO)

except KeyboardInterrupt:
    print("\nSimulação encerrada.")
except FileNotFoundError:
    print(f"\nErro: Arquivo {TTY_PATH} não existe. Certifique-se de que o comando 'socat' está rodando.")
