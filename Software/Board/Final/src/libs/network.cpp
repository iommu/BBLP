#include "network.hpp"

NetworkHandler::NetworkHandler() {
  // Shutdown wifi to save power
  disconn();
}

String NetworkHandler::getQuestions() {
  connect();
  //
  String ret = "";
  HTTPClient client;

  client.begin(server_uri + "/pull");

  if (client.GET()) { // if response code > 0
    Serial.println("Got JSON from server");
    ret = client.getString();
  } else {
    Serial.println("Err getting JSON from server");
    Serial.println("Server URI = " + server_uri);
  }

  client.end();
  //
  disconn();
  return ret;
}

void NetworkHandler::uploadAnswers(String answer) {
  connect();
  //
  HTTPClient client;

  client.begin(server_uri + "/push");
  client.POST(answer);

  client.end();
  //
  disconn();
  return;
}

void NetworkHandler::updateTime() {
  connect();
  //
  configTime(36000 /*Syd offset*/, 0 /*daylight 0/3600*/, "pool.ntp.org");
  // 
  disconn();
}

void NetworkHandler::connect() {
  // Start ADC needed for WIFI
  adc_power_on();
  WiFi.disconnect(false); // Reconnect the network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  // Wait while connecting
  Serial.print("Connecting to WIFI : ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("WiFi connected with IP addr :");
  Serial.println(WiFi.localIP());
}

void NetworkHandler::disconn() {
  // Disconnect and power down wifi
  adc_power_off();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}