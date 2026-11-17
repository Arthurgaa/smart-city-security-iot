# Contrato de integração — MQTT

Este documento define como o dashboard e o backend consomem as mensagens publicadas pelo ESP32 Smart City Security.

## Conexão de demonstração

- Transporte do dashboard: MQTT sobre WebSocket seguro (WSS)
- URL do dashboard: `wss://broker.emqx.io:8084/mqtt`
- Broker MQTT para clientes TCP: `broker.emqx.io:1883`
- Tópico raiz: `smartcity/security/demo-esp32-sos-74A9`
- Assinatura geral: `smartcity/security/demo-esp32-sos-74A9/#`

O broker é público e serve somente para demonstração. Mensagens podem ser vistas por terceiros e não há garantia de privacidade, disponibilidade ou isolamento entre participantes.

## Arquitetura

O ESP32 publica uma única vez no broker MQTT. Dois consumidores independentes recebem as mensagens:

1. Dashboard: atualiza indicadores e histórico local em tempo real.
2. Backend Java: valida os tópicos e payloads, identifica os sensores e grava as leituras/eventos no banco.

O ESP32 não chama a API REST e não recebe nem armazena a API Key do backend. O backend deve conectar-se ao broker como cliente MQTT próprio. A conexão do dashboard não persiste dados no banco.

## Tópicos

Substitua `<ROOT>` pelo tópico raiz acima.

| Tópico | Uso | Persistir no histórico? |
|---|---|---|
| `<ROOT>/status` | Estado/conectividade do dispositivo (`online`/`offline`) | Opcional; normalmente estado operacional |
| `<ROOT>/current/emergency` | Estado atual de emergência, retido | Não como novo evento histórico |
| `<ROOT>/current/motion` | Estado atual de movimento, retido | Não como novo evento histórico |
| `<ROOT>/current/low_light` | Estado atual de baixa luminosidade, retido | Não como novo evento histórico |
| `<ROOT>/telemetry` | Telemetria periódica com LDR e estado PIR | Sim, leitura LIGHTING; motion pode ser usado como snapshot |
| `<ROOT>/events/sos/+` | Acionamentos SOS | Sim, evento SOS |
| `<ROOT>/events/motion/+` | Transições/eventos do PIR | Sim, evento MOTION |
| `<ROOT>/events/lighting/+` | Transições da condição de iluminação | Sim, evento LIGHTING |

**Importante sobre retained:** os tópicos `current/...` representam o último estado conhecido e são publicados com retenção. Ao iniciar ou reconectar, o backend pode receber imediatamente uma mensagem antiga. Use-os apenas para inicializar/atualizar estado atual, nunca para criar uma nova leitura histórica a cada reconexão.

## Payloads publicados pelo firmware

### Telemetria — `<ROOT>/telemetry`

O firmware publica periodicamente um JSON com os campos:

```json
{
  "type": "telemetry",
  "device_id": "ESP32_SOS_001",
  "uptime_ms": 15000,
  "adc": 1730,
  "luminosidade_pct": 42,
  "dark": false,
  "motion": false,
  "diagnostico": "ILUMINACAO_OK",
  "latitude": -23.550520,
  "longitude": -46.633308
}
```

- `luminosidade_pct`: leitura LDR convertida para percentual de 0 a 100. Mapear para o sensor `LIGHTING`.
- `motion`: estado booleano observado do PIR. Pode ser usado para atualizar o estado atual; para histórico de transições, usar os eventos do tópico `events/motion`.
- `adc`: leitura bruta de 0 a 4095, útil para diagnóstico, não é o valor esperado pelo sensor LIGHTING.
- `device_id`: identificador do dispositivo.
- `uptime_ms`: tempo desde a inicialização.
- `latitude` e `longitude`: coordenadas fixas fictícias da demonstração, não GPS real.

### SOS — `<ROOT>/events/sos/{event_id}`

```json
{
  "type": "sos",
  "event_id": "A1B2C3-S1",
  "device_id": "ESP32_SOS_001",
  "counter": 1,
  "uptime_ms": 12000,
  "latitude": -23.550520,
  "longitude": -46.633308,
  "emergency": true
}
```

Cada acionamento deve ser tratado como um evento histórico. O `event_id` é a chave de deduplicação recomendada.

### Movimento — `<ROOT>/events/motion/{event_id}`

O evento atualmente publicado pelo firmware representa a detecção de movimento:

```json
{
  "type": "motion_event",
  "event_id": "A1B2C3-M1",
  "device_id": "ESP32_SOS_001",
  "counter": 1,
  "active": true,
  "uptime_ms": 14000,
  "latitude": -23.550520,
  "longitude": -46.633308
}
```

**Ajuste necessário no firmware para histórico completo:** publicar também um evento quando o PIR voltar para `LOW`, com `type: "motion_event"` e `active: false`. Hoje a transição para sem movimento atualiza apenas o tópico retido `current/motion`, sem evento histórico. Consulte o guia do consumidor backend.

### Iluminação — `<ROOT>/events/lighting/{event_id}`

```json
{
  "type": "lighting_event",
  "event_id": "A1B2C3-L1",
  "device_id": "ESP32_SOS_001",
  "counter": 1,
  "active": true,
  "luminosidade_pct": 15,
  "uptime_ms": 16000,
  "latitude": -23.550520,
  "longitude": -46.633308
}
```

O firmware publica o evento quando a condição de baixa luminosidade muda. `active: true` indica ponto escuro; `active: false` indica normalização.

## Regras para o consumidor backend

1. Assinar os tópicos de eventos e telemetria do ROOT configurado.
2. Validar o tópico, o JSON, o `type` e os campos obrigatórios antes de persistir.
3. Deduplicar eventos usando `event_id` (com restrição única no banco, se possível).
4. Mapear o `device_id` e o tipo de mensagem para os sensores cadastrados no backend.
5. Mapear `luminosidade_pct` para o sensor `LIGHTING` e o estado PIR para `MOTION`, usando valores numéricos `0/1`.
6. Tratar SOS como evento de emergência; não cadastrá-lo como leitura LIGHTING ou MOTION.
7. Não transformar mensagens retidas de `current/...` em leituras históricas.
8. Não confiar em coordenadas recebidas sem validar; neste projeto elas são fictícias.
9. Implementar reconexão e logs. QoS não substitui deduplicação nem persistência.

## Para produção

Use broker privado, autenticação por cliente, TLS, ACLs que limitem publicação/assinatura por tópico, rotação de credenciais, monitoramento e política de retenção. O broker público atual é somente para demonstração.
