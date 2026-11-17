# Como contribuir

Obrigado por contribuir com o Smart City Security.

## Fluxo sugerido

1. Faça um fork do repositório ou crie uma branch para sua alteração.
2. Mantenha as mudanças focadas e documente novos tópicos ou formatos MQTT.
3. Para alterações no firmware ou circuito, atualize os arquivos em `firmware/` e teste a simulação no Wokwi.
4. Teste o dashboard em um navegador atualizado; prefira utilizar um servidor local, como o Live Server.
5. Abra um Pull Request explicando o que mudou e como testar.

## Estrutura do projeto

- `firmware/smart_city_security.ino`: código-fonte do ESP32.
- `firmware/diagram.json`: componentes e conexões do circuito Wokwi.
- `dashboard/`: interface web.
- `docs/`: documentação dos contratos MQTT e integrações.

O projeto também possui uma [simulação no Wokwi](https://wokwi.com/projects/480294036688727041). Mantenha os arquivos versionados em `firmware/` consistentes com a simulação.

## Convenções

- Preserve os nomes e a hierarquia dos tópicos MQTT existentes, salvo quando a mudança for intencional e documentada.
- Ao alterar o payload, atualize também `docs/INTEGRACAO_APP.md` e, quando aplicável, `docs/CONSUMIDOR_MQTT_BACKEND.md`.
- Não inclua senhas, tokens, dados pessoais ou coordenadas reais.
- Ao modificar o firmware ou o circuito, atualize os arquivos correspondentes em `firmware/` e valide as ligações entre os GPIOs e os componentes.
- Prefira HTML, CSS e JavaScript simples para manter o dashboard fácil de executar.

## Relato de problemas

Inclua os passos para reproduzir, o navegador utilizado, o tópico MQTT envolvido e um exemplo anonimizado do payload. Para problemas de hardware ou simulação, informe também o componente, o GPIO e os passos para reproduzir no Wokwi.
