/*
  JSON Kitchen Sink

  This sketch demonstrates how to use various features
  of the Official Arduino_JSON library.

  This example code is in the public domain.
*/

#include <Arduino_JSON.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // boolean
  booleanDemo();

  intDemo();

  doubleDemo();

  stringDemo();

  arrayDemo();

  objectDemo();
}

void loop() {
}

void booleanDemo() {
  Serial.println("boolean");
  Serial.println("=======");

  JSONVar myBoolean = true;

  Serial.print("JSON.typeof(myBoolean) = ");
  Serial.println(JSON.typeof(myBoolean)); // prints: boolean

  Serial.print("myBoolean = ");
  Serial.println(myBoolean); // prints: true

  myBoolean = false;

  Serial.print("myBoolean = ");
  Serial.println((boolean) myBoolean); // prints: 0

  Serial.println();
}

void intDemo() {
  Serial.println("int");
  Serial.println("===");

  JSONVar myInt = 42;

  Serial.print("JSON.typeof(myInt) = ");
  Serial.println(JSON.typeof(myInt)); // prints: number

  Serial.print("myInt = ");
  Serial.println(myInt); // prints: 42

  myInt = 4242;

  Serial.print("myInt = ");
  Serial.println((int) myInt); // prints: 4242

  Serial.println();
}

void doubleDemo() {
  Serial.println("double");
  Serial.println("======");

  JSONVar myDouble = 42.5;

  Serial.print("JSON.typeof(myDouble) = ");
  Serial.println(JSON.typeof(myDouble)); // prints: number

  Serial.print("myDouble = ");
  Serial.println(myDouble); // prints: 42.5

  myDouble = 4242.4242;

  Serial.print("myDouble = ");
  Serial.println((double) myDouble, 4); // prints: 4242.4242

  Serial.println();
}

void stringDemo() {
  Serial.println("string");
  Serial.println("======");

  JSONVar myString = "Hello World!";

  Serial.print("JSON.typeof(myString) = ");
  Serial.println(JSON.typeof(myString)); // prints: string

  Serial.print("myString = ");
  Serial.println(myString); // prints: Hello World!

  myString = ":)";

  Serial.print("myString = ");
  Serial.println((const char*) myString); // prints: :)

  Serial.println();
}

void arrayDemo() {
  Serial.println("array");
  Serial.println("=====");

  JSONVar myArray;

  myArray[0] = 42;

  Serial.print("JSON.typeof(myArray) = ");
  Serial.println(JSON.typeof(myArray)); // prints: array

  Serial.print("myArray = ");
  Serial.println(myArray); // prints: [42]

  Serial.print("myArray[0] = ");
  Serial.println((int)myArray[0]); // prints: 42

  myArray[1] = 42.5;

  Serial.print("myArray = ");
  Serial.println(myArray); // prints: [42,42.5]

  Serial.print("myArray[1] = ");
  Serial.println((double)myArray[1]); // prints: 42.50

  Serial.println();
}

void objectDemo() {
  Serial.println("object");
  Serial.println("======");

  JSONVar myObject;

  myObject["foo"] = "bar";

  Serial.print("JSON.typeof(myObject) = ");
  Serial.println(JSON.typeof(myObject)); // prints: object

  Serial.print("myObject.keys() = ");
  Serial.println(myObject.keys()); // prints: ["foo"]

  Serial.print("myObject = ");
  Serial.println(myObject); // prints: {"foo":"bar"}

  myObject["blah"]["abc"] = 42;

  Serial.print("myObject.keys() = ");
  Serial.println(myObject.keys()); // prints: ["foo","blah"]

  Serial.print("myObject = ");
  Serial.println(myObject); // prints: {"foo":"bar","blah":{"abc":42}}
}
