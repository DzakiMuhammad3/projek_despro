#define RELAY 13

int a;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(RELAY, INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  a = digitalRead(RELAY);
  Serial.println(a);
  delay(2);
}
