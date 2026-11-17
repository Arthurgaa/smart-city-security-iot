#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_system.h>
#include <ctype.h>
#include <string.h>

// ============================================================
// SMART CITY SECURITY - SOS + LDR + PIR
// ESP32 DevKit-C / Wokwi
//
// Arquitetura:
// ESP32 -> MQTT Broker -> Dashboard
//                      -> Backend Java (consumidor MQTT)
//
// O ESP32 NAO chama a API HTTP e NAO armazena API Key.
// ============================================================

// 1. HARDWARE E DADOS DE DEMONSTRACAO
static const uint8_t PINO_BOTAO_SOS = 12;
static const uint8_t PINO_LDR = 34;
static const uint8_t PINO_PIR = 27;
static const uint8_t PINO_LED = 2;

// Coordenadas fixas e ficticias; nao representam GPS real.
static const double DEMO_LATITUDE = -23.550520;
static const double DEMO_LONGITUDE = -46.633308;

// 2. WI-FI
static const char* WIFI_SSID = "Wokwi-GUEST";
static const char* WIFI_PASSWORD = "";
static const uint8_t WIFI_CHANNEL = 6;

// 3. MQTT
static const char* MQTT_HOST = "broker.emqx.io";
static const uint16_t MQTT_PORT = 1883;
static const char* DEVICE_ID = "ESP32_SOS_001";
static const char* TOPIC_ROOT =
    "smartcity/security/demo-esp32-sos-74A9";

char topicStatus[128];
char topicTelemetry[128];
char topicEmergency[128];
char topicMotionStatus[128];
char topicLowLightStatus[128];
char mqttClientId[64];
char bootToken[12];

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// 4. TEMPOS E LIMITES
static const unsigned long TEMPO_INICIALIZACAO_MS = 2000;
static const unsigned long TEMPO_DEBOUNCE_BOTAO_MS = 50;
static const unsigned long TEMPO_AQUECIMENTO_PIR_MS = 3000;
static const unsigned long TEMPO_DEBOUNCE_PIR_MS = 100;

static const unsigned long INTERVALO_LDR_MS = 5000;
static const unsigned long INTERVALO_WIFI_MS = 10000;
static const unsigned long INTERVALO_STATUS_MS = 15000;
static const unsigned long MQTT_RETRY_INICIAL_MS = 3000;
static const unsigned long MQTT_RETRY_MAX_MS = 20000;

static const int LIMITE_PONTO_ESCURO = 20;

// 5. VARIAVEIS DE ESTADO
int ultimaLeituraBrutaBotao = HIGH;
int estadoBotaoEstavel = HIGH;
unsigned long inicioSistema = 0;
unsigned long ultimaMudancaBotao = 0;
bool botaoArmado = false;

int ultimaLeituraBrutaPIR = LOW;
int estadoPIRestavel = LOW;
unsigned long ultimaMudancaPIR = 0;
bool pirArmado = false;

unsigned long ultimaLeituraLDR = 0;
unsigned long ultimaTentativaWiFi = 0;
unsigned long proximaTentativaMQTT = 0;
unsigned long ultimoStatus = 0;
unsigned long ultimoLogWiFi = 0;

unsigned long intervaloAtualMQTT = MQTT_RETRY_INICIAL_MS;
bool wifiJaConectado = false;

uint32_t contadorSOS = 0;
uint32_t contadorMovimento = 0;
uint32_t contadorIluminacao = 0;

int ultimoValorLDR = 0;
int ultimaLuminosidade = 0;

bool ldrJaLido = false;
bool estadoPontoEscuro = false;
bool estadoPontoEscuroConhecido = false;

// 6. LOGS
void imprimirHorario() {
  unsigned long segundos = millis() / 1000UL;
  unsigned long horas = segundos / 3600UL;
  unsigned long minutos = (segundos % 3600UL) / 60UL;
  unsigned long secs = segundos % 60UL;

  Serial.printf("[%02lu:%02lu:%02lu]", horas, minutos, secs);
}

