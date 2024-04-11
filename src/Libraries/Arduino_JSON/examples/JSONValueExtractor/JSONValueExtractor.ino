/*
  JSON Value Extractor

  This sketch demonstrates how to use some features
  of the Official Arduino JSON library to traverse through all the
  key value pair in the object and the nested objects.
  Can be very helpful when searching for a specific data in a key
  which is nested at multiple levels
  The sketch actually use recursion to traverse all the keys in
  a given JSON.

  Example originally added on 24-03-2020
  by Madhur Dixit https://github.com/Chester-King

  This example code is in the public domain.
*/

#include <Arduino_JSON.h>

void setup() {

  Serial.begin(9600);
  while (!Serial);
  valueExtractor();

}

void loop() {
}

void valueExtractor() {

  Serial.println("object");
  Serial.println("======");
  JSONVar myObject;

  // Making a JSON Object
  myObject["foo"] = "bar";
  myObject["blah"]["abc"] = 42;
  myObject["blah"]["efg"] = "pod";
  myObject["blah"]["cde"]["pan1"] = "King";
  myObject["blah"]["cde"]["pan2"] = 3.14;
  myObject["jok"]["hij"] = "bar";

  Serial.println(myObject);
  Serial.println();
  Serial.println("Extracted Values");
  Serial.println("======");

  objRec(myObject);

}

void objRec(JSONVar myObject) {
  Serial.println("{");
  for (int x = 0; x < myObject.keys().length(); x++) {
    if ((JSON.typeof(myObject[myObject.keys()[x]])).equals("object")) {
      Serial.print(myObject.keys()[x]);
      Serial.println(" : ");
      objRec(myObject[myObject.keys()[x]]);
    }
    else {
      Serial.print(myObject.keys()[x]);
      Serial.print(" : ");
      Serial.println(myObject[myObject.keys()[x]]);
    }
  }
  Serial.println("}");
}
