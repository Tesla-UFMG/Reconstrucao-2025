import requests
import pandas as pd

# Parâmetros
session_key = 9159       # Exemplo: sessão do GP – ajuste se precisar
driver_number = 1        # Número do carro do Max Verstappen

# 1) Obtendo dados de localização (posição na pista)
url_location = f"https://api.openf1.org/v1/location?driver_number={driver_number}&session_key={session_key}"
response_location = requests.get(url_location)
data_location = response_location.json()
df_location = pd.DataFrame(data_location)

# 2) Obtendo dados do carro (velocidade, marcha, rpm, throttle, brake, DRS etc.)
url_car_data = f"https://api.openf1.org/v1/car_data?driver_number={driver_number}&session_key={session_key}"
response_car_data = requests.get(url_car_data)
data_car = response_car_data.json()
df_car = pd.DataFrame(data_car)

# 3) Converter a coluna "date" em datetime para ambos os DataFrames
#    Usamos format='ISO8601' para aceitar timestamps como "2023-09-15T12:49:01+00:00" e outros
df_location['date'] = pd.to_datetime(df_location['date'], format='ISO8601')
df_car['date']      = pd.to_datetime(df_car['date'], format='ISO8601')

# 4) Ordenar por data antes de mesclar
df_location = df_location.sort_values("date")
df_car      = df_car.sort_values("date")

# 5) Mesclar ambos com merge_asof (junção aproximada baseada na coluna "date")
#    Tolerância de 100ms para alinhar registros de localização e telemetria
df_combined = pd.merge_asof(
    df_location,
    df_car,
    on="date",
    direction="nearest",
    tolerance=pd.Timedelta("100ms")
)

# 6) Exportar para CSV
df_combined.dropna(inplace=True, how='any')  
df_combined.to_csv("verstappen_percurso_completo.csv", index=False)

print("Arquivo salvo como 'verstappen_percurso_completo.csv'")
