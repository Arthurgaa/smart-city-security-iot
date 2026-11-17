# Integração MQTT do ESP32 com o backend SOS Maria da Penha

## Decisão de arquitetura

O ESP32 publica eventos e telemetria no broker MQTT. O dashboard e o backend consomem os mesmos tópicos, de forma independente.

```text
ESP32 ── MQTT ──> Broker
                  ├──> Dashboard web
                  └──> Backend Java ──> Banco de dados
```

O ESP32 **não envia HTTP diretamente para a API**, não precisa conhecer a URL pública do backend e não armazena a `IOT_API_KEY`. O backend fica responsável por receber, validar, mapear e persistir os dados.

## O que muda no firmware ESP32/Wokwi

O firmware está mantido no Wokwi, não neste repositório. O código atualmente publicado no Wokwi ainda contém uma integração HTTP experimental. Para adotar a arquitetura decidida:

1. Remover os includes `WiFiClientSecure.h` e `HTTPClient.h` se não forem usados por outra funcionalidade.
2. Remover as configurações `API_BASE_URL`, `IOT_API_KEY`, `ID_SENSOR_LDR`, `ID_SENSOR_PIR`, `API_TLS_INSECURE_DEMO`, `API_TIMEOUT_MS`, `API_SOS_PATH` e `API_SOS_HABILITADA`.
3. Remover `apiConfigurada()`, `enviarPOSTAPI()`, `enviarLeituraAPI()` e `enviarAlertaSOSAPI()`.
4. Remover chamadas a `enviarLeituraAPI()` em `processarSensorPIR()` e `enviarDadosIluminacao()`.
5. Remover a chamada `enviarAlertaSOSAPI()` de `dispararAlertaSOS()`.
6. Manter a publicação MQTT já existente para SOS, movimento, iluminação, telemetria e estados atuais.
7. Adicionar um evento MQTT para a transição do PIR para sem movimento, como descrito abaixo.

Não remova a publicação MQTT atual nem altere o tópico raiz sem alinhar com o dashboard e o backend.

## Ajuste necessário no evento PIR

Hoje o firmware publica `events/motion/{event_id}` quando o PIR detecta movimento (`HIGH`). Quando retorna para `LOW`, publica apenas `current/motion = false` como estado retido. Para que o backend registre tanto início quanto fim da detecção, publique também um evento não retido na transição para `LOW`.

Contrato sugerido para o evento de normalização:

```json
{
  "type": "motion_event",
  "event_id": "BOOTID-M2",
  "device_id": "ESP32_SOS_001",
  "counter": 2,
  "active": false,
  "uptime_ms": 25000,
  "latitude": -23.550520,
  "longitude": -46.633308
}
```

O evento de detecção existente continua igual, com `active: true`. Ambos devem ser publicados em `<ROOT>/events/motion/{event_id}` com `retained=false`.

O dashboard também deve interpretar `active:false` como normalização no histórico, em vez de registrar sempre `DETECTADO`. A tela de estado atual já recebe `current/motion`.

## O que precisa ser implementado no backend Java

A API atual documenta o endpoint REST `POST /sensors/{id}/readings` com `X-API-Key`, mas essa rota não é chamada pelo ESP32 nesta arquitetura. O backend precisa de um cliente MQTT próprio.

Consulte [CONSUMIDOR_MQTT_BACKEND.md](./CONSUMIDOR_MQTT_BACKEND.md), que descreve conexão, tópicos, mapeamento, deduplicação e persistência.

### Mapeamento de dados

| Mensagem MQTT | Tipo no backend | Valor a persistir |
|---|---|---|
| Telemetria `luminosidade_pct` | `LIGHTING` | número de 0 a 100 |
| Evento PIR `active:true` | `MOTION` | `1` |
| Evento PIR `active:false` | `MOTION` | `0` |
| Evento SOS | Evento de emergência | registro SOS, não leitura de sensor |

O tipo cadastrado na API e o ID do registro do sensor são conceitos diferentes. O backend deve resolver os registros `LIGHTING` e `MOTION` associados ao dispositivo `ESP32_SOS_001`; não presumir que o texto do tipo seja o ID.

## O que não fazer

- Não colocar API Key no firmware, no dashboard ou neste repositório.
- Não manter o envio HTTP direto do ESP32 em paralelo como se fosse a arquitetura oficial.
- Não persistir `current/...` retido como evento novo a cada reconexão.
- Não cadastrar o SOS como sensor `MOTION`.
- Não considerar as coordenadas fixas como GPS real.

## Critérios de teste ponta a ponta

1. Iniciar o broker e o backend consumidor MQTT.
2. Iniciar o dashboard e a simulação Wokwi.
3. Confirmar que o dashboard continua recebendo os eventos em tempo real.
4. Confirmar nos logs do backend o recebimento dos tópicos de telemetria/eventos.
5. Consultar o banco/API e verificar a criação de leituras LIGHTING, MOTION e registros SOS.
6. Reiniciar o backend e confirmar reconexão MQTT sem duplicar eventos antigos.
7. Confirmar que nenhuma API Key ou URL REST do backend é necessária no firmware.
