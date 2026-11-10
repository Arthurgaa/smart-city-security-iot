# Smart City Security — IoT

Projeto demonstrativo de segurança urbana com ESP32, sensores e comunicação MQTT. Este repositório reúne os materiais de integração para que aplicações e dashboards possam consumir os dados publicados pelo dispositivo.

> **Firmware:** mantido no Wokwi. O código-fonte do firmware e o arquivo de circuito não fazem parte deste repositório.

## Links

- [Projeto no Wokwi](https://wokwi.com/projects/480294036688727041)
- [Dashboard web](./dashboard/)
- [Guia de integração MQTT](./docs/INTEGRACAO_APP.md)
- [Requisitos da entrega — Parte 6: Simulação de IoT](./docs/REQUISITOS_ENTREGA.md)

## Estrutura

```text
smart-city-security-iot/
├── README.md
├── CONTRIBUTING.md
├── dashboard/
│   └── index.html
└── docs/
    └── INTEGRACAO_APP.md
```

## Funcionalidades demonstradas

- Leitura de dois sensores: LDR (luminosidade) e PIR (movimento).
- Alertas para baixa luminosidade (abaixo de 20%) e detecção de movimento.
- Recepção de telemetria e eventos SOS, movimento e iluminação por MQTT.
- Estado atual de emergência, movimento e baixa luminosidade.
- Eventos de SOS, movimento e iluminação.
- Atualização em tempo real por MQTT sobre WebSocket.

## Executar o dashboard

Abra `dashboard/index.html` em um navegador com acesso à internet. O dashboard usa bibliotecas carregadas por CDN e conecta ao broker público de demonstração configurado no arquivo.

Para integrar outro dispositivo, ajuste o tópico raiz e mantenha o formato das mensagens documentado em [docs/INTEGRACAO_APP.md](./docs/INTEGRACAO_APP.md).

## Segurança e limitações

A configuração padrão utiliza um broker MQTT público para fins didáticos. Não publique dados pessoais, credenciais, coordenadas reais ou informações sensíveis. Para uso real, utilize broker privado com autenticação, TLS, ACLs por tópico e política de retenção adequada.

As coordenadas exibidas no mapa são fixas e servem apenas como demonstração; não representam GPS do dispositivo. O histórico do dashboard é armazenado localmente no navegador e não é um banco de dados compartilhado.

## Contribuição

Consulte [CONTRIBUTING.md](./CONTRIBUTING.md).
