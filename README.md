# Protótipo IoT: Monitoramento de Temperatura e Umidade com ESP32 e Dashboard em Nuvem

Este projeto implementa um sistema de monitoramento de IoT que coleta dados de temperatura e umidade de um sensor DHT11 conectado a um ESP32-C3 Super Mini. Os dados são enviados para um broker MQTT na nuvem e visualizados em tempo real em um dashboard dinâmico rodando em um notebook do Google Colab.

## Arquitetura do Projeto

A comunicação entre o dispositivo e o dashboard é desacoplada através do protocolo MQTT, o que resolve problemas de conectividade em redes locais restritivas e cria uma solução mais robusta e escalável.

1.  **ESP32 (Publisher):** O dispositivo lê os dados do sensor, trata possíveis falhas (usando uma média de leituras anteriores), formata os dados em um pacote JSON e os **publica** em um tópico MQTT na internet.
2.  **HiveMQ Broker (Intermediário):** Um servidor público na nuvem que recebe as mensagens do ESP32 e as redireciona para qualquer cliente que esteja "escutando" o mesmo tópico.
3.  **Google Colab (Subscriber):** O notebook Python se **inscreve** no mesmo tópico MQTT. Ao receber uma mensagem, ele a processa, corrige o fuso horário para a exibição correta e atualiza um gráfico em tempo real.

```
┌─────────────┐        ┌──────────────────┐        ┌──────────────────┐
│ ESP32 + DHT11 │───────>│ Broker MQTT      │<───────│ Google Colab     │
│  (Publica)    │        │ (broker.hivemq.com) │        │  (Inscreve-se)   │
└─────────────┘        └──────────────────┘        └──────────────────┘
```

## Componentes do Projeto

1.  `firmware_esp32/firmware_esp32.ino`: O código C++ a ser carregado no ESP32.
2.  `dashboard_colab.ipynb`: O notebook Python para visualização dos dados (pode ser um arquivo `.py` se preferir).
3.  `README.md`: Este arquivo.

---

## Pré-requisitos

### Hardware
*   Placa ESP32-C3 Super Mini ou similar.
*   Sensor de Temperatura e Umidade DHT11.
*   Protoboard e Jumpers para conexão.

### Software
*   **Arduino IDE** configurada para a placa ESP32.
    *   **Bibliotecas Arduino necessárias:**
        *   `DHT sensor library` by Adafruit
        *   `PubSubClient` by Nick O'Leary
        *   `ArduinoJson` by Benoît Blanchon
*   **Conta Google** para usar o Google Colab.
*   **Bibliotecas Python** (serão instaladas no Colab): `paho-mqtt`, `pandas`, `plotly`.

---

## Guia de Configuração e Execução

Siga os passos nesta ordem para rodar o projeto.

### Passo 1: Montar o Circuito

Conecte o sensor DHT11 ao ESP32-C3 Super Mini da seguinte forma:
*   **VCC / +** do DHT11 -> Pino **3.3V** do ESP32
*   **GND / -** do DHT11 -> Pino **G** (GND) do ESP32
*   **DATA / OUT** do DHT11 -> Pino **4** do ESP32

### Passo 2: Carregar o Firmware no ESP32

1.  Abra o arquivo `firmware_esp32/firmware_esp32.ino` na Arduino IDE.
2.  **Instale as Bibliotecas:** Vá em `Ferramentas > Gerenciar Bibliotecas...` e instale as três bibliotecas listadas nos pré-requisitos.
3.  **Configure suas Credenciais de Wi-Fi:** No topo do código, altere as seguintes linhas com os dados da sua rede:
    ```cpp
    const char* ssid = "NOME_DA_SUA_REDE";
    const char* password = "SENHA_DA_SUA_REDE";
    ```
4.  **Selecione a Placa Correta:** Vá em `Ferramentas > Placa` e escolha uma placa compatível, como "ESP32C3 Dev Module".
5.  **Conecte o ESP32** ao seu computador via USB.
6.  **Carregue o Código:** Clique no botão "Carregar" (seta para a direita).
7.  **Verifique a Saída:** Após o carregamento, abra o **Monitor Serial** (`Ferramentas > Monitor Serial`) com a velocidade de **115200 baud**. Você deverá ver mensagens confirmando a conexão com o Wi-Fi e, em seguida, o envio de mensagens para o broker MQTT.
    ```
    WiFi conectado!
    Endereço IP: 192.168.1.10
    Tentando conectar ao MQTT Broker...Conectado!
    Publicando no tópico br/com/meuprojeto/esp32c3/sensor: {"temperature":28.5,"humidity":67}
    ```

### Passo 3: Executar o Dashboard no Google Colab

1.  Acesse [colab.research.google.com](https://colab.research.google.com) e crie um novo notebook.
2.  **Crie a primeira célula de código** para instalar as dependências. Cole o código abaixo e execute-a:
    ```python
    !pip install paho-mqtt pandas plotly
    ```
3.  **Crie a segunda célula de código** e cole nela todo o conteúdo do arquivo `dashboard_colab.ipynb` (ou o último script Python que fizemos).
4.  **Execute a segunda célula.**
5.  **Observe o resultado:** O output da célula mostrará primeiro as mensagens de conexão e, em seguida, começará a exibir as mensagens recebidas do seu ESP32. Logo após a primeira mensagem, o gráfico será renderizado e se atualizará a cada 5 segundos.
    ```
    Conectando ao Broker MQTT em broker.hivemq.com...
    Inscrevendo-se no tópico: br/com/meuprojeto/esp32c3/sensor
    Aguardando os primeiros dados do sensor...
    Mensagem recebida: {"temperature":28.5,"humidity":67}
    (Gráfico aparece aqui)
    ```

Se todos os passos foram seguidos, seu sistema de monitoramento estará totalmente funcional
