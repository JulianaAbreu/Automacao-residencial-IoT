#include <UIPEthernet.h>
#include <utility/logging.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <Servo.h>

// Ethernet
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0x05 };
IPAddress ip(172, 16, 0, 100);
// MQTT
long lastReconnectAttempt = 0;
const char *server = "m11.cloudmqtt.com";
const char *user = "usuario";
const char *pass = "132";
const int port = 19604;


//Pinos
const int ledSend = 2;
const int ledReceive = 3;

const int btnLed = 6; //btn
const int leda = 4;
const int btnMot = 7;
const int mot = 8;
Servo servo1;

//Estados
int prevBtnStateLed = 0;
int prevBtnStateMotor = 0; //prevBtnsState

int ledOn = 1; //Estado LED
int btnOn = 0; //Estado botão

int pressMot = 160;
const int PORTA_ABERTA = 20;
const int PORTA_FECHADA = 120;
int portao = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  piscarLed(ledReceive);
  Serial.print("Callback: ");
  Serial.println(topic);

  char* payloadAsChar = payload;

  // Workaround para pequeno bug na biblioteca
  payloadAsChar[length] = 0;

  // Converter em tipo String para conveniência
  String topicStr = String(topic);
  String msg = String(payloadAsChar);
  Serial.print("Topic received: "); Serial.println(topic);
  Serial.print("Message: "); Serial.println(msg);

  // Dentro do callback da biblioteca MQTT,
  // devemos usar Serial.flush() para garantir que as mensagens serão enviadas
  Serial.flush();

  // https://www.arduino.cc/en/Reference/StringToInt
  int msgComoNumero = msg.toInt();

  Serial.print("Numero recebido: "); Serial.println(msgComoNumero);
  Serial.flush();

  if (topicStr == "ledcomando") {
    if (msgComoNumero == 0) {
      ledOn = 0;
    } else {
      ledOn = 1;
    }
    digitalWrite(leda, ledOn);
  }
  else if (topicStr == "motorcomando") {
    if (msgComoNumero == 0) {
      pressMot = PORTA_ABERTA;
    } else {
      pressMot = PORTA_FECHADA;
    }
    servo1.write(pressMot);

  }

}

EthernetClient ethClient;
PubSubClient client(server, port, callback, ethClient);

void setup()
{
  //Serial
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Serial ok");

  //Ethernet
  Ethernet.begin(mac);
  lastReconnectAttempt = 0;
  Serial.println("Tentando conectar...");
  //MQTT
  if (client.connect("arduino", user, pass)) {
    Serial.println("Conectado!");
    if (client.subscribe("ledcomando")) {
      Serial.println("Inscrito em ledcomando");
    }
    if (client.subscribe("motorcomando")) {
      Serial.println("Inscrito em motorcomando");
    }
  }
  else
  {
    Serial.println("Falhou!");
  }
  digitalWrite(ledReceive, LOW);
  digitalWrite(ledSend, LOW);

  pinMode(leda, OUTPUT); // Led
  digitalWrite(leda, ledOn);

  pinMode(btnLed, INPUT); // Botaoled

  servo1.attach(mot); // Motor
  pinMode(btnMot, INPUT); //BotaoMotor


}

void loop()
{
  Serial.flush();
  //MQTT
  if (!client.connected()) {
    Serial.println("Desconectado");
    long now = millis();
    if (now - lastReconnectAttempt > 2500) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }

  // ========== PARTE DO LED =======
  int btnStateLed = digitalRead(btnLed);

  if (btnStateLed && !prevBtnStateLed) {
    // Invertemos o estado do LED
    char ledChar;
    if (ledOn) {
      ledOn = 0;
    } else {
      ledOn = 1;
    }
    digitalWrite(leda, ledOn);
    publicarInt("led", ledOn);

  }

  prevBtnStateLed = btnStateLed;

  //==========PARTE DO MOTOR==============

  int btnStateMotor = digitalRead(btnMot);
  if (btnStateMotor && !prevBtnStateMotor) {
    // Invertemos o estado do motor
    if (pressMot == PORTA_ABERTA) {
      pressMot = PORTA_FECHADA;
      portao = 1;
    } else {
      pressMot = PORTA_ABERTA;
      portao = 0;
    }
    publicarInt("motor", portao);
  }

  servo1.write(pressMot);

  prevBtnStateMotor = btnStateMotor;
}

//FUNÇÕES
void publicarInt(char* topic, int num) {
  if (client.connected()) {
    if (num) {
      if (client.publish(topic, "1")) {
        Serial.print("Enviado: ");
        Serial.println(topic);
        piscarLed(ledSend);
      }
    } else {
      if (client.publish(topic, "0")) {
        Serial.print("Enviado: ");
        Serial.println(topic);
        piscarLed(ledSend);
      }
    }
  }
}

void piscarLed(int led) {
  digitalWrite(led, HIGH);
  delay(250);
  digitalWrite(led, LOW);
}

boolean reconnect() {

  Serial.println("Tentando conectar...");
  if (client.connect("arduino", user, pass)) {
    Serial.println("Conectado");
    if (client.subscribe("ledcomando")) {
      Serial.println("Inscrito em ledcomando");
    }
    if (client.subscribe("motorcomando")) {
      Serial.println("Inscrito em motorcomando");
    }
  }
  return client.connected();
}

