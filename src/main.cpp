#include <Arduino.h>
#include <Fuzzy.h>
#include <AntaresESPHTTP.h>
#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <dimmable_light.h>

// Dimmer
const int syncPin = 13;
const int ledPin = 14;
DimmableLight light(ledPin);

// Instantiating a Fuzzy object
Fuzzy *fuzzy = new Fuzzy();

// Antares Config
#define ACCESSKEY "2943a4cc68ecba07:99a731b711abe367"  // Antares Access Key
#define WIFISSID "Pixel_6664"
#define PASSWORD "mdhkrmd008"
#define projectName "mdhkrmd-project"   // Replace with the Antares application name that was created

#define deviceInkubator "inkubator"     // Replace with the Antares device name that was created
#define deviceLampu "controlLampu"
#define deviceKipas "controlKipas"
#define deviceFuzzy "controlFuzzy"
AntaresESPHTTP antares(ACCESSKEY);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Pixel_6664";
const char* password = "mdhkrmd008";

// DHT
#define DHTPIN 19
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

// Fan
int ENA = 12;
int IN1 = 5;
int IN2 = 4;

int ENB = 25;
int IN3 = 27;
int IN4 = 26;

const int frequency = 5000;
const int pwm_channel = 0;
const int resolution = 8;

String readDHTtemperature() {
  float suhu = dht.readTemperature();
  if (isnan(suhu)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return "Failed to read";
  } else {
    return String(suhu);
  }
}

String readDHThumidity() {
  float lembab = dht.readHumidity();
  if (isnan(lembab)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return "Failed to read";
  } else {
    return String(lembab);
  }
}

void kontrolLampu(){
    antares.get(projectName, deviceLampu);        
    if (antares.getSuccess()) {
        int lampuValue = antares.getInt("lampu");
        light.setBrightness(lampuValue);
    }
}

void kontrolKipas(){        
    antares.get(projectName, deviceKipas);
    if (antares.getSuccess()) {
        int kipasValue = antares.getInt("kipas");
        ledcWrite(pwm_channel, kipasValue);
    }
}

