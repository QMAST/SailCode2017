
String transmissionBuffer = "";         // a string to hold incoming data
const int ledPin = 13;

void setup() {
  // initialize serial:
  Serial.begin(9600);
  transmissionBuffer.reserve(200); // reserve 200 bytes for the inputString:
  pinMode(ledPin, OUTPUT);
  Serial.print("10,0,Device Initialized - Ready for Commands;");
}

void loop() {
  // do something here eventually

}

/*
SerialEvent occurs whenever a new data comes in the
hardware serial RX.  This routine is run between each
time loop() runs, so using delay inside loop can delay
response.  Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    transmissionBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {
      int iCommand = transmissionBuffer.substring(0, 2).toInt();
      String data = transmissionBuffer.substring(3, transmissionBuffer.indexOf(";"));
      switch (iCommand) {
        case 20:
        Serial.print("20,200,added;");
        break;
        case 21:
        if(data == "0"){
          digitalWrite(ledPin, LOW);
          Serial.print("21,200,0;");
        }else if(data == "1"){
          digitalWrite(ledPin, HIGH);
          Serial.print("21,200,1;");
        }else{
          Serial.print("21,501,Unknown Command:'"+data+"';");
        }
        break;
        default:
        Serial.print("501;");
        break;
      }
      // clear the string:
      transmissionBuffer = "";
    }
  }
}