void logMensagem(const char* categoria, const char* mensagem) {
  imprimirHorario();
  Serial.printf(" [%-10s] %s\n", categoria, mensagem);
}

// 7. PUBLICACAO MQTT
bool publicarJson(const char* topic,
                  const char* payload,
                  bool retained = false) {
  if (!mqtt.connected()) {
    logMensagem("MQTT", "Sem conexao; mensagem nao publicada.");
    return false;
  }

  const bool ok = mqtt.publish(topic, payload, retained);
  if (!ok) {
    logMensagem("MQTT", "Falha ao publicar mensagem.");
  }
  return ok;
}

void publicarEstadoAtual() {
  publicarJson(topicStatus, "online", true);

  publicarJson(
      topicEmergency,
      (botaoArmado && estadoBotaoEstavel == LOW) ? "true" : "false",
      true
  );

  publicarJson(
      topicMotionStatus,
      (pirArmado && estadoPIRestavel == HIGH) ? "true" : "false",
      true
  );

  publicarJson(
      topicLowLightStatus,
      estadoPontoEscuro ? "true" : "false",
      true
  );
}

// 8. WI-FI E MQTT
void iniciarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  ultimaTentativaWiFi = millis();
  ultimoLogWiFi = millis();
  logMensagem("WIFI", "Conectando a rede Wokwi-GUEST...");
}

void manterWiFi() {
  const unsigned long agora = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiJaConectado) {
      wifiJaConectado = true;
      Serial.println();
      logMensagem("WIFI", "Conectado com sucesso.");
      Serial.print("[WIFI] IP: ");
      Serial.println(WiFi.localIP());

      // Ao recuperar Wi-Fi, permite nova tentativa MQTT imediata.
      proximaTentativaMQTT = agora;
    }
    return;
  }

  if (wifiJaConectado) {
    wifiJaConectado = false;
    logMensagem("WIFI", "Conexao Wi-Fi perdida.");
  }

  if (agora - ultimoLogWiFi >= 5000) {
    ultimoLogWiFi = agora;
    Serial.printf("[WIFI] Aguardando conexao. Status: %d\n",
                  (int)WiFi.status());
  }

  if (agora - ultimaTentativaWiFi >= INTERVALO_WIFI_MS) {
    ultimaTentativaWiFi = agora;
    logMensagem("WIFI", "Solicitando reconexao Wi-Fi...");
    WiFi.reconnect();
  }
}

void conectarMQTT() {
  const unsigned long agora = millis();

  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) {
    return;
  }

  if (agora < proximaTentativaMQTT) {
    return;
  }

  logMensagem("MQTT", "Conectando ao broker EMQX...");

  const bool conectado = mqtt.connect(
      mqttClientId,
      topicStatus,
      0,
      true,
      "offline"
  );

  if (conectado) {
    logMensagem("MQTT", "Conectado ao broker.");
    intervaloAtualMQTT = MQTT_RETRY_INICIAL_MS;
    proximaTentativaMQTT = agora + intervaloAtualMQTT;

    publicarEstadoAtual();

    char info[180];
    snprintf(info, sizeof(info),
        "{"
          "\"type\":\"device_info\","
          "\"device_id\":\"%s\","
          "\"boot_id\":\"%s\""
        "}",
        DEVICE_ID,
        bootToken
    );
    publicarJson(topicTelemetry, info, false);
  } else {
    Serial.printf("[MQTT] Falha na conexao. Estado: %d\n",
                  mqtt.state());

    // Backoff progressivo: 3, 6, 12 e no maximo 20 segundos.
    proximaTentativaMQTT = agora + intervaloAtualMQTT;
    intervaloAtualMQTT *= 2;
    if (intervaloAtualMQTT > MQTT_RETRY_MAX_MS) {
      intervaloAtualMQTT = MQTT_RETRY_MAX_MS;
    }
  }
}

void manterConexoes() {
  manterWiFi();

  // O loop MQTT e executado frequentemente no loop principal.
  if (mqtt.connected()) {
    mqtt.loop();
  }

  if (!mqtt.connected()) {
    conectarMQTT();
  }
}