void setup()
{
  // Set the Serial output
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);

  while (!Serial)
    ;
  Serial.println();
  Serial.println("Dimmable Light for Arduino: first example");

  Serial.print("Initializing DimmableLight library... ");
  DimmableLight::setSyncPin(syncPin);
  // VERY IMPORTANT: Call this method to activate the library
  DimmableLight::begin();
  Serial.println("Done!");

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT); 
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT); 
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  ledcSetup(pwm_channel, frequency, resolution);
  ledcAttachPin(ENA, pwm_channel);
  ledcAttachPin(ENB, pwm_channel);
  
  // Suhu input
  FuzzyInput *suhu = new FuzzyInput(1);
  FuzzySet *suhurendah = new FuzzySet(28, 28, 34, 37.5);
  suhu->addFuzzySet(suhurendah);
  FuzzySet *suhuoptimal = new FuzzySet(37, 38, 38, 38.5);
  suhu->addFuzzySet(suhuoptimal);
  FuzzySet *suhupanas= new FuzzySet(38, 42, 47, 47);
  suhu->addFuzzySet(suhupanas);
  fuzzy->addFuzzyInput(suhu);

  // Kelembapan input
  FuzzyInput *kelembapan = new FuzzyInput(2);
  FuzzySet *kelembapankering = new FuzzySet(0, 0, 16, 33);
  kelembapan->addFuzzySet(kelembapankering);
  FuzzySet *kelembapanoptimal = new FuzzySet(16, 33, 49, 66);
  kelembapan->addFuzzySet(kelembapanoptimal);
  FuzzySet *kelembapanbasah = new FuzzySet(49, 66, 100, 100);
  kelembapan->addFuzzySet(kelembapanbasah);
  fuzzy->addFuzzyInput(kelembapan);

  ///////////////////////////////////////////////////

  // Lampu Output
  FuzzyOutput *lampu = new FuzzyOutput(1);
  FuzzySet *lampuredup = new FuzzySet(0, 0, 60, 100);
  lampu->addFuzzySet(lampuredup);
  FuzzySet *lampuoptimal = new FuzzySet(80, 100, 120, 150);
  lampu->addFuzzySet(lampuoptimal);
  FuzzySet *lampupanas = new FuzzySet(140, 175, 255, 255);
  lampu->addFuzzySet(lampupanas);
  fuzzy->addFuzzyOutput(lampu);

  // Kipas Output
  FuzzyOutput *kipas = new FuzzyOutput(2);
  FuzzySet *kipaspelan = new FuzzySet(0, 0, 60, 90);
  kipas->addFuzzySet(kipaspelan);
  FuzzySet *kipassedang = new FuzzySet(65, 80, 95, 130);
  kipas->addFuzzySet(kipassedang);
  FuzzySet *kipaskenceng= new FuzzySet(125, 145, 255, 255);
  kipas->addFuzzySet(kipaskenceng);
  fuzzy->addFuzzyOutput(kipas);

  ///////////////////////////// Fuzzy Rules /////////////////////////////

  // suhuRendah_kelembapanKering -> lampuPanas_kipasKenceng
  FuzzyRuleAntecedent *suhuRendah_kelembapanKering = new FuzzyRuleAntecedent();
  suhuRendah_kelembapanKering->joinWithAND(suhurendah, kelembapankering);

  FuzzyRuleConsequent *lampuPanas_kipasPelan2 = new FuzzyRuleConsequent();
  lampuPanas_kipasPelan2->addOutput(lampupanas);
  lampuPanas_kipasPelan2->addOutput(kipaspelan);

  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, suhuRendah_kelembapanKering, lampuPanas_kipasPelan2);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // suhuRendah_kelembapanOptimal -> lampuPanas_kipasSedang
  FuzzyRuleAntecedent *suhuRendah_kelembapanOptimal = new FuzzyRuleAntecedent();
  suhuRendah_kelembapanOptimal->joinWithAND(suhurendah, kelembapanoptimal);

  FuzzyRuleConsequent *lampuPanas_kipasSedang = new FuzzyRuleConsequent();
  lampuPanas_kipasSedang->addOutput(lampupanas);
  lampuPanas_kipasSedang->addOutput(kipassedang);

  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, suhuRendah_kelembapanOptimal, lampuPanas_kipasSedang);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // suhuRendah_kelembapanBasah -> lampuPanas_kipasPelan
  FuzzyRuleAntecedent *suhuRendah_kelembapanBasah = new FuzzyRuleAntecedent();
  suhuRendah_kelembapanBasah->joinWithAND(suhurendah, kelembapanbasah);

  FuzzyRuleConsequent *lampuPanas_kipasPelan = new FuzzyRuleConsequent();
  lampuPanas_kipasPelan->addOutput(lampupanas);
  lampuPanas_kipasPelan->addOutput(kipaspelan);

  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, suhuRendah_kelembapanBasah, lampuPanas_kipasPelan);
  fuzzy->addFuzzyRule(fuzzyRule3);

  // suhuOptimal_kelembapanKering -> lampuOptimal_kipasKenceng
  FuzzyRuleAntecedent *suhuOptimal_kelembapanKering = new FuzzyRuleAntecedent();
  suhuOptimal_kelembapanKering->joinWithAND(suhuoptimal, kelembapankering);

  FuzzyRuleConsequent *lampuOptimal_kipasSedang2 = new FuzzyRuleConsequent();
  lampuOptimal_kipasSedang2->addOutput(lampuoptimal);
  lampuOptimal_kipasSedang2->addOutput(kipassedang);

  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, suhuOptimal_kelembapanKering, lampuOptimal_kipasSedang2);
  fuzzy->addFuzzyRule(fuzzyRule4);

  // suhuOptimal_kelembapanOptimal -> lampuOptimal_kipasSedang
  FuzzyRuleAntecedent *suhuOptimal_kelembapanOptimal = new FuzzyRuleAntecedent();
  suhuOptimal_kelembapanOptimal->joinWithAND(suhuoptimal, kelembapanoptimal);

  FuzzyRuleConsequent *lampuOptimal_kipasSedang = new FuzzyRuleConsequent();
  lampuOptimal_kipasSedang->addOutput(lampuoptimal);
  lampuOptimal_kipasSedang->addOutput(kipassedang);

  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, suhuOptimal_kelembapanOptimal, lampuOptimal_kipasSedang);
  fuzzy->addFuzzyRule(fuzzyRule5);

  // suhuOptimal_kelembapanBasah -> lampuOptimal_kipasPelan
  FuzzyRuleAntecedent *suhuOptimal_kelembapanBasah = new FuzzyRuleAntecedent();
  suhuOptimal_kelembapanBasah->joinWithAND(suhuoptimal, kelembapanbasah);

  FuzzyRuleConsequent *lampuOptimal_kipasPelan = new FuzzyRuleConsequent();
  lampuOptimal_kipasPelan->addOutput(lampuoptimal);
  lampuOptimal_kipasPelan->addOutput(kipaspelan);

  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, suhuOptimal_kelembapanBasah, lampuOptimal_kipasPelan);
  fuzzy->addFuzzyRule(fuzzyRule6);

  // suhuPanas_kelembapanKering -> lampuRedup_kipasKenceng
  FuzzyRuleAntecedent *suhuPanas_kelembapanKering = new FuzzyRuleAntecedent();
  suhuPanas_kelembapanKering->joinWithAND(suhupanas, kelembapankering);

  FuzzyRuleConsequent *lampuRedup_kipasKenceng = new FuzzyRuleConsequent();
  lampuRedup_kipasKenceng->addOutput(lampuredup);
  lampuRedup_kipasKenceng->addOutput(kipaskenceng);

  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, suhuPanas_kelembapanKering, lampuRedup_kipasKenceng);
  fuzzy->addFuzzyRule(fuzzyRule7);

  // suhuPanas_kelembapanOptimal -> lampuRedup_kipasSedang
  FuzzyRuleAntecedent *suhuPanas_kelembapanOptimal = new FuzzyRuleAntecedent();
  suhuPanas_kelembapanOptimal->joinWithAND(suhupanas, kelembapanoptimal);

  FuzzyRuleConsequent *lampuRedup_kipasSedang = new FuzzyRuleConsequent();
  lampuRedup_kipasSedang->addOutput(lampuredup);
  lampuRedup_kipasSedang->addOutput(kipassedang);

  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, suhuPanas_kelembapanOptimal, lampuRedup_kipasSedang);
  fuzzy->addFuzzyRule(fuzzyRule8);

  // suhuPanas_kelembapanBasah -> lampuRedup_kipasPelan
  FuzzyRuleAntecedent *suhuPanas_kelembapanBasah = new FuzzyRuleAntecedent();
  suhuPanas_kelembapanBasah->joinWithAND(suhupanas, kelembapanbasah);

  FuzzyRuleConsequent *lampuRedup_kipasPelan = new FuzzyRuleConsequent();
  lampuRedup_kipasPelan->addOutput(lampuredup);
  lampuRedup_kipasPelan->addOutput(kipaspelan);

  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, suhuPanas_kelembapanBasah, lampuRedup_kipasPelan);
  fuzzy->addFuzzyRule(fuzzyRule9);

  antares.setDebug(true);  // Enable Antares library debug mode
  antares.wifiConnection(WIFISSID, PASSWORD);  // Connect to   WiFi using provided SSID and password
  
  antares.get(projectName, deviceInkubator);
  antares.get(projectName, deviceLampu);
  antares.get(projectName, deviceKipas);
  antares.get(projectName, deviceFuzzy);
  
  // pinMode(BUTTON_UP, INPUT_PULLUP);
  // pinMode(BUTTON_LOW, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Temperature and PID
  dht.begin();
}

