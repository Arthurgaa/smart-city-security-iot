# Integração MQTT do ESP32 com o backend SOS Maria da Penha

## Arquitetura vigente

O ESP32 publica telemetria, estados e eventos no broker MQTT. Dashboard e backend são consumidores independentes:

```text
ESP32 ── MQTT ──> Broker
                  ├──> Dashboard web
                  └──> Backend Java ──> Banco de dados
```

O firmware não chama a API REST, não precisa conhecer a URL do backend e não armazena `IOT_API_KEY` nem JWT. O backend implementa seu próprio cliente MQTT, valida as mensagens e chama os serviços internos da aplicação para persistência.

## Configuração da demonstração

- Broker TCP: `tcp://broker.emqx.io:1883`
- Broker WSS do dashboard: `wss://broker.emqx.io:8084/mqtt`
- Tópico raiz: `smartcity/security/demo-esp32-sos-74A9`
- Device ID: `ESP32_SOS_001`
- Assinatura ampla: `smartcity/security/demo-esp32-sos-74A9/#`

O broker é público, sem isolamento de dados, e deve ser usado somente para demonstração.

## Tópicos relevantes

| Tópico | Finalidade |
|---|---|
| `<ROOT>/status` | Disponibilidade online/offline (LWT/retained) |
| `<ROOT>/telemetry` | Telemetria periódica e mensagem de identificação `device_info` |
| `<ROOT>/current/emergency` | Estado atual de SOS (retained) |
| `<ROOT>/current/motion` | Estado atual do PIR (retained) |
| `<ROOT>/current/low_light` | Estado atual da iluminação (retained) |
| `<ROOT>/events/sos/+` | Eventos SOS |
| `<ROOT>/events/motion/+` | Transições PIR HIGH e LOW |
| `<ROOT>/events/lighting/+` | Transições de baixa luminosidade |

## Contrato e processamento

- Telemetria: exigir `type == "telemetry"`; usar `luminosidade_pct` (0–100) para LIGHTING e `motion` como snapshot do PIR.
- Identificação: `type == "device_info"` pode aparecer em `/telemetry`; ignorar para gravação de leituras ou tratar como metadado.
- Movimento: `type == "motion_event"`; `active:true` indica detecção (HIGH) e `active:false` indica normalização (LOW). Ambos são eventos históricos e usam `event_id`.
- Iluminação: `type == "lighting_event"`; `active:true` indica baixa luminosidade e `active:false` indica normalização.
- SOS: `type == "sos"`; tratar como alerta de emergência, não como leitura de sensor.
- Estados `current/+`: mensagens retidas para reconstruir o estado atual; não gerar novo evento histórico ao recebê-las.
- Deduplicar eventos por `event_id`, idealmente com restrição única no banco.

## QoS e confiabilidade

As publicações de eventos do firmware são QoS 0 e não retidas. Isso é suficiente para a demonstração, mas uma desconexão pode causar perda de mensagens. Reconexão MQTT não recupera eventos QoS 0 perdidos. Em produção, avaliar QoS 1, armazenamento temporário no dispositivo ou gateway, broker privado e política de reprocessamento.

## Segurança

Não colocar API Key, JWT ou credenciais do backend no ESP32, dashboard ou payload MQTT. Para produção, utilizar broker privado com TLS, autenticação por cliente e ACLs por tópico. O consumidor MQTT deve executar no backend e chamar os serviços internos da aplicação; não deve simular uma chamada autenticada de cidadão ao endpoint REST de SOS.

## Implementação no backend

Consulte [CONSUMIDOR_MQTT_BACKEND.md](./CONSUMIDOR_MQTT_BACKEND.md) para dependências Java, configuração, mapeamento LIGHTING/MOTION, fluxo SOS, reconexão e critérios de aceite.

## Firmware e simulação

O firmware e o circuito continuam mantidos no projeto Wokwi: [abrir simulação ESP32](https://wokwi.com/projects/480294036688727041). O repositório GitHub documenta o contrato de integração; para revisão/versionamento do código, recomenda-se manter também uma cópia sincronizada do sketch e do `diagram.json` neste repositório.