// 9. EVENTO SOS
void dispararAlertaSOS() {
  contadorSOS++;

  Serial.println();
  Serial.println("==================================================");
  logMensagem("ALERTA SOS", "EMERGENCIA DETECTADA!");
  Serial.printf("Dispositivo: %s | Ocorrencia: %lu\n",
                DEVICE_ID, (unsigned long)contadorSOS);
  Serial.println("==================================================");

  char eventId[64];
  snprintf(eventId, sizeof(eventId), "%s-S%lu",
           bootToken, (unsigned long)contadorSOS);

  char payload[400];
  snprintf(payload, sizeof(payload),
      "{"
        "\"type\":\"sos\","
        "\"event_id\":\"%s\","
        "\"device_id\":\"%s\","
        "\"counter\":%lu,"
        "\"uptime_ms\":%lu,"
        "\"latitude\":%.6f,"
        "\"longitude\":%.6f,"
        "\"emergency\":true"
      "}",
      eventId, DEVICE_ID,
      (unsigned long)contadorSOS,
      millis(), DEMO_LATITUDE, DEMO_LONGITUDE
  );

  char topicEvent[180];
  snprintf(topicEvent, sizeof(topicEvent), "%s/events/sos/%s",
           TOPIC_ROOT, eventId);

  // Evento historico sem retain; estado atual retido.
  publicarJson(topicEvent, payload, false);
  publicarJson(topicEmergency, "true", true);
}

void liberarAlertaSOS() {
  digitalWrite(PINO_LED, LOW);
  publicarJson(topicEmergency, "false", true);
  logMensagem("SOS", "Botao liberado; estado de emergencia encerrado.");
}

// 10. EVENTOS DO PIR
void registrarEventoMovimento(bool ativo) {
  contadorMovimento++;

  char eventId[64];
  snprintf(eventId, sizeof(eventId), "%s-M%lu",
           bootToken, (unsigned long)contadorMovimento);

  char payload[400];
  snprintf(payload, sizeof(payload),
      "{"
        "\"type\":\"motion_event\","
        "\"event_id\":\"%s\","
        "\"device_id\":\"%s\","
        "\"counter\":%lu,"
        "\"active\":%s,"
        "\"uptime_ms\":%lu,"
        "\"latitude\":%.6f,"
        "\"longitude\":%.6f"
      "}",
      eventId, DEVICE_ID,
      (unsigned long)contadorMovimento,
      ativo ? "true" : "false",
      millis(), DEMO_LATITUDE, DEMO_LONGITUDE
  );

  char topicEvent[180];
  snprintf(topicEvent, sizeof(topicEvent), "%s/events/motion/%s",
           TOPIC_ROOT, eventId);

  publicarJson(topicMotionStatus, ativo ? "true" : "false", true);
  publicarJson(topicEvent, payload, false);

  if (ativo) {
    logMensagem("PIR", "Movimento detectado na area monitorada.");
  } else {
    logMensagem("PIR", "Movimento encerrado; area normalizada.");
  }
}

void processarSensorPIR(unsigned long agora) {
  const int leitura = digitalRead(PINO_PIR);

  if (leitura != ultimaLeituraBrutaPIR) {
    ultimaLeituraBrutaPIR = leitura;
    ultimaMudancaPIR = agora;
  }

  if ((agora - ultimaMudancaPIR >= TEMPO_DEBOUNCE_PIR_MS) &&
      (estadoPIRestavel != ultimaLeituraBrutaPIR)) {
    estadoPIRestavel = ultimaLeituraBrutaPIR;

    if (pirArmado) {
      registrarEventoMovimento(estadoPIRestavel == HIGH);
    }
  }

  if (!pirArmado &&
      (agora - inicioSistema >= TEMPO_AQUECIMENTO_PIR_MS) &&
      estadoPIRestavel == LOW &&
      ultimaLeituraBrutaPIR == LOW &&
      (agora - ultimaMudancaPIR >= TEMPO_DEBOUNCE_PIR_MS)) {
    pirArmado = true;
    logMensagem("PIR", "Sensor de movimento aquecido e armado.");
    publicarJson(topicMotionStatus, "false", true);
  }
}

