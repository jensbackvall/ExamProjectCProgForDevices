#include "Ultralyd.h"
#include <arduino.h>
#include <stdio.h>


// Function for getting the ultrasound distance
int distance(int trigPin,int echoPin) {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // defines variables
  long duration;
  int distanceR;
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distanceR= duration*0.034/2;
    return distanceR;
}
