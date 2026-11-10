# Como contribuir

Obrigado por contribuir com o Smart City Security.

## Fluxo sugerido

1. Faça um fork do repositório ou crie uma branch para sua alteração.
2. Mantenha as mudanças focadas e documente novos tópicos ou formatos MQTT.
3. Teste o dashboard em um navegador atualizado.
4. Abra um Pull Request explicando o que mudou e como testar.

## Convenções

- Preserve os nomes e a hierarquia dos tópicos MQTT existentes, salvo quando a mudança for intencional e documentada.
- Ao alterar o payload, atualize também `docs/INTEGRACAO_APP.md`.
- Não inclua senhas, tokens, dados pessoais ou coordenadas reais.
- Não adicione `sketch.ino` nem `diagram.json`: o firmware e o circuito são mantidos no projeto Wokwi.
- Prefira HTML, CSS e JavaScript simples para manter o dashboard fácil de executar.

## Relato de problemas

Inclua os passos para reproduzir, o navegador utilizado, o tópico MQTT envolvido e um exemplo anonimizado do payload.
