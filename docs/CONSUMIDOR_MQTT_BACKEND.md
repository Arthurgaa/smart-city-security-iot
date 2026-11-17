# Guia de implementação — consumidor MQTT no backend

Este guia é destinado ao repositório Java da API SOS Maria da Penha. O firmware e o dashboard já publicam/consomem MQTT; para persistir os dados, o backend precisa ser mais um cliente MQTT inscrito nos mesmos tópicos.

## Fluxo esperado

```text
ESP32 --publish--> broker MQTT
                    |          |
                    v          v
                Dashboard   Backend Java
                               |
                               v
                           Banco de dados
```

O backend não deve depender do dashboard para receber os dados. Ele deve manter sua própria conexão MQTT, com reconexão automática.

## Broker e tópicos de demonstração

- Host TCP: `broker.emqx.io`
- Porta: `1883`
- Tópico raiz: `smartcity/security/demo-esp32-sos-74A9`
- Assinaturas:
  - `smartcity/security/demo-esp32-sos-74A9/telemetry`
  - `smartcity/security/demo-esp32-sos-74A9/events/sos/+`
  - `smartcity/security/demo-esp32-sos-74A9/events/motion/+`
  - `smartcity/security/demo-esp32-sos-74A9/events/lighting/+`

Para simplificar a primeira versão, também é possível assinar `smartcity/security/demo-esp32-sos-74A9/#` e filtrar os tópicos no código. Não persista automaticamente `status` ou `current/+` como eventos históricos.

## Dependência e configuração

Escolham uma biblioteca cliente MQTT compatível com a versão do Spring Boot do projeto, por exemplo Eclipse Paho Java ou Spring Integration MQTT. Evitem criar uma chamada HTTP do consumidor para a própria API: o consumidor deve chamar a camada de serviço/repositório da aplicação diretamente.

Centralizar as configurações em variáveis de ambiente, por exemplo:

- `MQTT_BROKER_URI=tcp://broker.emqx.io:1883`
- `MQTT_CLIENT_ID=sos-backend-consumer`
- `MQTT_TOPIC_ROOT=smartcity/security/demo-esp32-sos-74A9`

O broker atual é público e sem autenticação. Isso é apenas para demonstração; em produção use broker privado, usuário/senha ou certificados, TLS e ACLs.

## Mapeamento para o domínio da API

A documentação da API existente descreve tipos de sensor `LIGHTING` e `MOTION`, e a rota REST `POST /sensors/{id}/readings` recebe `{"value": número}`. No consumidor MQTT, reaproveitem a mesma regra de negócio de gravação, chamando o serviço diretamente, sem exigir `X-API-Key` na mensagem MQTT.

| Origem MQTT | Registro de destino | Valor |
|---|---|---|
| Telemetria: `luminosidade_pct` | Sensor cadastrado do tipo `LIGHTING` vinculado ao ESP32 | 0 a 100 |
| Evento PIR: `active:true` | Sensor cadastrado do tipo `MOTION` | 1 |
| Evento PIR: `active:false` | Sensor cadastrado do tipo `MOTION` | 0 |
| Evento SOS | Serviço/entidade de alerta SOS | evento SOS, não leitura de sensor |

Não presumam que `LIGHTING` ou `MOTION` sejam IDs. São tipos. O backend deve localizar os registros de sensor cadastrados e associá-los ao `device_id` `ESP32_SOS_001`. O tipo e o formato do ID devem seguir a entidade real implementada no backend.

## Regras de processamento

### 1. Telemetria LDR

Para cada mensagem em `/telemetry`:

1. Fazer parse do JSON.
2. Exigir `type == "telemetry"`, `device_id` conhecido e `luminosidade_pct` numérico.
3. Validar `0 <= luminosidade_pct <= 100`.
4. Resolver o sensor LIGHTING associado ao dispositivo.
5. Persistir a leitura com o valor percentual.

