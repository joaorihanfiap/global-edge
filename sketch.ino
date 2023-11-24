#include <WiFi.h>
#include <PubSubClient.h>

// Configurações - variáveis editáveis
const char* default_SSID = "Wokwi-GUEST"; // Nome da rede Wi-Fi
const char* default_PASSWORD = ""; // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "46.17.108.113"; // IP do Broker MQTT
const int default_BROKER_PORT = 1883; // Porta do Broker MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/triagem001/cmd"; // Tópico MQTT de escuta
const char* default_TOPICO_PUBLISH_1 = "/TEF/triagem001/attrs"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_2 = "/TEF/triagem001/attrs/o"; // Tópico MQTT de envio de informações para Broker
const char* default_ID_MQTT = "triagem001"; // ID MQTT
const int default_D4 = 2; // Pino do LED onboard
// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "triagem001";

// Variáveis para configurações editáveis
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

WiFiClient espClient;
PubSubClient MQTT(espClient);
char EstadoSaida = '0';

#define PULSE_PER_BEAT    1           // Number of pulses per heartbeat
#define INTERRUPT_PIN     5           // Interrupt pin
#define SAMPLING_INTERVAL 1000        // Sampling interval in milliseconds

volatile uint16_t pulse;              // Variable to be incremented in the interrupt
uint16_t count;                       // Variable to store the current value of pulse

float heartRate;                      // Heart rate calculated from count
float oxygenSaturation;               // Oxygen saturation level (SpO2)

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;  // Mutex to ensure safe access to pulse

void IRAM_ATTR HeartRateInterrupt() {
  portENTER_CRITICAL_ISR(&mux);  // Enter a critical interrupt section
  pulse++;  // Increment the pulse variable safely
  portEXIT_CRITICAL_ISR(&mux);   // Exit the critical interrupt section
}

void initSerial() {
    Serial.begin(115200);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi();
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);
}

void setup() {
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    delay(5000);
    MQTT.publish(TOPICO_PUBLISH_1, "s|on");

    Serial.begin(115200);

    pinMode(INTERRUPT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), HeartRateInterrupt, RISING);  // Configure interrupt on the pin    
}

void loop() {
    VerificaConexoesWiFIEMQTT();
    EnviaEstadoOutputMQTT();
    handleOxigenio();
    MQTT.loop();
    Oximeter();    
}

void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());

    // Garantir que o LED inicie desligado
    digitalWrite(D4, LOW);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Forma o padrão de tópico para comparação
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Compara com o tópico recebido
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
        Serial.println("- Led Ligado");
    }

    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        // Serial.println("- Led Desligado");
    }
    // Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000);
}

void InitOutput() {
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

void Oximeter() {
  static unsigned long startTime;
  if (millis() - startTime < SAMPLING_INTERVAL) return;   // Sampling interval
  startTime = millis();

  portENTER_CRITICAL(&mux);  // Enter a critical section
  count = pulse;  // Save the current value of pulse and reset pulse
  pulse = 0;
  portEXIT_CRITICAL(&mux);   // Exit the critical section

  // Simulate oximetry measurement (replace this with your actual oximetry calculation)
  // For illustration purposes, we use a simple linear relation with heart rate
  oxygenSaturation = map(count, 1, 10, 91, 100);  // Map heart rate to SpO2 range


  if (oxygenSaturation >= 96.0) {
    Serial.println("Saturação do Oxigênio: " + String(oxygenSaturation, 2) + "%");
  }

  if (oxygenSaturation < 96.0) {
    Serial.println("Alerta: Níveis de Saturação do Oxigênio muito Baixos! (" + String(oxygenSaturation, 2) + "%)");

  }
}

void handleOxigenio() {
    float oxigenio = oxygenSaturation;
    if (!isnan(oxigenio)) {
      String mensagem = String(oxigenio);
      // Serial.print("Porcentagem de Oxigenio: ");
      // Serial.println(mensagem.c_str() + "%");
      MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());
    } else {
        Serial.println("Erro ao ler a temperatura do sensor DHT22.");      
    }
}