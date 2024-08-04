/****************************************************************************************************************************
  AsyncTCPServer.ino

  For RP2040W with CYW43439 WiFi

  AsyncTCP_RP2040W is a library for the RP2040W with CYW43439 WiFi

  Based on and modified from ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncTCP_RP2040W
  Licensed under GPLv3 license
 *****************************************************************************************************************************/

#include <AsyncTCP_RP2040W.h>

#include <vector>

#define TCP_PORT          5698

static std::vector<AsyncClient*> clients; // a list to hold all clients

char ssid[] = "your_ssid";        // your network SSID (name)
char pass[] = "12345678";         // your network password (use for WPA, or use as key for WEP), length must be 8+

int status = WL_IDLE_STATUS;

IPAddress serverIP;

/* clients events */
static void handleError(void* arg, AsyncClient* client, int8_t error)
{
  (void) arg;

  Serial.printf("\nConnection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

#define REPLY_SIZE      64

static void handleData(void* arg, AsyncClient* client, void *data, size_t len)
{
  (void) arg;

  Serial.printf("\nData received from client %s \n", client->remoteIP().toString().c_str());
  Serial.write((uint8_t*)data, len);

  // reply to client
  if (client->space() > REPLY_SIZE && client->canSend())
  {
    char reply[REPLY_SIZE];
    sprintf(reply, "You've connected to AsyncTCPServer @ %s", serverIP.toString().c_str());
    client->add(reply, strlen(reply));
    client->send();
  }
}

static void handleDisconnect(void* arg, AsyncClient* client)
{
  (void) arg;

  Serial.printf("\nClient %s disconnected\n", client->remoteIP().toString().c_str());
}

static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time)
{
  (void) arg;
  (void) time;

  Serial.printf("\nClient ACK timeout ip: %s\n", client->remoteIP().toString().c_str());
}


/* server events */
static void handleNewClient(void* arg, AsyncClient* client)
{
  (void) arg;

  Serial.printf("\nNew client has been connected to server, IP: %s", client->remoteIP().toString().c_str());

  // add to list
  clients.push_back(client);

  // register events
  client->onData(&handleData, NULL);
  client->onError(&handleError, NULL);
  client->onDisconnect(&handleDisconnect, NULL);
  client->onTimeout(&handleTimeOut, NULL);
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  serverIP = WiFi.localIP();
  Serial.print("Local IP Address: ");
  Serial.println(serverIP);
}

void setup()
{
  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  delay(200);

  Serial.print("\nStart AsyncTCP_Server on ");
  Serial.print(BOARD_NAME);
  Serial.print(" with ");
  Serial.println(SHIELD_TYPE);
  Serial.println(ASYNCTCP_RP2040W_VERSION);

  ///////////////////////////////////

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");

    // don't continue
    while (true);
  }

  Serial.print(F("Connecting to SSID: "));
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);

  delay(1000);

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED)
  {
    delay(500);

    // Connect to WPA/WPA2 network
    status = WiFi.status();
  }

  printWifiStatus();

  ///////////////////////////////////

  AsyncServer* server = new AsyncServer(TCP_PORT); // start listening on tcp port 7050

  server->onClient(&handleNewClient, server);
  server->begin();

  Serial.print(F("AsyncTCPServer is @ IP: "));
  Serial.print(serverIP);
  Serial.print(F(", port: "));
  Serial.println(TCP_PORT);
}

void loop()
{
}
