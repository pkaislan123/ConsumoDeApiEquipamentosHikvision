/************************************************************
 * Projeto: Integração ESP32/ESP8266 com Central PIMA
 * Autor: Aislan Silva Costa
 * Descrição:
 *   Este firmware realiza comunicação periódica entre um 
 *   dispositivo ESP (ESP32 ou ESP8266) e uma central PIMA 
 *   de alarme, utilizando autenticação HTTP Digest.
 * 
 *   O sistema escolhe automaticamente entre Wi-Fi e Ethernet,
 *   conforme prioridade configurada, e realiza requisições 
 *   GET em endpoints de zonas e partições.
 ************************************************************/

#include <EEPROM.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include <ArduinoHttpClient.h>

// ======== BIBLIOTECAS DE REDE ========
#ifdef ESP32
  #include <WiFi.h>
  #include <WebServer.h>
  #include "SPIFFS.h"
  #include <Update.h>
  #include <esp_system.h>
  #include <esp_task_wdt.h>
  #include <ETH.h>  // Ethernet nativa do ESP32
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <Ethernet.h> // Ethernet externa via ENC28J60 ou W5500
  #include <FS.h>
#else
  #error "Placa não suportada. Escolha ESP32 ou ESP8266."
#endif

/************************************************************
 * CONFIGURAÇÕES DE REDE
 ************************************************************/
#define RANDOM_REG32 8556822323

const char* WIFI_SSID     = "Seu_SSID";
const char* WIFI_PASSWORD = "Sua_Senha";

// MAC fictício para Ethernet (ajuste se necessário)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient ethClient;
WiFiClient wifiClient;

/************************************************************
 * ENDPOINTS DO SISTEMA PIMA
 ************************************************************/
const char* url_rota_zonas      = "/ISAPI/SecurityCP/status/zones?format=json";
const char* url_rota_particoes  = "/ISAPI/SecurityCP/status/subSystems?format=json";

/************************************************************
 * FUNÇÕES DE AUTENTICAÇÃO DIGEST
 ************************************************************/

// Extrai parâmetros de cabeçalhos HTTP ("realm", "nonce", etc.)
String exractParam(String& authReq, const String& param, const char delimit) {
  int _begin = authReq.indexOf(param);
  if (_begin == -1) return "";
  return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

// Gera um cnonce (Client Nonce) aleatório
String getCNonce(const int len) {
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  String s = "";
  for (int i = 0; i < len; ++i) s += alphanum[rand() % (sizeof(alphanum) - 1)];
  return s;
}

// Monta o cabeçalho Authorization: Digest ... conforme RFC 2617
String getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter) {
  String realm  = exractParam(authReq, "realm=\"", '"');
  String nonce  = exractParam(authReq, "nonce=\"", '"');
  String cNonce = getCNonce(8);

  char nc[9];
  snprintf(nc, sizeof(nc), "%08x", counter);

  MD5Builder md5;
  md5.begin();
  md5.add(username + ":" + realm + ":" + password);
  md5.calculate();
  String h1 = md5.toString();

  md5.begin();
  md5.add(method + ":" + uri);
  md5.calculate();
  String h2 = md5.toString();

  md5.begin();
  md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":auth:" + h2);
  md5.calculate();
  String response = md5.toString();

  return "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce + 
         "\", uri=\"" + uri + "\", algorithm=\"MD5\", qop=auth, nc=" + String(nc) + 
         ", cnonce=\"" + cNonce + "\", response=\"" + response + "\"";
}

/************************************************************
 * GERENCIAMENTO DE CONEXÕES
 ************************************************************/

// ---- Conexão Wi-Fi ----
void connectWiFi() {
  Serial.println("\n[WiFi] Iniciando conexão...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi conectado com sucesso!");
    Serial.print("IP Wi-Fi: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Falha ao conectar ao Wi-Fi!");
  }
}

// ---- Conexão Ethernet ----
void connectEthernet() {
  Serial.println("\n[Ethernet] Inicializando...");
  Ethernet.begin(mac);

  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("✅ Ethernet conectada!");
    Serial.print("IP Ethernet: ");
    Serial.println(Ethernet.localIP());
  } else {
    Serial.println("❌ Falha: Cabo desconectado ou erro de inicialização.");
  }
}

/************************************************************
 * SETUP INICIAL
 ************************************************************/
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n===== Inicializando Sistema =====");

  // Inicializa o sistema de arquivos SPIFFS
  if (!SPIFFS.begin()) Serial.println("❌ Erro ao montar SPIFFS!");
  else Serial.println("✅ SPIFFS montado com sucesso.");

  randomSeed(RANDOM_REG32);

#if defined(ESP32)
  connectWiFi();      // Wi-Fi nativo
#elif defined(ESP8266)
  connectWiFi();      // Conexão Wi-Fi
  delay(2000);
  connectEthernet();  // Ethernet opcional
