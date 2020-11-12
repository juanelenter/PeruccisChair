// Libraries
#include <math.h>
#include <CircularBuffer.h>

// I/O
#define joyX A0
#define joyY A1
#define dir_pin_i 46
#define dir_pin_d 44
#define pwm_pin_i 8
#define pwm_pin_d 6

// Macros
#define window_size 20
#define pwm_max 120
const int responsiveness = 3; //Updating voltages time period in ms.
const float setup_time = 0.5;

typedef struct{
  float x;
  float y;
  }coordinates;

//Global variables
unsigned long time = 0; 
coordinates offset;
CircularBuffer<int,window_size> Vi;
CircularBuffer<int,window_size> Vd;

// UTILS
coordinates get_offset() {
  offset.x = 0;
  offset.y = 0;
  int n = 100;
  for ( int i = 0; i < n; i++) {
    offset.x += analogRead(joyX);
    offset.y += analogRead(joyY);
    delay(setup_time);
    //Serial.println(offset.x);
    }
  offset.x = offset.x / n;
  offset.y = offset.y / n;
  Serial.println("Offset Values:");
  Serial.println(offset.x);
  Serial.println(offset.y);
  return offset;
}

coordinates get_xy() {
  coordinates point;
  point.x = analogRead(joyX) - offset.x;
  point.y = analogRead(joyY) - offset.y;
  return point;
}

coordinates xy_to_volts( coordinates point ) {

  //Compute phase, magnitude and scaling factor A
 
  //Volts will store Voltages (Vi, Vd) associated to point (x,y)
  coordinates volts;
  volts.x = (point.y - point.x)*pwm_max/1024;
  volts.y = (point.y + point.x)*pwm_max/1024;
  return volts;
}

void act_volts( ) { // This could be more efficient only substracting last and adding first.
  double avg_i = 0;
  double avg_d = 0;
  using index_t = decltype(Vi)::index_t; // ths line ensures using the right type for the index variable
  for (index_t i = 0; i < Vi.size(); i++) {
    avg_i += Vi[i] / (float)Vi.size();
    avg_d += Vd[i] / (float)Vd.size();
  }
  
  if (avg_i >= 0) {
    digitalWrite(dir_pin_i, HIGH);
    }
  else {
    digitalWrite(dir_pin_i, LOW);
  }
  analogWrite((int)pwm_pin_i, abs(avg_i));
  
  if (avg_d >= 0) {
    digitalWrite(dir_pin_d, LOW);
    }
  else {
    digitalWrite(dir_pin_d, HIGH);
  }
  analogWrite((int)pwm_pin_d, abs(avg_d));
 
  // Serial.println("Voltages updated");
  Serial.print("  VOltages i, d:   ");
  Serial.print(avg_i);
  Serial.print("  ,  ");
  Serial.println(avg_d);
}

void kill_volts( ) { // This could be more efficient only substracting last and adding first.

  analogWrite( (int)pwm_pin_i, 0);
  
  analogWrite( (int)pwm_pin_d, 0);
 
  // Serial.println("Voltages updated");
  Serial.print("  VOltages i, d:   ");
  Serial.print(0);
  Serial.print("  ,  ");
  Serial.println(0);
}

void setup() {
 
  pinMode(dir_pin_i, OUTPUT);
  pinMode(dir_pin_d, OUTPUT);
  pinMode(pwm_pin_i, OUTPUT);
  pinMode(pwm_pin_d, OUTPUT);
  delay(3000);
  while (!Vi.isFull()){ // Initialize Voltage Buffers with zeros.
    Vi.unshift(0);
    Vd.unshift(0);
    }
  Serial.begin(9600);
  offset = get_offset();
  time = millis();
  Serial.println("Setup complete.");
}

void loop() {
  
  coordinates point, voltages; 
  point = get_xy(); // Read input x,y
  voltages = xy_to_volts( point ); // Convert x,y to desired voltages in motors i (left) and d (right)

  Vi.unshift(voltages.x); // Push voltages into circular buffers
  Vd.unshift(voltages.y);
   
  if (millis() - time >= responsiveness ) {
    time = millis();
    Serial.print("point x, y: ");
    Serial.print(point.x);
    Serial.print("  , ");
    Serial.print(point.y);
    if ( sqrt( pow(point.x,2) + pow(point.y,2) ) < 20){
      kill_volts();
    }
    else{
    act_volts();
  }
  }
}
