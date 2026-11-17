# Smart City Security — IoT

Projeto demonstrativo de segurança urbana com ESP32, sensores, MQTT e dashboard web. O ESP32 publica os eventos em um broker MQTT; o dashboard e o backend devem consumir esses mesmos tópicos. O backend é responsável por validar, mapear e persistir as leituras no banco de dados.

## Links

- [Projeto no Wokwi](https://wokwi.com/projects/480294036688727041)
- [Firmware ESP32](./firmware/smart_city_security.ino)
- [Diagrama de circuito Wokwi](./firmware/diagram.json)
- [Dashboard web](./dashboard/)
- [Contrato MQTT do projeto](./docs/INTEGRACAO_APP.md)
- [Guia para o consumidor MQTT no backend](./docs/CONSUMIDOR_MQTT_BACKEND.md)
- [Integração SOS](./docs/INTEGRACAO_API_SOS.md)
- [Requisitos da entrega — Parte 6: Simulação de IoT](./docs/REQUISITOS_ENTREGA.md)

## Arquitetura

```text
                         ┌──> Dashboard web (tempo real)
ESP32 ── publica MQTT ──> Broker MQTT
                         └──> Backend Java ──> Banco de dados
                              (valida, mapeia
                               e persiste)
```

O ESP32 não chama a API REST e não precisa armazenar a API Key do backend. O dashboard consome MQTT diretamente. O backend deve implementar seu próprio cliente MQTT e persistir as mensagens válidas.

## Funcionalidades

- Leitura de LDR (luminosidade) e PIR (movimento).
- Regra de baixa luminosidade abaixo de 20%.
- Eventos de SOS, movimento e iluminação publicados por MQTT.
- Eventos PIR para início e encerramento do movimento.
- Estado atual de emergência, movimento e baixa luminosidade.
- Dashboard web atualizado em tempo real por MQTT sobre WebSocket.
- Identificação de eventos por `event_id` e identificador de inicialização `boot_id`.

## Executar a simulação no Wokwi

1. Abra o [projeto no Wokwi](https://wokwi.com/projects/480294036688727041) ou importe os arquivos desta pasta no Wokwi.
2. Utilize o firmware `firmware/smart_city_security.ino`.
3. Mantenha o arquivo `firmware/diagram.json` junto ao projeto.
4. Instale a biblioteca Arduino **PubSubClient** no ambiente Arduino, se necessário.
5. Inicie a simulação e acompanhe o Monitor Serial em 115200 baud.

O firmware foi fornecido para ESP32 DevKit-C e utiliza a rede `Wokwi-GUEST`, o broker público de demonstração `broker.emqx.io:1883` e o tópico raiz definido no próprio código.

## Executar o dashboard

Sirva a pasta do projeto por um servidor HTTP local e abra `dashboard/index.html` pelo endereço fornecido. No VS Code, você pode usar a extensão Live Server (clique com o botão direito no arquivo e selecione **Open with Live Server**). Evite abrir o arquivo diretamente pelo protocolo `file://`, pois o navegador pode restringir o carregamento de recursos externos.

O dashboard usa bibliotecas carregadas por CDN, tiles do OpenStreetMap e conecta ao broker público de demonstração configurado no arquivo. Consulte [docs/INTEGRACAO_APP.md](./docs/INTEGRACAO_APP.md) para tópicos e formatos das mensagens.

## Integração com o backend Java

O backend deve implementar um consumidor MQTT independente, validar o formato das mensagens, deduplicar eventos usando `event_id` e persistir os eventos aplicáveis. Mensagens de estado atual retidas (`current/...`) representam o estado mais recente e não devem ser tratadas como novos eventos históricos.

Consulte [docs/CONSUMIDOR_MQTT_BACKEND.md](./docs/CONSUMIDOR_MQTT_BACKEND.md) e [docs/INTEGRACAO_API_SOS.md](./docs/INTEGRACAO_API_SOS.md).

## Segurança e limitações

- A configuração padrão utiliza um broker MQTT público e sem autenticação para fins didáticos. Não publique dados pessoais, credenciais, coordenadas reais ou informações sensíveis.
- Para uso real, utilize broker privado com autenticação, TLS, ACLs por tópico e política de retenção adequada.
- As coordenadas no firmware são fixas e fictícias; não representam GPS real.
- Eventos históricos são publicados com QoS 0, sem garantia de entrega.
- O histórico exibido pelo dashboard não substitui a persistência centralizada do backend.

## Contribuição

Consulte [CONTRIBUTING.md](./CONTRIBUTING.md).
