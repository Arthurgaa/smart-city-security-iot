# Smart City Security — IoT

Projeto demonstrativo de segurança urbana com ESP32, sensores, MQTT e dashboard web. O ESP32 publica os eventos em um broker MQTT; o dashboard e o backend devem consumir esses mesmos tópicos. O backend é responsável por validar, mapear e persistir as leituras no banco de dados.

> **Firmware:** mantido no Wokwi. O código-fonte do firmware e o arquivo de circuito não fazem parte deste repositório.

## Links

- [Projeto no Wokwi](https://wokwi.com/projects/480294036688727041)
- [Dashboard web](./dashboard/)
- [Contrato MQTT do projeto](./docs/INTEGRACAO_APP.md)
- [Guia para o consumidor MQTT no backend](./docs/CONSUMIDOR_MQTT_BACKEND.md)
- [Requisitos da entrega — Parte 6: Simulação de IoT](./docs/REQUISITOS_ENTREGA.md)

## Arquitetura definida

```text
                         ┌──> Dashboard web (tempo real)
ESP32 ── publica MQTT ──> Broker MQTT
                         └──> Backend Java ──> Banco de dados
                              (valida, mapeia
                               e persiste)
```

O ESP32 não chama a API REST e não precisa armazenar a API Key do backend. O dashboard continua consumindo MQTT diretamente. O backend precisa implementar seu próprio cliente MQTT e persistir as mensagens válidas.

## Funcionalidades demonstradas

- Leitura de LDR (luminosidade) e PIR (movimento).
- Regra de baixa luminosidade abaixo de 20%.
- Eventos de SOS, movimento e iluminação publicados por MQTT.
- Estado atual de emergência, movimento e baixa luminosidade.
- Dashboard web atualizado em tempo real por MQTT sobre WebSocket.
- Persistência no banco realizada pelo backend consumidor MQTT (integração a implementar no repositório da API).

## Executar o dashboard

Abra `dashboard/index.html` em um navegador com acesso à internet. O dashboard usa bibliotecas carregadas por CDN e conecta ao broker público de demonstração configurado no arquivo.

Consulte [docs/INTEGRACAO_APP.md](./docs/INTEGRACAO_APP.md) para tópicos e formatos das mensagens. Consulte [docs/CONSUMIDOR_MQTT_BACKEND.md](./docs/CONSUMIDOR_MQTT_BACKEND.md) para o trabalho necessário no backend.

## Segurança e limitações

A configuração padrão utiliza um broker MQTT público para fins didáticos. Não publique dados pessoais, credenciais, coordenadas reais ou informações sensíveis. Para uso real, utilize broker privado com autenticação, TLS, ACLs por tópico e política de retenção adequada.

As coordenadas exibidas no mapa são fixas e servem apenas como demonstração; não representam GPS real. O histórico do dashboard é armazenado localmente no navegador e não é o banco de dados compartilhado da aplicação.

## Contribuição

Consulte [CONTRIBUTING.md](./CONTRIBUTING.md).
