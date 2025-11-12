🔐 Sistema de Comunicação Segura com Central de Alarme via ESP32 / ESP8266

🚀 Projeto de integração IoT com autenticação HTTP Digest (MD5), desenvolvido para comunicação segura entre microcontroladores ESP32 ou ESP8266 e centrais de alarme profissionais PIMA, utilizando Ethernet e Wi-Fi com redundância automática.

🧠 Visão Geral do Projeto

Este projeto implementa um cliente embarcado capaz de se autenticar em uma API de central de alarme (PIMA) usando autenticação HTTP Digest — o mesmo protocolo usado em sistemas corporativos de segurança eletrônica.

O código foi desenvolvido em C++ no ambiente Arduino, utilizando bibliotecas nativas do ESP32/ESP8266 para garantir estabilidade, segurança e compatibilidade com hardware IoT.

O sistema é capaz de:

🛰️ Comunicar-se via Wi-Fi e Ethernet (com fallback automático)

🔐 Realizar autenticação Digest MD5 com a central

⚙️ Buscar dados de zonas e partições da central

💾 Armazenar logs e parâmetros em SPIFFS / EEPROM

📡 Processar e exibir os resultados em tempo real via Serial Monitor

⚙️ Especificações Técnicas
Recurso	Descrição
🔧 Microcontrolador	ESP32 / ESP8266 (NodeMCU)
🌐 Comunicação	Ethernet (W5500 ou ENC28J60) + Wi-Fi
🔐 Autenticação	HTTP Digest (MD5)
💾 Armazenamento	SPIFFS / EEPROM
📊 Dados obtidos	Zonas e Partições da central de alarme
🧰 Ambiente	Arduino IDE
🧩 Arquitetura do Sistema
+-----------------------------------------------+
|                 CENTRAL PIMA                  |
|  (API HTTP Digest - /ISAPI/SecurityCP/...)    |
+-------------------------^---------------------+
                          |
                 Comunicação Segura
                          |
        +--------------------------------+
        |            ESP32 / ESP8266     |
        |--------------------------------|
        | 🔹 Wi-Fi + Ethernet Fallback   |
        | 🔹 MD5 Digest Authentication   |
        | 🔹 SPIFFS e EEPROM Storage     |
        | 🔹 JSON Parsing (ArduinoJson)  |
        +--------------------------------+
                          |
                     Monitor Serial
                          |
                 Exibição de Status

🧠 Como o Sistema Funciona

1️⃣ Inicialização: O ESP inicia o Wi-Fi e tenta ativar a Ethernet.
2️⃣ Validação: O código verifica se há uma central configurada e disponível.
3️⃣ Autenticação:

Envia uma requisição GET sem autenticação.

Recebe o cabeçalho WWW-Authenticate com realm, nonce, qop.

Gera um hash MD5 com username:realm:password e um cnonce aleatório.

Envia nova requisição autenticada com Authorization: Digest ....
4️⃣ Requisição: O sistema acessa as rotas:

/ISAPI/SecurityCP/status/zones?format=json

/ISAPI/SecurityCP/status/subSystems?format=json
5️⃣ Processamento: O ESP decodifica o JSON retornado e processa os dados de status.

🧰 Bibliotecas Utilizadas

<WiFi.h> / <ESP8266WiFi.h> → Conexão Wi-Fi

<Ethernet.h> / <ETH.h> → Conexão Ethernet

<ArduinoJson.h> → Processamento JSON

<MD5Builder.h> → Geração do hash MD5

<EEPROM.h> → Armazenamento de variáveis persistentes

<SPIFFS.h> → Sistema de arquivos embarcado

<ArduinoHttpClient.h> → Requisições HTTP

⚡ Instalação e Execução
🔹 1. Configurar o ambiente

Baixe e instale o Arduino IDE (versão mais recente).

Instale o pacote ESP32/ESP8266 em Ferramentas > Placa > Gerenciador de Placas.

Adicione as bibliotecas necessárias em Sketch > Incluir Biblioteca > Gerenciar Bibliotecas....

🔹 2. Configurar o Wi-Fi

No início do código, altere as credenciais:

const char* WIFI_SSID = "Seu_SSID";
const char* WIFI_PASSWORD = "Sua_Senha";

🔹 3. Configurar a Central

Defina no código:

String enderecoIpCentralGlobal = "192.168.0.100";  // IP da central PIMA
String portaCentralGlobal = "80";                  // Porta HTTP
String usuarioCentralGlobal = "admin";             // Usuário
String senhaCentralGlobal = "12345";               // Senha

🔹 4. Upload do Código

1️⃣ Conecte o ESP via USB.
2️⃣ Selecione a placa correta em Ferramentas > Placa.
3️⃣ Clique em Carregar (Upload).

🔹 5. Monitoramento

Abra o Serial Monitor com baud rate de 115200.
Você verá logs como:

✅ Conectado ao Wi-Fi com sucesso!
buscando por zonas via wifi
reposta por zonas: { "zones": [ ... ] }

🧩 Funções-Chave
Função	Descrição
buscarAcoesCentral()	Controla o ciclo de requisições para zonas e partições
buscarAcoes(bool isParticao)	Requisição genérica com fallback de rede
openConnectionEth() / openConnectionWifi()	Conexões HTTP autenticadas
getDigestAuth()	Gera o cabeçalho HTTP Digest completo
processarStatusCentral()	Interpreta e processa a resposta JSON
💡 Aplicações Práticas

✅ Monitoramento remoto de centrais de alarme
✅ Gateways IoT para segurança eletrônica
✅ Automação predial e industrial
✅ Integração com sistemas em nuvem

🔒 Segurança e Confiabilidade

Este projeto implementa autenticação segura Digest (MD5), garantindo que:

As senhas nunca são enviadas em texto puro.

Cada requisição usa um nonce e cnonce aleatórios.

Os dados são transmitidos com integridade garantida.

 

👨‍💻 Autor

Aislan Silva Costa
Engenheiro de Sistemas Embarcados • Especialista em IoT e Segurança Eletrônica
📧 Contato: comprasaislan@gmail.com

🪪 Licença

Este projeto é distribuído sob a licença MIT, permitindo uso, modificação e distribuição livre, desde que os devidos créditos sejam mantidos.

🏷️ Hashtags

#IoT #ESP32 #ESP8266 #Arduino #SegurancaEletronica #HTTPDigest #MD5 #Ethernet #Automacao #PIMA #Ciberseguranca #DesenvolvimentoEmbarcado #Tecnologia #Inovacao #OpenSource