// 11. EVENTOS DE ILUMINACAO
void publicarEventoIluminacao(bool ativo, int luminosidade) {
  contadorIluminacao++;

  char eventId[64];
  snprintf(eventId, sizeof(eventId), "%s-L%lu",
           bootToken, (unsigned long)contadorIluminacao);

  char payload[400];
  snprintf(payload, sizeof(payload),
      "{"
        "\"type\":\"lighting_event\","
        "\"event_id\":\"%s\","
        "\"device_id\":\"%s\","
        "\"counter\":%lu,"
        "\"active\":%s,"
        "\"luminosidade_pct\":%d,"
        "\"uptime_ms\":%lu,"
        "\"latitude\":%.6f,"
        "\"longitude\":%.6f"
      "}",
      eventId, DEVICE_ID,
      (unsigned long)contadorIluminacao,
      ativo ? "true" : "false",
      luminosidade, millis(),
      DEMO_LATITUDE, DEMO_LONGITUDE
  );

  char topicEvent[180];
  snprintf(topicEvent, sizeof(topicEvent), "%s/events/lighting/%s",
           TOPIC_ROOT, eventId);

  publicarJson(topicEvent, payload, false);
}

void enviarDadosIluminacao() {
  const int valorLDR = analogRead(PINO_LDR);

  int luminosidade = map(valorLDR, 0, 4095, 0, 100);
  luminosidade = constrain(luminosidade, 0, 100);

  const bool pontoEscuro = luminosidade < LIMITE_PONTO_ESCURO;

  ultimoValorLDR = valorLDR;
  ultimaLuminosidade = luminosidade;
  ldrJaLido = true;

  const bool primeiraLeitura = !estadoPontoEscuroConhecido;
  const bool mudouEstado =
      primeiraLeitura || (pontoEscuro != estadoPontoEscuro);

  if (mudouEstado) {
    estadoPontoEscuro = pontoEscuro;
    estadoPontoEscuroConhecido = true;

    publicarJson(topicLowLightStatus, pontoEscuro ? "true" : "false", true);

    if (pontoEscuro) {
      logMensagem("ALERTA LDR", "PONTO COM BAIXA LUMINOSIDADE!");
      publicarEventoIluminacao(true, luminosidade);
    } else if (!primeiraLeitura) {
      logMensagem("LDR", "Luminosidade normalizada.");
      publicarEventoIluminacao(false, luminosidade);
    }
  }

  const char* diagnostico = "ILUMINACAO_OK";
  if (luminosidade < LIMITE_PONTO_ESCURO) {
    diagnostico = "PONTO_ESCURO";
  } else if (luminosidade < 50) {
    diagnostico = "BAIXA_LUMINOSIDADE";
  }

  Serial.println();
  Serial.println("---------------- TELEMETRIA LDR ----------------");
  imprimirHorario();
  Serial.printf(" [LDR] ADC=%d | Luminosidade=%d%% | Diagnostico=%s\n",
                valorLDR, luminosidade, diagnostico);

  char payload[400];
  snprintf(payload, sizeof(payload),
      "{"
        "\"type\":\"telemetry\","
        "\"device_id\":\"%s\","
        "\"uptime_ms\":%lu,"
        "\"adc\":%d,"
        "\"luminosidade_pct\":%d,"
        "\"dark\":%s,"
        "\"motion\":%s,"
        "\"diagnostico\":\"%s\","
        "\"latitude\":%.6f,"
        "\"longitude\":%.6f"
      "}",
      DEVICE_ID, millis(), valorLDR, luminosidade,
      pontoEscuro ? "true" : "false",
      (pirArmado && estadoPIRestavel == HIGH) ? "true" : "false",
      diagnostico, DEMO_LATITUDE, DEMO_LONGITUDE
  );

  if (publicarJson(topicTelemetry, payload, false)) {
    logMensagem("MQTT", "Telemetria LDR/PIR publicada.");
  }
}

