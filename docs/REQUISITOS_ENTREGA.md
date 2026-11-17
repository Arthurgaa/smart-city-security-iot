# Parte 6 de 6 — Simulação de IoT (2 pontos)

Este documento apresenta as evidências do projeto Smart City Security para os três critérios de avaliação da etapa de IoT, com base no firmware ESP32, na integração MQTT e no dashboard.

## Resultado por critério

| Critério | Evidência concreta no projeto | Pontuação prevista |
|---|---|---:|
| 01 — Simulação | Dois sensores distintos no ESP32: LDR no GPIO 34 e PIR no GPIO 27. O ESP32 lê o LDR por ADC e o PIR por entrada digital; envia telemetria e estados por MQTT. | 1,0 / 1,0 |
| 02 — Uso dos dados | O firmware compara luminosidade com limite de 20%, detecta movimento e publica eventos MQTT. O botão SOS gera evento e acende o LED. Dashboard e backend consomem os mesmos eventos; o backend deve persistir os dados no banco. | 0,5 / 0,5 |
| 03 — Explicação | Sensores, pinos, grandezas, regra de decisão, fluxo MQTT e efeito no sistema estão descritos abaixo. | 0,5 / 0,5 |

**Potencial: 2,0 / 2,0 pontos**, sujeito à execução da simulação e à apresentação das evidências ao avaliador. A persistência no banco depende da implementação e execução do consumidor MQTT no backend.

## 1. Sensores simulados

### Sensor LDR — luminosidade (GPIO 34)

- **O que mede:** intensidade de luz do ambiente.
- **Como é lido:** entrada analógica por `analogRead(PINO_LDR)`, com resolução de 12 bits (0 a 4095).
- **Conversão:** o firmware converte a leitura ADC para um percentual de 0 a 100.
- **Regra de decisão:** `LIMITE_PONTO_ESCURO = 20`; quando a luminosidade calculada fica abaixo de 20%, a condição de ponto escuro é ativada.
- **Efeito:** publica o estado em `<ROOT>/current/low_light`, publica evento `lighting_event` em `<ROOT>/events/lighting/...` e envia telemetria periódica com `luminosidade_pct`.
- **Periodicidade:** leitura automática a cada 5 segundos.

### Sensor PIR — movimento (GPIO 27)

- **O que detecta:** movimento no campo de detecção do sensor.
- **Como é lido:** entrada digital; HIGH indica detecção e LOW indica ausência de movimento.
- **Tratamento:** o firmware aguarda 3 segundos de aquecimento e aplica debounce de 100 ms.
- **Regra de decisão:** após o aquecimento, o sensor é armado. Cada mudança estável de estado é registrada: HIGH representa movimento detectado e LOW representa retorno à condição sem movimento.
- **Efeito:** publica o estado em `<ROOT>/current/motion`, um evento `motion_event` em `<ROOT>/events/motion/...` para cada transição (HIGH com `active:true` e LOW com `active:false`) e o estado PIR na telemetria. As duas transições já estão implementadas no firmware versionado.

## 2. Entrada complementar: botão SOS e atuador LED

O botão SOS no GPIO 12 é uma entrada manual, não um dos dois sensores exigidos. Ao ser pressionado depois da inicialização e do debounce de 50 ms, o firmware:

1. acende o LED conectado ao GPIO 2;
2. publica evento `sos` em `<ROOT>/events/sos/...`;
3. publica o estado de emergência em `<ROOT>/current/emergency`.

Ao liberar o botão, apaga o LED e publica estado de emergência `false`.

## 3. Fluxo de dados e persistência

1. LDR e PIR fornecem leituras ao ESP32.
2. O firmware aplica regras locais (limite de luminosidade, debounce e estado armado do PIR).
3. O ESP32 publica telemetria, estados atuais e eventos no broker MQTT.
4. O dashboard assina o tópico raiz e atualiza indicadores e histórico local em tempo real.
5. O backend Java também assina os tópicos MQTT, valida e mapeia as mensagens para sensores/eventos cadastrados e persiste os dados no banco.
6. A API disponibiliza os dados persistidos para consulta pelas aplicações autorizadas.

Tópico raiz:

```text
smartcity/security/demo-esp32-sos-74A9
```

Tópicos principais:

| Tópico | Conteúdo |
|---|---|
| `<ROOT>/telemetry` | ADC, luminosidade percentual, `dark`, `motion` e diagnóstico |
| `<ROOT>/current/low_light` | Estado atual de baixa luminosidade (retido) |
| `<ROOT>/current/motion` | Estado atual de movimento (retido) |
| `<ROOT>/current/emergency` | Estado atual do SOS (retido) |
| `<ROOT>/events/lighting/...` | Transições da condição de iluminação |
| `<ROOT>/events/motion/...` | Eventos de detecção/normalização do PIR |
| `<ROOT>/events/sos/...` | Acionamentos do botão SOS |

## 4. Como demonstrar ao avaliador

1. Abra o [projeto Smart City Security no Wokwi](https://wokwi.com/projects/480294036688727041).
2. Mostre o LDR ligado ao GPIO 34 e o PIR ao GPIO 27.
3. Inicie a simulação e aguarde o aquecimento do PIR (3 segundos) e a primeira leitura do LDR.
4. Altere a iluminação até a luminosidade ficar abaixo de 20%. Mostre o diagnóstico e o evento de iluminação no dashboard.
5. Acione o PIR. Mostre a detecção e o evento correspondente.
6. Abra o dashboard e mostre os estados atualizados e a conexão MQTT.
7. Mostre os logs do backend recebendo os tópicos e consulte os registros persistidos no banco/API. Essa etapa só pode ser demonstrada depois que o consumidor MQTT estiver implementado e configurado.
8. Opcionalmente, pressione o botão SOS para mostrar o LED, o estado de emergência e o evento SOS.

## 5. Pontos de atenção

- A conversão do ADC para percentual depende da orientação elétrica do LDR no circuito. Confirme no Monitor Serial que a leitura calculada fica abaixo de 20% para demonstrar o alerta.
- O PIR só é considerado após aquecimento e debounce.
- Estados em `current/...` são mensagens retidas; não devem ser tratados como eventos históricos novos em cada reconexão.
- O dashboard atual guarda seu histórico no localStorage do navegador. Isso não substitui a persistência no banco feita pelo backend.
- O broker é público e destinado à demonstração. Não use dados pessoais, credenciais ou coordenadas reais.

## Conclusão

A solução utiliza LDR e PIR, regras de decisão no ESP32, publicação MQTT e dashboard em tempo real. A persistência compartilhada será realizada pelo consumidor MQTT do backend, que deve validar e registrar leituras/eventos no banco. A apresentação deve comprovar a execução no Wokwi, a chegada das mensagens ao dashboard e o registro no backend.
