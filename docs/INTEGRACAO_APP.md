# Guia de integração — MQTT

Este documento descreve como uma aplicação pode consumir as mensagens do projeto Smart City Security.

## Conexão de demonstração

- Transporte: MQTT sobre WebSocket seguro (WSS)
- Broker: `wss://broker.emqx.io:8084/mqtt`
- Tópico raiz: `smartcity/security/demo-esp32-sos-74A9`
- Assinatura geral: `smartcity/security/demo-esp32-sos-74A9/#`

O broker é público e serve somente para demonstração. Mensagens podem ser vistas por terceiros e não há garantia de privacidade, disponibilidade ou isolamento entre participantes.

## Tópicos

| Tópico | Finalidade |
|---|---|
| `<ROOT>/status` | Estado/conectividade do dispositivo |
| `<ROOT>/current/emergency` | Estado atual de emergência |
| `<ROOT>/current/motion` | Estado atual de movimento |
| `<ROOT>/current/low_light` | Estado atual de baixa luminosidade |
| `<ROOT>/telemetry` | Telemetria periódica |
| `<ROOT>/events/sos/...` | Eventos de SOS |
| `<ROOT>/events/motion/...` | Eventos de movimento |
| `<ROOT>/events/lighting/...` | Eventos de iluminação |

Substitua `<ROOT>` pelo tópico raiz indicado acima. Os sufixos dos tópicos de eventos podem identificar eventos individuais.

## Payload de telemetria

Mensagens de telemetria são JSON e incluem `type: "telemetry"`.

Exemplo ilustrativo:

```json
{
  "type": "telemetry",
  "luminosidade_pct": 42,
  "adc": 1730,
  "device_id": "demo-esp32",
  "dark": false,
  "motion": false
}
```

Campos:

- `type`: identifica a mensagem de telemetria.
- `luminosidade_pct`: luminosidade em percentual.
- `adc`: leitura bruta do conversor analógico-digital.
- `device_id`: identificador opcional do dispositivo.
- `dark`: indicador opcional de baixa luminosidade.
- `motion`: indicador opcional de movimento.

Os valores do exemplo são fictícios. Trate campos opcionais como ausentes quando não forem enviados.

## Tipos de evento

O dashboard reconhece os seguintes identificadores de tipo:

- `sos`: evento de emergência/SOS.
- `motion_event`: evento de movimento.
- `lighting_event`: evento relacionado à iluminação.

A estrutura detalhada de cada evento deve ser confirmada com o payload efetivamente publicado pelo firmware. Uma aplicação robusta deve validar o JSON e tolerar campos ausentes ou desconhecidos.

## Exemplo de assinatura JavaScript

A biblioteca MQTT.js pode ser carregada pelo CDN:

```html
<script src="https://unpkg.com/mqtt/dist/mqtt.min.js"></script>
```

Exemplo mínimo:

```javascript
const ROOT = "smartcity/security/demo-esp32-sos-74A9";
const client = mqtt.connect("wss://broker.emqx.io:8084/mqtt");

client.on("connect", () => {
  client.subscribe(`${ROOT}/#`);
});

client.on("message", (topic, payload) => {
  const raw = payload.toString();

  try {
    const data = JSON.parse(raw);
    console.log("Mensagem recebida:", topic, data);
  } catch {
    console.warn("Payload não é JSON válido:", topic, raw);
  }
});
```

## Recomendações para aplicações consumidoras

1. Conecte-se por WSS e implemente reconexão com intervalo progressivo.
2. Valide o tópico e o conteúdo antes de atualizar a interface.
3. Não presuma que mensagens chegarão em uma ordem específica.
4. Diferencie estado atual (`current/...`) de eventos históricos (`events/...`).
5. Não trate a ausência de mensagem como prova de que um sensor está inativo.
6. Evite armazenar ou exibir informações sensíveis.

## Para produção

Use um broker privado, autenticação individual por cliente, TLS, ACLs que limitem publicação/assinatura por tópico, rotação de credenciais, monitoramento e política de retenção. Nunca coloque credenciais administrativas no código do navegador.

## Observação sobre localização

As coordenadas do mapa no dashboard são apenas marcadores demonstrativos fixos. Não são obtidas por GPS e não devem ser interpretadas como localização real do dispositivo.
