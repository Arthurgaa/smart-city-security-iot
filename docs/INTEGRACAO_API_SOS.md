# Integração do ESP32 com a API SOS Maria da Penha

Este guia complementa a integração MQTT do dashboard. O ESP32 pode continuar publicando MQTT para o dashboard e, adicionalmente, enviar leituras por HTTP para a API REST do aplicativo.

## 1. Endpoint correto

No backend SOS Maria da Penha, a rota documentada é:

```http
POST /sensors/{id}/readings
```

Atenção: o caminho implementado no backend é `/sensors/{id}/readings` (não `/sensores/{id}/leituras`).

O corpo da requisição contém somente `value`:

```json
{ "value": 18.5 }
```

Headers obrigatórios para o simulador IoT:

```http
Content-Type: application/json
X-API-Key: SUA_CHAVE_IOT
```

Resposta de sucesso: HTTP `201 Created`, com os dados da leitura e o campo `alert`.

## 2. Mapeamento dos sensores

A API aceita os seguintes tipos:

| Sensor físico | Tipo cadastrado na API | Valor enviado | Regra de alerta |
|---|---|---|---|
| LDR | `LIGHTING` | luminosidade de 0 a 100 | abaixo de 20 |
| PIR | `MOTION` | `1` com movimento, `0` sem movimento | valor 1 |

O botão SOS do projeto não é um dos tipos de sensor aceitos por essa rota. Mantenha o evento SOS no fluxo MQTT/dashboard ou combine com a equipe uma rota específica do backend para SOS. Não envie o botão como `MOTION`, pois isso mistura significados.

## 3. Preparação no backend

1. Execute a API conforme o README do backend: `mvn spring-boot:run`.
2. A API local fica em `http://localhost:8086`. Esse endereço só é acessível na máquina que está executando o backend.
3. Entre no Swagger em `http://localhost:8086/swagger-ui/index.html`.
4. Faça login como ADMIN e use `POST /sensors` para cadastrar um sensor `LIGHTING` e outro `MOTION`. Informe latitude e longitude válidas (a API exige esses campos).
5. Anote os IDs retornados pelo cadastro. Cada tipo precisa do seu próprio ID.
6. Configure a variável `IOT_API_KEY` no backend ou copie a chave gerada e exibida no log de inicialização, conforme o README do backend. Não publique essa chave no GitHub.
7. Para o Wokwi alcançar a API, disponibilize o backend por uma URL acessível pela internet. O endereço `localhost` do seu computador não funciona dentro do simulador. Use um ambiente de hospedagem ou túnel HTTPS de desenvolvimento.

Backend: https://github.com/stephanyborzi/Global-Solution---SOS-Maria-da-Penha-CitizenTech-

## 4. Código para acrescentar ao sketch ESP32

O firmware continua no Wokwi. Acrescente os includes abaixo no início do sketch:

```cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
```

Configure os dados de integração. Troque os valores de exemplo pelos dados reais do backend:

```cpp
const char* API_BASE_URL = "https://SUA-URL-PUBLICA";
const char* IOT_API_KEY = "SUA_CHAVE_IOT";
const long ID_SENSOR_LDR = 1;  // ID retornado por POST /sensors
const long ID_SENSOR_PIR = 2;  // ID retornado por POST /sensors
```

Adicione esta função ao sketch:

```cpp
bool enviarLeituraAPI(long sensorId, float valor) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] Wi-Fi desconectado; leitura nao enviada.");
    return false;
  }

  WiFiClientSecure client;
  // SOMENTE para teste/demonstração. Em produção, valide o certificado
  // TLS do servidor usando a CA correspondente; não use setInsecure().
  client.setInsecure();

  HTTPClient http;
  String url = String(API_BASE_URL) + "/sensors/" + String(sensorId) + "/readings";

  if (!http.begin(client, url)) {
    Serial.println("[API] Falha ao iniciar conexao HTTP.");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", IOT_API_KEY);

  String body = "{\"value\":" + String(valor, 1) + "}";
  int status = http.POST(body);

  if (status >= 200 && status < 300) {
    Serial.printf("[API] Leitura enviada. HTTP %d | sensor=%ld | value=%.1f\n",
                  status, sensorId, valor);
    http.end();
    return true;
  }

  Serial.printf("[API] Falha no envio. HTTP %d | resposta: %s\n",
                status, http.getString().c_str());
  http.end();
  return false;
}
```

### Onde chamar a função

- **LDR:** depois de calcular a luminosidade percentual (0 a 100), envie a leitura no mesmo ciclo de amostragem do LDR. Exemplo: `enviarLeituraAPI(ID_SENSOR_LDR, luminosidadePct);`
- **PIR:** quando o estado estável/debounced mudar, envie `1` para movimento detectado ou `0` para ausência de movimento. Exemplo: `enviarLeituraAPI(ID_SENSOR_PIR, movimento ? 1 : 0);`

Não envie o valor bruto ADC do LDR para a API: ela espera percentual entre 0 e 100. Não envie o PIR como `true/false`: a API espera número `1/0`.

A função deve ser chamada depois que o Wi-Fi estiver conectado. Evite fazer POST em todo loop do ESP32; envie LDR no intervalo de leitura já existente e PIR somente em mudanças de estado (ou em uma frequência combinada com o grupo).

## 5. Como testar

1. Confirme que a API está acessível pela URL pública configurada em `API_BASE_URL`.
2. Inicie o Wokwi e abra o Serial Monitor.
3. Verifique mensagens `[API] Leitura enviada. HTTP 201`.
4. No Swagger, consulte `GET /sensors/{id}/readings` para verificar o histórico.
5. Teste LDR abaixo de 20% e confirme `alert: true`; teste acima/igual a 20% e confirme `alert: false`.
6. Acione o PIR e confirme valor `1` com alerta; depois confirme valor `0` sem alerta.

## 6. Segurança e limitações

- Nunca coloque a chave IoT real em repositório público, README, dashboard ou prints.
- A chave `X-API-Key` autentica o simulador e deve ser configurada no backend.
- `client.setInsecure()` desativa a validação do certificado TLS e está indicado aqui apenas para demonstração em ambiente controlado. Em produção, use validação de certificado.
- Não use coordenadas fictícias como se fossem localização real. A API exige latitude/longitude no cadastro do sensor; alinhe com a equipe quais coordenadas de demonstração serão usadas.
- O MQTT e o HTTP têm funções diferentes neste projeto: MQTT continua alimentando o dashboard; HTTP envia leituras persistidas ao backend. O dashboard MQTT, por si só, não grava dados na API.
- O código acima é um trecho de integração para inserir no sketch existente no Wokwi. Os IDs, a URL pública e a chave precisam ser preenchidos com os valores do ambiente da equipe antes do teste.
