/*
  JSON Object

  This sketch demonstrates how to use various features
  of the Official Arduino_JSON library, in particular for JSON objects.

  This example code is in the public domain.
*/

#include <Arduino_JSON.h>
#include <assert.h>

const char input[] = "{\"result\":true,\"count\":42,\"foo\":\"bar\"}";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  demoParse();

  demoCreation();
}

void loop() {
}

void demoParse() {
  Serial.println("parse");
  Serial.println("=====");

  JSONVar myObject = JSON.parse(input);

  // JSON.typeof(jsonVar) can be used to get the type of the variable
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }

  Serial.print("JSON.typeof(myObject) = ");
  Serial.println(JSON.typeof(myObject)); // prints: object

  // myObject.hasOwnProperty(key) checks if the object contains an entry for key
  if (myObject.hasOwnProperty("result")) {
    Serial.print("myObject[\"result\"] = ");

    Serial.println((bool) myObject["result"]);
  }

  if (myObject.hasOwnProperty("count")) {
    Serial.print("myObject[\"count\"] = ");

    Serial.println((int) myObject["count"]);
  }

  if (myObject.hasOwnProperty("count")) {
    Serial.print("myObject[\"count\"] = ");

    Serial.println((double) myObject["count"]);
  }

  if (myObject.hasOwnProperty("foo")) {
    Serial.print("myObject[\"foo\"] = ");

    Serial.println((const char*) myObject["foo"]);
  }

  // JSONVars can be printed using print or println
  Serial.print("myObject = ");
  Serial.println(myObject);

  Serial.println();
}

void demoCreation() {
  Serial.println("creation");
  Serial.println("========");

  JSONVar myObject;

  myObject["hello"] = "world";
  myObject["true"] = true;

  myObject["x1"] = (int) 42;
  myObject["x2"] = (long) 42;
  myObject["x3"] = (unsigned long) 42;

  int x1 = myObject["x1"];
  assert(x1 == 42);
  
  long x2 = myObject["x2"];
  assert(x2 == 42);

  unsigned long x3 = myObject["x3"];
  assert(x3 == 42);

  Serial.print("myObject.keys() = ");
  Serial.println(myObject.keys());

  // JSON.stringify(myVar) can be used to convert the JSONVar to a String
  String jsonString = JSON.stringify(myObject);

  Serial.print("JSON.stringify(myObject) = ");
  Serial.println(jsonString);

  Serial.println();

  // myObject.keys() can be used to get an array of all the keys in the object
  JSONVar keys = myObject.keys();

  for (int i = 0; i < keys.length(); i++) {
    JSONVar value = myObject[keys[i]];

    Serial.print("JSON.typeof(myObject[");
    Serial.print(keys[i]);
    Serial.print("]) = ");
    Serial.println(JSON.typeof(value));

    Serial.print("myObject[");
    Serial.print(keys[i]);
    Serial.print("] = ");
    Serial.println(value);

    Serial.println();
  }

  Serial.println();

  // setting a value to undefined can remove it from the object
  myObject["x"] = undefined;

  // you can also change a value
  myObject["hello"] = "there!";

  Serial.print("myObject = ");
  Serial.println(myObject);
}