// 12. MONITOR SERIAL
void imprimirAjuda() {
  Serial.println();
  Serial.println("============== COMANDOS DISPONIVEIS ==============");
  Serial.println("h ou ? - Exibir esta ajuda");
  Serial.println("s     - Consultar status do sistema");
  Serial.println("l     - Forcar leitura do LDR");
  Serial.println("p     - Consultar estado do sensor PIR");
  Serial.println("==================================================");
}

void imprimirStatus() {
  Serial.println();
  Serial.println("================ STATUS DO SISTEMA ================");
  Serial.printf("Wi-Fi: %s\n",
      WiFi.status() == WL_CONNECTED ? "CONECTADO" : "DESCONECTADO");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
  Serial.printf("MQTT: %s\n", mqtt.connected() ? "CONECTADO" : "DESCONECTADO");
  Serial.printf("MQTT state: %d\n", mqtt.state());
  Serial.printf("Broker: %s:%u\n", MQTT_HOST, MQTT_PORT);
  Serial.printf("Botao SOS: %s\n",
      digitalRead(PINO_BOTAO_SOS) == LOW ? "PRESSIONADO" : "LIBERADO");
  Serial.printf("Protecao SOS: %s\n", botaoArmado ? "ARMADA" : "INICIALIZANDO");
  Serial.printf("PIR: %s | %s\n",
      pirArmado ? "ARMADO" : "AQUECENDO",
      digitalRead(PINO_PIR) == HIGH ? "MOVIMENTO" : "SEM MOVIMENTO");
  Serial.printf("LED: %s\n", digitalRead(PINO_LED) ? "ACESO" : "APAGADO");
  Serial.printf("Alertas SOS: %lu | Transicoes PIR: %lu\n",
      (unsigned long)contadorSOS, (unsigned long)contadorMovimento);

  if (ldrJaLido) {
    Serial.printf("LDR ADC: %d | Luminosidade: %d%% | Ponto escuro: %s\n",
        ultimoValorLDR, ultimaLuminosidade,
        estadoPontoEscuro ? "SIM" : "NAO");
  } else {
    Serial.println("LDR: aguardando primeira leitura.");
  }
  Serial.println("==================================================");
}

void processarComandosSerial() {
  while (Serial.available() > 0) {
    const char comando = (char)tolower((unsigned char)Serial.read());

    if (comando == '\n' || comando == '\r') continue;

    switch (comando) {
      case 'h':
      case '?':
        imprimirAjuda();
        break;
      case 's':
        imprimirStatus();
        break;
      case 'l':
        logMensagem("COMANDO", "Leitura manual do LDR solicitada.");
        enviarDadosIluminacao();
        ultimaLeituraLDR = millis();
        break;
      case 'p':
        Serial.printf("[PIR] %s | Estado: %s\n",
            pirArmado ? "ARMADO" : "AQUECENDO",
            digitalRead(PINO_PIR) == HIGH
                ? "MOVIMENTO DETECTADO" : "SEM MOVIMENTO");
        break;
      default:
        logMensagem("AVISO", "Comando desconhecido. Digite h para ajuda.");
        break;
    }
  }
}