void loop()
{
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");
  lcd.print(suhu);
  
  lcd.setCursor(0,1);
  lcd.print("Kelembapan: ");
  lcd.print(kelembapan);
  
  float lampu;
  float kipas;
  
  // float suhu = random(32,47);
  // float kelembapan = random(25,50);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  antares.get(projectName, deviceInkubator);
  antares.get(projectName, deviceFuzzy);
  
  if (antares.getSuccess()) {
        String fuzzyValue = antares.getString("fuzzySet");

        if (fuzzyValue == "on"){
          fuzzy->setInput(1, suhu);
          fuzzy->setInput(2, kelembapan);
          
          // Running the Fuzzification
          fuzzy->fuzzify();
          
          // Running the Defuzzification
          lampu = fuzzy->defuzzify(1);
          kipas = fuzzy->defuzzify(2);

          light.setBrightness(lampu);
          ledcWrite(pwm_channel, kipas);
        }

        else if (fuzzyValue == "off")
        {
          kontrolLampu();
          kontrolKipas();
        }     
  }

  Serial.print("Suhu: ");
  Serial.println(suhu);
  Serial.print("Kelembapan: ");
  Serial.println(kelembapan);
     
  
  Serial.print("lampu: ");
  Serial.println(lampu);
  
  Serial.print("kipas: ");
  Serial.println(kipas);

  Serial.println("");

  antares.add("temperature", suhu);
  antares.add("humidity", kelembapan);
  antares.add("lampu", lampu);
  antares.add("kipas", kipas);

  antares.send(projectName, deviceInkubator);
  
  lcd.clear();
  delay(1000);
}