#endif
}

/************************************************************
 * LOOP PRINCIPAL
 ************************************************************/
void loop() {
  buscarAcoesCentral();  // Executa busca periódica na central
}

/************************************************************
 * FUNÇÕES DE SINCRONIZAÇÃO DE DADOS
 ************************************************************/

// Verifica tempo e dispara sincronizações
void buscarAcoesCentral() {
  if (millis() - tempo_buscar_acoes_central > 1500 && validarDadosCentral()) {
    buscarAcoes(false); // 🔹 Busca por Zonas
    buscarAcoes(true);  // 🔹 Busca por Partições
    tempo_buscar_acoes_central = millis();
  }
}

// Função genérica: faz requisição via Wi-Fi ou Ethernet
void buscarAcoes(bool isParticao) {
  if (!((isETH && flag) || WiFi.status() == WL_CONNECTED)) return;
  if (!isZones && !isParticao) return;

  rota1 = isParticao;
  url_rota = (char*)(rota1 ? url_rota_particoes : url_rota_zonas);

  if (primeiro_uso_global != 1 || configurado_central_global != 1) return;

  bool wifiPrioritario = configurado_prioridade_global != 0;
  String carga = "errou";

  // Define prioridade de rede
  if (wifiPrioritario) {
    carga = tentarConexaoWiFi();
    if (carga == "errou") carga = tentarConexaoEthernet();
  } else {
    carga = tentarConexaoEthernet();
    if (carga == "errou") carga = tentarConexaoWiFi();
  }

  // Processa resposta válida
  if (carga.length() > 10 && carga != "errou") processarStatusCentral(carga);
}

/************************************************************
 * ROTINAS DE REQUISIÇÃO HTTP (GENÉRICAS)
 ************************************************************/

// Wrapper genérico para Wi-Fi
String tentarConexaoWiFi() {
  if (WiFi.status() != WL_CONNECTED) return "errou";
  String carga = openConnectionGeneric(wifiClient, "Wi-Fi");
  if (carga == "errou") Serial.println("[WiFi] Falha na requisição.");
  return carga;
}

// Wrapper genérico para Ethernet
String tentarConexaoEthernet() {
  if (Ethernet.linkStatus() == LinkOFF || !isETH || !flag) return "errou";
  String carga = openConnectionGeneric(ethClient, "Ethernet");
  if (carga == "errou") Serial.println("[Ethernet] Falha na requisição.");
  return carga;
}

/************************************************************
 * openConnectionGeneric()
 * Função unificada para Wi-Fi e Ethernet
 ************************************************************/
String openConnectionGeneric(Client& client, const char* tipoConexao) {
  String ip_central   = String(enderecoIpCentralGlobal);
  int porta           = String(portaCentralGlobal).toInt();
  String response     = "";

  Serial.printf("[HTTP-%s] Solicitando dados de %s...\n", tipoConexao, rota1 ? "partições" : "zonas");

  // Tenta conectar à central
  if (!client.connect(ip_central.c_str(), porta)) {
    Serial.printf("❌ Falha na conexão via %s.\n", tipoConexao);
    return "errou";
  }

  // Envia requisição inicial (sem autenticação)
  client.printf("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", url_rota, ip_central.c_str());
  unsigned long tempoInicio = millis();
  String linha;

  // Aguarda cabeçalho de autenticação
  while (millis() - tempoInicio < TEMPO_ISPEIRA) {
    while (client.available()) {
      linha = client.readStringUntil('\n');
      linha.trim();

      if (linha.startsWith("WWW-Authenticate:")) {
        // Monta autenticação Digest e refaz a requisição
        String auth = getDigestAuth(linha, String(usuarioCentralGlobal), String(senhaCentralGlobal), "GET", String(url_rota), 1);
        client.stop();
        delay(100);

        if (!client.connect(ip_central.c_str(), porta)) return "errou";
        client.printf("GET %s HTTP/1.1\r\nHost: %s\r\nAuthorization: %s\r\nCache-Control: no-cache\r\nConnection: keep-alive\r\n\r\n", url_rota, ip_central.c_str(), auth.c_str());

        // Lê resposta completa
        String resposta;
        unsigned long inicio2 = millis();
        while (millis() - inicio2 < TEMPO_ISPEIRA) {
          while (client.available()) {
            char c = client.read();
            resposta += c;
            inicio2 = millis();
          }
          if (!client.connected()) break;
          yield();
        }

        // Extrai corpo JSON
        int i1 = resposta.indexOf("{");
        int i2 = resposta.lastIndexOf("}");
        if (i1 >= 0 && i2 > i1) response = resposta.substring(i1, i2 + 1);
        client.stop();
        return response;
      }
    }
    if (!client.connected()) break;
    yield();
  }

  client.stop();
  return "errou";
}
