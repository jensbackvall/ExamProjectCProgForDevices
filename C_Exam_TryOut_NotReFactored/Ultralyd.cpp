#include "Ultralyd.h"
#include <arduino.h>
#include <stdio.h>



// Function for getting the ultrasound distance
int distance(int trigPin,int echoPin) {
  delay(10);
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
  // Calculating the distance, example:
  // speed of sound is approx. 340 m/s, or 340000 cm per 1000000 microseconds.
  // The time is for the soundwave to hit a passing train and come back, i.e. twice the 
  // actual distance.So if the time is 1000 microseconds (or 0.001 seconds), the distance is:
  // (1000 * 0.034) / 2 = 34/2 = 17 cm. 
  distanceR= duration*0.034/2;
    return distanceR;
}
