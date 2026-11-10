# Parte 6 de 6 — Simulação de IoT (2 pontos)

Este documento apresenta as evidências do projeto Smart City Security para os três critérios de avaliação da etapa de IoT, com base no firmware ESP32 fornecido e na integração MQTT do repositório.

## Resultado por critério

| Critério | Evidência concreta no projeto | Pontuação prevista |
|---|---|---:|
| 01 — Simulação | Dois sensores distintos no ESP32: LDR no GPIO 34 e PIR no GPIO 27. O ESP32 lê o LDR por ADC e o PIR por entrada digital; envia telemetria e estados por MQTT. | 1,0 / 1,0 |
| 02 — Uso dos dados | O firmware compara luminosidade com limite de 20%, detecta a borda de movimento do PIR e publica eventos MQTT. O botão SOS também gera evento e acende o LED. O dashboard mostra estados e registra eventos, com aviso visual de evento recebido. | 0,5 / 0,5 |
| 03 — Explicação | Sensores, pinos, grandezas, regra de decisão e efeito no sistema estão descritos abaixo. | 0,5 / 0,5 |

**Potencial: 2,0 / 2,0 pontos**, sujeito à execução da simulação e à apresentação das evidências ao avaliador.

## 1. Sensores simulados

### Sensor LDR — luminosidade (GPIO 34)

- **O que mede:** intensidade de luz do ambiente.
- **Como é lido:** entrada analógica por `analogRead(PINO_LDR)`, com resolução de 12 bits (0 a 4095).
- **Conversão:** o firmware converte a leitura ADC para um percentual de 0 a 100.
- **Regra de decisão:** `LIMITE_PONTO_ESCURO = 20`; quando a luminosidade calculada fica abaixo de 20%, a condição de ponto escuro é ativada.
- **Efeito:** publica o estado em `<ROOT>/current/low_light` e publica um evento `lighting_event` em `<ROOT>/events/lighting/...`. Quando a luminosidade volta ao normal, publica evento com `active:false`.
- **Periodicidade:** leitura automática a cada 5 segundos.

### Sensor PIR — movimento (GPIO 27)

- **O que detecta:** movimento no campo de detecção do sensor.
- **Como é lido:** entrada digital; o nível HIGH indica detecção de movimento e LOW indica ausência de movimento.
- **Tratamento:** o firmware aguarda 3 segundos de aquecimento e aplica debounce de 100 ms para evitar oscilações rápidas.
- **Regra de decisão:** quando o sensor está armado e muda para HIGH de forma estável, o firmware considera movimento detectado.
- **Efeito:** publica `true` em `<ROOT>/current/motion` e um evento `motion_event` em `<ROOT>/events/motion/...`. Quando retorna a LOW, publica `false` no estado atual.

## 2. Entrada complementar: botão SOS e atuador LED

O botão SOS no GPIO 12 é uma entrada manual, não um dos dois sensores exigidos. Ao ser pressionado depois da inicialização e do debounce de 50 ms, o firmware:

1. acende o LED conectado ao GPIO 2;
2. publica um evento `sos` em `<ROOT>/events/sos/...`;
3. publica o estado de emergência em `<ROOT>/current/emergency`.

Ao liberar o botão, apaga o LED e publica estado de emergência `false`. Isso demonstra uma ação adicional do sistema além dos sensores ambientais.

## 3. Fluxo de dados

1. LDR e PIR fornecem leituras ao ESP32.
2. O firmware aplica regras locais (limite de luminosidade, debounce e estado armado do PIR).
3. O ESP32 publica telemetria, estados atuais e eventos no broker MQTT.
4. O dashboard conecta ao broker por MQTT sobre WebSocket seguro (WSS), assina o tópico raiz e atualiza os indicadores, a lista de eventos e o aviso visual.

Tópico raiz:

```text
smartcity/security/demo-esp32-sos-74A9
```

Tópicos principais:

| Tópico | Conteúdo |
|---|---|
| `<ROOT>/telemetry` | ADC, luminosidade percentual, `dark`, `motion` e diagnóstico |
| `<ROOT>/current/low_light` | Estado atual de baixa luminosidade |
| `<ROOT>/current/motion` | Estado atual de movimento |
| `<ROOT>/current/emergency` | Estado atual do SOS |
| `<ROOT>/events/lighting/...` | Transições da condição de iluminação |
| `<ROOT>/events/motion/...` | Detecções de movimento |
| `<ROOT>/events/sos/...` | Acionamentos do botão SOS |

## 4. Como demonstrar ao avaliador

1. Abra o [projeto Smart City Security no Wokwi](https://wokwi.com/projects/480294036688727041).
2. Mostre o LDR ligado ao GPIO 34 e o PIR ao GPIO 27, além do ESP32.
3. Inicie a simulação e aguarde o aquecimento do PIR (3 segundos) e a primeira leitura do LDR.
4. Altere a iluminação do LDR até a luminosidade ficar abaixo de 20%. Mostre no Monitor Serial o diagnóstico de ponto escuro e a publicação do evento de iluminação.
5. Acione o PIR. Mostre a mensagem de movimento detectado e a publicação do evento correspondente.
6. Abra o dashboard e mostre os estados atualizados, a mensagem de evento recebida e a conexão MQTT.
7. Opcionalmente, pressione o botão SOS para mostrar o LED, o estado de emergência e o evento SOS.

## 5. Pontos de atenção para uma demonstração correta

- A conversão do ADC para percentual depende da orientação elétrica do LDR no circuito. Para demonstrar o alerta, confirme no Monitor Serial que a leitura calculada fica abaixo de 20%; se o sentido estiver invertido em relação à luz, será necessário ajustar o mapeamento ou a ligação.
- O PIR só é considerado após o período de aquecimento e o debounce. Não espere um evento instantâneo ao iniciar a simulação.
- O firmware publica os eventos com retenção MQTT (`retained=true`). Assim, um dashboard que se conecta depois pode receber eventos antigos retidos. Apresente-os como eventos recebidos/registrados, não necessariamente como uma detecção que acabou de ocorrer.
- O dashboard exibe os dados que recebe. A decisão de limiar de luminosidade e a detecção de borda do PIR são realizadas pelo firmware no ESP32.
- O broker é público e destinado à demonstração. Não use dados pessoais, credenciais ou coordenadas reais. As coordenadas no firmware são fixas e fictícias.

## Conclusão

O firmware apresentado implementa dois sensores distintos (LDR e PIR), regras de decisão para baixa luminosidade e movimento, publicação de telemetria/estados/eventos por MQTT e integração com um dashboard. Portanto, a solução contempla os três itens da rubrica; a apresentação deve comprovar a execução no Wokwi e a chegada dos eventos ao painel.
