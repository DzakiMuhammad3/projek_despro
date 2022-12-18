#define DOOR_SENSOR 14
String masterCard[3] ={"04382652F85180", "77E88D2A", "237497AC"};
int sensor;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DOOR_SENSOR, INPUT_PULLUP);

}

void loop() {
  // put your main code here, to run repeatedly:
//  masterCard = {"04382652F85180", "77E88D2A", "237497AC"};
  for(int i=0; i<3; i++){
    Serial.println(masterCard[i]);
  }
  sensor =digitalRead(DOOR_SENSOR);
  Serial.println(String(sensor));
  delay(100);

}
