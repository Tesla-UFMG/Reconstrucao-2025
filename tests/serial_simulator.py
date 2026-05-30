import time
import math

TTY_PATH = "/dev/ttyACM50"
TAXA_ATUALIZACAO = 0.1  # Loop base a 10Hz (0.1s)

# Limites da caixa de coordenadas geográficas (Bounding Box da Pista)
LAT_MIN = -19.87390071161265
LAT_MAX = -19.872134978622697
LON_MIN = -43.97541167608723
LON_MAX = -43.974145677954716

# Cálculo do centro e da amplitude para formar o circuito elíptico
LAT_CENTER = (LAT_MAX + LAT_MIN) / 2.0
LON_CENTER = (LON_MAX + LON_MIN) / 2.0
LAT_AMP = (LAT_MAX - LAT_MIN) / 2.0
LON_AMP = (LON_MAX - LON_MIN) / 2.0

# Tempo que o carro leva para dar uma volta completa na pista (em segundos)
TEMPO_DE_VOLTA = 25.0 

print(f"Injetando sinais em {TTY_PATH}...")

try:
    with open(TTY_PATH, "w") as f:
        start_time = time.time()
        loop_count = 0  # Contador de *ticks* do escalonador
        
        while True:
            t = time.time() - start_time
            linhas = []
            
            # ==========================================
            # TASKS DE 10Hz (Atualizam em todos os ciclos)
            # ==========================================
            
            # ID 32: Sinais de base
            val1_32 = math.sin(t/4) * 180
            val2_32 = math.cos(t/4) * 180
            val3_32 = int(t // 2) % 2
            val4_32 = (t % 5) * (255.0 / 5.0)
            linhas.append(f"32,{val1_32:.4f},{val2_32:.4f},{val3_32},{val4_32:.1f},0,0,0,0")
            
            # ==========================================
            # TASKS DE 5Hz (Atualizam a cada 2 ciclos)
            # ==========================================
            if loop_count % 2 == 0:
                # ID 33: Sinais rápidos
                val1_33 = math.sin(t/2) * 90
                val2_33 = math.cos(t/2) * 90
                val3_33 = int(t // 1) % 2
                val4_33 = (t % 3) * (255.0 / 3.0)
                linhas.append(f"33,{val1_33:.4f},{val2_33:.4f},{val3_33},{val4_33:.1f},0,0,0,0")

                # ID 36: Simulação de GPS
                omega = (2 * math.pi) / TEMPO_DE_VOLTA
                lat_sim = LAT_CENTER + LAT_AMP * math.sin(omega * t)
                lon_sim = LON_CENTER + LON_AMP * math.cos(omega * t)
                linhas.append(f"36,{lat_sim:.8f},{lon_sim:.8f},0,0.0,0,0,0,0")

            # ==========================================
            # TASKS DE 2Hz (Atualizam a cada 5 ciclos)
            # ==========================================
            if loop_count % 5 == 0:
                # ID 34: Sinais lentos
                val1_34 = math.sin(t/8) * 360
                val2_34 = math.cos(t/8) * 360
                val3_34 = int(t // 4) % 2
                val4_34 = (t % 10) * (255.0 / 10.0)
                linhas.append(f"34,{val1_34:.4f},{val2_34:.4f},{val3_34},{val4_34:.1f},0,0,0,0")

            # ==========================================
            # TASKS DE 1Hz (Atualizam a cada 10 ciclos)
            # ==========================================
            if loop_count % 10 == 0:
                # ID 35: Sinais mistos
                val1_35 = math.sin(t) * 100
                val2_35 = math.cos(t/10) * 100
                val3_35 = int(t // 0.5) % 2
                val4_35 = (t % 2) * (255.0 / 2.0)
                linhas.append(f"35,{val1_35:.4f},{val2_35:.4f},{val3_35},{val4_35:.1f},0,0,0,0")
            
            # ==========================================
            # ESCRITA NO BUFFER E CONTROLE DE TEMPO
            # ==========================================
            for linha in linhas:
                f.write(linha + "\n")
                print(linha)
                
            f.flush()
            
            # Incrementa o contador de *ticks* e dorme
            loop_count += 1
            time.sleep(TAXA_ATUALIZACAO)

except KeyboardInterrupt:
    print("\nSimulação encerrada.")
except FileNotFoundError:
    print(f"\nErro: Arquivo {TTY_PATH} não existe. Certifique-se de que o comando 'socat' está rodando.")
