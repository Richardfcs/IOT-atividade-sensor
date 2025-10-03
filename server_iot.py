import paho.mqtt.client as mqtt
import json
import time
import threading
import pandas as pd
from datetime import datetime
import plotly.graph_objs as go
from plotly.subplots import make_subplots
from IPython.display import display, clear_output

# =========== Configurações do MQTT =============
# Devem ser EXATAMENTE as mesmas do código do ESP32
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "br/com/meuprojeto/esp32c3/sensor"

# =========== Configurações de Plotagem =============
READ_INTERVAL = 5  # Intervalo de atualização do gráfico (em segundos)
MAX_POINTS = 100   # Quantidade máxima de pontos a manter no gráfico

# --- Variáveis globais para buffer de dados ---
data_buffer = {
    "timestamp": [],
    "temperature": [],
    "humidity": []
}
buffer_lock = threading.Lock() # Para evitar problemas com threads

# --- Funções MQTT ---

# Esta função será chamada toda vez que uma nova mensagem chegar no tópico
def on_message(client, userdata, msg):
    try:
        # Decodifica a mensagem (payload) de bytes para string
        payload_str = msg.payload.decode()
        print(f"Mensagem recebida: {payload_str}")
        
        # Converte a string JSON para um dicionário Python
        data = json.loads(payload_str)
        
        temp = data.get("temperature")
        hum = data.get("humidity")
        
        if temp is not None and hum is not None:
            # Trava o buffer para adicionar os novos dados com segurança
            with buffer_lock:
                data_buffer["timestamp"].append(datetime.now())
                data_buffer["temperature"].append(temp)
                data_buffer["humidity"].append(hum)
                
                # Mantém o buffer com no máximo MAX_POINTS
                if len(data_buffer["timestamp"]) > MAX_POINTS:
                    for key in data_buffer:
                        data_buffer[key].pop(0)
                        
    except Exception as e:
        print(f"Erro ao processar mensagem: {e}")

# Função para plotar os dados (Sua função original)
def plot_buffer():
    with buffer_lock:
        # Cria uma cópia para evitar problemas de concorrência durante a plotagem
        df = pd.DataFrame(data_buffer.copy())
        
    if df.empty:
        print("Aguardando os primeiros dados do sensor...")
        return

    fig = make_subplots(specs=[[{"secondary_y": True}]])
    fig.add_trace(
        go.Scatter(x=df["timestamp"], y=df["temperature"], mode="lines+markers", name="Temperatura (°C)"),
        secondary_y=False
    )
    fig.add_trace(
        go.Scatter(x=df["timestamp"], y=df["humidity"], mode="lines+markers", name="Umidade (%)"),
        secondary_y=True
    )
    fig.update_layout(
        title_text="Monitoramento de Temperatura e Umidade em Tempo Real via MQTT",
        xaxis_title="Horário"
    )
    fig.update_yaxes(title_text="<b>Temperatura (°C)</b>", secondary_y=False)
    fig.update_yaxes(title_text="<b>Umidade (%)</b>", secondary_y=True)
    
    clear_output(wait=True)
    display(fig)

# --- Programa Principal ---
if __name__ == "__main__":
    # Cria um cliente MQTT
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    # Associa a função on_message ao cliente
    client.on_message = on_message

    print(f"Conectando ao Broker MQTT em {MQTT_BROKER}...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    
    print(f"Inscrevendo-se no tópico: {MQTT_TOPIC}")
    client.subscribe(MQTT_TOPIC)

    # Inicia o loop de rede do MQTT em uma thread separada.
    # Isso permite que o programa continue rodando enquanto escuta as mensagens.
    client.loop_start()

    # Loop principal para plotar o gráfico
    try:
        while True:
            plot_buffer()
            time.sleep(READ_INTERVAL)
    except KeyboardInterrupt:
        print("Parando o cliente MQTT...")
        client.loop_stop()
        print("Programa finalizado.")