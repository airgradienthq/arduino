/*
  This file is part of the Arduino_JSON library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _JSON_VAR_H_
#define _JSON_VAR_H_

#include <Arduino.h>

struct cJSON;

#define typeof typeof_
#define null nullptr

class JSONVar : public Printable {
public:
  JSONVar();
  JSONVar(bool b);
  JSONVar(char i);
  JSONVar(unsigned char i);
  JSONVar(short i);
  JSONVar(unsigned short i);
  JSONVar(int i);
  JSONVar(unsigned int i);
  JSONVar(long l);
  JSONVar(unsigned long ul);
  JSONVar(double d);
  JSONVar(const char* s);
  JSONVar(const String& s);
  JSONVar(const JSONVar& v);
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  JSONVar(JSONVar&& v);
#endif
  JSONVar(nullptr_t);
  virtual ~JSONVar();

  virtual size_t printTo(Print& p) const;

  operator bool() const;
  operator char() const;
  operator unsigned char() const;
  operator short() const;
  operator unsigned short() const;
  operator int() const;
  operator unsigned int() const;
  operator long () const;
  operator unsigned long () const;
  operator double() const;
  operator const char* () const;
  operator const String () const;

  void operator=(const JSONVar& v);
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  JSONVar& operator=(JSONVar&& v);
#endif
  void operator=(bool b);
  void operator=(char i);
  void operator=(unsigned char i);
  void operator=(short i);
  void operator=(unsigned short i);
  void operator=(int i);
  void operator=(unsigned int i);
  void operator=(long l);
  void operator=(unsigned long ul);
  void operator=(double d);
  void operator=(const char* s);
  void operator=(const String& s);
  void operator=(nullptr_t);

  bool operator==(const JSONVar& v) const;
  bool operator==(nullptr_t) const;

  JSONVar operator[](const char* key);
  JSONVar operator[](const String& key);
  JSONVar operator[](int index);
  JSONVar operator[](const JSONVar& key);  

  int length() const;
  JSONVar keys() const;
  bool hasOwnProperty(const char* key) const;
  bool hasOwnProperty(const String& key) const;
  
  bool hasPropertyEqual(const char* key,  const char* value) const;  
  bool hasPropertyEqual(const char* key,  const JSONVar& value) const;  
  bool hasPropertyEqual(const String& key,  const String& value) const;  
  bool hasPropertyEqual(const String& key,  const JSONVar& value) const;  
  
  JSONVar filter(const char* key, const char* value) const;
  JSONVar filter(const char* key, const JSONVar& value) const;
  JSONVar filter(const String& key, const String& value) const;
  JSONVar filter(const String& key, const JSONVar& value) const;

  static JSONVar parse(const char* s);
  static JSONVar parse(const String& s);
  static String stringify(const JSONVar& value);
  static String typeof_(const JSONVar& value);

private:
  JSONVar(struct cJSON* json, struct cJSON* parent);

  void replaceJson(struct cJSON* json);

private:
  struct cJSON* _json;
  struct cJSON* _parent;
};

extern JSONVar undefined;

#endif