O firmware publica telemetria aproximadamente a cada 5 segundos. Isso pode gerar muitas linhas. Para a demonstração, é aceitável persistir cada amostra; se o volume for excessivo, combinem um intervalo de persistência (por exemplo, uma amostra a cada 15 ou 30 segundos), mantendo os eventos de alerta.

### 2. Eventos PIR

Para cada mensagem em `/events/motion/+`:

1. Exigir `type == "motion_event"`, `event_id`, `device_id` e boolean `active`.
2. Converter `active=true` em leitura MOTION `1`, e `active=false` em `0`.
3. Resolver o sensor MOTION do dispositivo e persistir.
4. Deduplicar usando `event_id`.

**Ajuste no firmware:** atualmente o ESP32 publica evento quando o PIR vai para HIGH, mas ao voltar para LOW publica somente o estado retido `current/motion=false`. Para gravar também a normalização, o firmware precisa publicar um segundo `motion_event` com `active:false`, em tópico `/events/motion/{event_id}`. O dashboard foi ajustado para exibir os dois estados.

### 3. Eventos de iluminação

Para cada mensagem em `/events/lighting/+`:

1. Exigir `type == "lighting_event"`, `event_id`, `device_id`, boolean `active` e `luminosidade_pct` numérico.
2. Validar o percentual entre 0 e 100.
3. Deduplicar por `event_id`.
4. Registrar a transição/alerta. Se a API tratar também leituras do sensor LIGHTING, evitar gravar a mesma amostra duas vezes: a telemetria é a fonte de leituras periódicas; o evento de iluminação é a fonte do histórico de alertas.

### 4. SOS

Para cada mensagem em `/events/sos/+`:

1. Exigir `type == "sos"`, `event_id`, `device_id` e `emergency == true`.
2. Validar os campos e deduplicar por `event_id`.
3. Persistir como evento de emergência pelo serviço de domínio adequado.

**Atenção:** a rota REST `POST /sos-alerts` documentada atualmente exige JWT de perfil CITIZEN. O consumidor MQTT não possui esse JWT e não deve fingir ser um cidadão nem guardar token pessoal no ESP32. A equipe do backend deve implementar o fluxo de persistência interno para SOS recebido via MQTT (reutilizando serviço/repositório), ou definir explicitamente uma integração autenticada para esse caso. A mensagem MQTT, por si só, não cria um registro SOS na API.

## Deduplicação, disponibilidade e confiabilidade

- Use `event_id` como identificador idempotente para SOS, PIR e iluminação; crie uma restrição única no banco quando possível.
- A telemetria não tem `event_id` atualmente. Para evitar duplicidade após reconexões, não use mensagens retidas para telemetria; se precisarem de idempotência por amostra, adicionem um identificador de amostra ao firmware.
- O firmware publica atualmente eventos com QoS 0 e `retained=false`; uma desconexão pode causar perda de evento. O consumidor deve registrar logs e reconectar, mas não consegue recuperar mensagens QoS 0 perdidas enquanto estava offline.
- Não transforme mensagens retidas em novas leituras ao reiniciar o consumidor. Os tópicos `current/+` servem apenas para reconstruir o estado atual.
- Configure timeout, reconexão, shutdown limpo e logs com tópico, tipo e resultado, sem registrar segredos.

## Critérios de aceite

- [ ] Backend conecta ao broker e reconecta após queda.
- [ ] Recebe telemetria LDR e persiste valores de 0 a 100 no sensor LIGHTING.
- [ ] Recebe movimento e normalização do PIR e persiste 1/0 no sensor MOTION.
- [ ] Recebe SOS e cria registro pelo fluxo interno próprio, sem JWT pessoal.
- [ ] Deduplica eventos por `event_id`.
- [ ] Ignora `current/+` como fonte de histórico.
- [ ] Dashboard continua funcionando sem alteração da sua conexão MQTT.
- [ ] Não existe API Key do backend no firmware nem no dashboard.
