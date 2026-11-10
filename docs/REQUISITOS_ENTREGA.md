# Parte 6 de 6 — Simulação de IoT (2 pontos)

Este documento relaciona o projeto Smart City Security aos três critérios de avaliação da etapa de IoT.

## Visão geral da avaliação

| Critério | O que a entrega precisa demonstrar | Evidência disponível no projeto | Situação |
|---|---|---|---|
| 01 — Simulação (1,0 pt) | Pelo menos dois sensores simulados por script, JSON ou MQTT simulado | A interface MQTT prevê dados de luminosidade (`luminosidade_pct` / `adc`) e movimento (`motion`), além dos estados `current/motion` e `current/low_light` | **Parcialmente evidenciado**: a integração está preparada para dois sinais de sensores, mas é necessário conferir o circuito e o firmware no Wokwi para comprovar os componentes simulados |
| 02 — Uso dos dados (0,5 pt) | Os dados precisam provocar uma ação, decisão ou alerta na aplicação | O dashboard atualiza os indicadores e registra mensagens/eventos recebidos por MQTT | **Parcialmente atendido**: há atualização visual e histórico; é preciso demonstrar que o firmware publica eventos ao detectar as condições, ou implementar uma regra explícita de alerta na aplicação |
| 03 — Explicação (0,5 pt) | Informar qual sensor é usado, o que mede e como influencia o sistema | Este documento descreve os sinais previstos na integração | **Documentado em nível funcional**; confirmar o modelo exato dos componentes no Wokwi antes da apresentação |

## Sensores e sinais monitorados

### 1. Sensor de luminosidade (LDR ou módulo equivalente)

- **O que mede:** intensidade de luz do ambiente. A leitura analógica do sensor é convertida em um valor bruto (`adc`) e/ou em percentual (`luminosidade_pct`).
- **Como aparece no sistema:** a telemetria MQTT pode transportar `luminosidade_pct`, `adc` e o indicador booleano `dark`. O estado também pode ser publicado em `<ROOT>/current/low_light`.
- **Como influencia a aplicação:** quando a leitura indica pouca iluminação, a aplicação pode sinalizar condição de baixa luminosidade no painel e registrar um evento de iluminação.
- **Atenção:** a interface e os tópicos indicam esse sinal, mas o componente físico exato (LDR, fototransistor ou outro) deve ser confirmado no circuito Wokwi.

### 2. Sensor de movimento (PIR ou módulo equivalente)

- **O que detecta:** presença de movimento no campo de detecção do sensor.
- **Como aparece no sistema:** o indicador `motion` pode ser enviado na telemetria e o estado atual pode ser publicado em `<ROOT>/current/motion`.
- **Como influencia a aplicação:** a detecção pode atualizar o painel e gerar um evento de movimento em `<ROOT>/events/motion/...`.
- **Atenção:** o tipo exato do componente (por exemplo, PIR) deve ser confirmado no circuito Wokwi.

### 3. Botão de SOS (entrada de acionamento, não contabilizada como sensor)

O projeto também prevê estado de emergência e eventos SOS. Um botão de SOS é um dispositivo de entrada acionado pelo usuário; não deve ser apresentado como um segundo sensor. Ele complementa a solução permitindo gerar manualmente um alerta de emergência.

## Fluxo de dados esperado

1. Os sensores simulados fornecem leituras/estados ao ESP32.
2. O firmware interpreta as leituras e identifica condições como movimento ou baixa luminosidade.
3. O ESP32 publica telemetria, estados atuais e/ou eventos em tópicos MQTT.
4. O dashboard assina `<ROOT>/#`, recebe as mensagens e atualiza os indicadores e a lista de eventos.

Tópico raiz usado na demonstração:

```text
smartcity/security/demo-esp32-sos-74A9
```

## Como demonstrar os critérios ao avaliador

1. Abra o [projeto no Wokwi](https://wokwi.com/projects/480294036688727041) e mostre os dois componentes sensores no circuito.
2. Execute a simulação e altere a condição de cada sensor (por exemplo, iluminar/escurecer o LDR e ativar o sensor de movimento).
3. Mostre no dashboard a alteração dos valores/estados recebidos via MQTT.
4. Mostre a publicação do evento correspondente no histórico ou no monitor MQTT.
5. Explique que luminosidade representa a condição de iluminação do local e movimento indica atividade detectada; ambos ajudam a identificar situações que merecem atenção.

## Evidências que ainda precisam ser conferidas

O repositório contém a camada de integração e o dashboard, mas o firmware e o diagrama permanecem no Wokwi. Portanto, para afirmar que a simulação atende integralmente ao critério 01 e que os alertas são efetivamente disparados pelo dispositivo, é necessário conferir no Wokwi:

- quais componentes estão conectados ao ESP32;
- se há pelo menos dois sensores distintos simulados;
- quais limites/condições fazem o firmware publicar os eventos de movimento e iluminação;
- se os eventos aparecem no broker durante a demonstração.

**Importante:** o dashboard que está neste repositório apresenta estados recebidos e registra eventos publicados. A documentação dos tópicos, por si só, não comprova que o firmware esteja efetivamente gerando esses eventos. Não apresente essa parte como comprovada antes de executar o teste.

## Segurança e escopo

O broker configurado é público e serve para fins didáticos. Não envie dados pessoais, credenciais ou localização real. Para produção, use broker privado com autenticação, TLS e ACLs por tópico.