// 13. SETUP
void setup() {
  Serial.begin(115200);

  pinMode(PINO_BOTAO_SOS, INPUT_PULLUP);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_LDR, INPUT);
  pinMode(PINO_PIR, INPUT);

  analogReadResolution(12);
  digitalWrite(PINO_LED, LOW);

  snprintf(topicStatus, sizeof(topicStatus), "%s/status", TOPIC_ROOT);
  snprintf(topicTelemetry, sizeof(topicTelemetry), "%s/telemetry", TOPIC_ROOT);
  snprintf(topicEmergency, sizeof(topicEmergency),
           "%s/current/emergency", TOPIC_ROOT);
  snprintf(topicMotionStatus, sizeof(topicMotionStatus),
           "%s/current/motion", TOPIC_ROOT);
  snprintf(topicLowLightStatus, sizeof(topicLowLightStatus),
           "%s/current/low_light", TOPIC_ROOT);

  snprintf(bootToken, sizeof(bootToken), "%08lX",
           (unsigned long)esp_random());

  // Client ID exclusivo por inicializacao para evitar disputa
  // caso uma sessao anterior ainda esteja ativa no broker.
  snprintf(mqttClientId, sizeof(mqttClientId),
           "ESP32SOS-%08lX-%s",
           (unsigned long)(uint32_t)ESP.getEfuseMac(),
           bootToken);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(30);
  // Aumentado de 1 para 3 segundos para tolerar latencia do broker.
  mqtt.setSocketTimeout(3);
  mqtt.setBufferSize(768);

  inicioSistema = millis();

  ultimaLeituraBrutaBotao = digitalRead(PINO_BOTAO_SOS);
  estadoBotaoEstavel = ultimaLeituraBrutaBotao;
  ultimaMudancaBotao = millis();

  ultimaLeituraBrutaPIR = digitalRead(PINO_PIR);
  estadoPIRestavel = ultimaLeituraBrutaPIR;
  ultimaMudancaPIR = millis();

  ultimaLeituraLDR = millis();
  proximaTentativaMQTT = millis();
  ultimoStatus = millis();

  Serial.println();
  Serial.println("==================================================");
  Serial.println("       SMART CITY SECURITY - ESP32");
  Serial.println("       SOS + LDR + PIR + MQTT");
  Serial.println("==================================================");

  logMensagem("BOOT", "Botao GPIO12 | LDR GPIO34 | PIR GPIO27 | LED GPIO2");
  logMensagem("BOOT", "Protecao SOS: 2 segundos; aquecimento PIR: 3 segundos.");
  logMensagem("MQTT", "ESP32 publica eventos; dashboard e backend consomem MQTT.");
  Serial.println("ATENCAO: broker publico e coordenadas ficticias.");

  imprimirAjuda();
  iniciarWiFi();
}

// 14. LOOP PRINCIPAL
void loop() {
  const unsigned long agora = millis();

  // Mantem a sessao MQTT viva com chamadas frequentes.
  // Se desconectar, a rotina agenda a reconexao com backoff.
  manterConexoes();

  processarComandosSerial();

  // Botao SOS com debounce.
  const int leituraBotao = digitalRead(PINO_BOTAO_SOS);

  if (leituraBotao != ultimaLeituraBrutaBotao) {
    ultimaLeituraBrutaBotao = leituraBotao;
    ultimaMudancaBotao = agora;
  }

  if ((agora - ultimaMudancaBotao >= TEMPO_DEBOUNCE_BOTAO_MS) &&
      (estadoBotaoEstavel != ultimaLeituraBrutaBotao)) {
    estadoBotaoEstavel = ultimaLeituraBrutaBotao;

    if (botaoArmado) {
      if (estadoBotaoEstavel == LOW) {
        digitalWrite(PINO_LED, HIGH);
        dispararAlertaSOS();
      } else {
        liberarAlertaSOS();
      }
    }
  }

  // Arma o botao apos a inicializacao.
  if (!botaoArmado) {
    const bool tempoConcluido =
        (agora - inicioSistema >= TEMPO_INICIALIZACAO_MS);
    const bool botaoLiberado =
        (estadoBotaoEstavel == HIGH && ultimaLeituraBrutaBotao == HIGH);
    const bool sinalEstavel =
        (agora - ultimaMudancaBotao >= TEMPO_DEBOUNCE_BOTAO_MS);

    if (tempoConcluido && botaoLiberado && sinalEstavel) {
      botaoArmado = true;
      logMensagem("BOOT", "Inicializacao concluida. Sistema SOS ARMADO.");
      publicarJson(topicEmergency, "false", true);
    }
  }

  processarSensorPIR(agora);

  if (agora - ultimaLeituraLDR >= INTERVALO_LDR_MS) {
    ultimaLeituraLDR = agora;
    enviarDadosIluminacao();
  }

  if (mqtt.connected() &&
      agora - ultimoStatus >= INTERVALO_STATUS_MS) {
    ultimoStatus = agora;
    publicarJson(topicStatus, "online", true);
  }

  // Pequena pausa para evitar loop excessivamente agressivo.
  delay(2);
}
