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

#include "cjson/cJSON.h"

#include "JSONVar.h"

JSONVar::JSONVar(struct cJSON* json, struct cJSON* parent) :
  _json(json),
  _parent(parent)
{
}

JSONVar::JSONVar(bool b) :
  JSONVar()
{
  *this = b;
}

JSONVar::JSONVar (char i) :
    JSONVar ()
{
    *this = i;
}

JSONVar::JSONVar (unsigned char i) :
    JSONVar ()
{
    *this = i;
}

JSONVar::JSONVar (short i) :
    JSONVar ()
{
    *this = i;
}

JSONVar::JSONVar (unsigned short i) :
    JSONVar ()
{
    *this = i;
}

JSONVar::JSONVar(int i) :
  JSONVar()
{
  *this = i;
}

JSONVar::JSONVar (unsigned int i) :
    JSONVar ()
{
    *this = i;
}

JSONVar::JSONVar(long l) :
  JSONVar()
{
  *this = l;
}

JSONVar::JSONVar(unsigned long ul) :
  JSONVar()
{
  *this = ul;
}

JSONVar::JSONVar(double d) :
  JSONVar()
{
  *this = d;
}

JSONVar::JSONVar(const char* s)  :
  JSONVar()
{
  *this = s;
}

JSONVar::JSONVar(const String& s)  :
  JSONVar()
{
  *this = s;
}

JSONVar::JSONVar(const JSONVar& v)
{
  _json = cJSON_Duplicate(v._json, true);
  _parent = NULL;
}

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
JSONVar::JSONVar(JSONVar&& v)
{
  cJSON* tmp;

  // swap _json
  tmp = _json;
  _json = v._json;
  v._json = tmp;

  // swap parent
  tmp = _parent;
  _parent = v._parent;
  v._parent = tmp;
}
#endif

JSONVar::JSONVar(nullptr_t)  :
  JSONVar()
{
  *this = nullptr;
}

JSONVar::JSONVar() :
  JSONVar(NULL, NULL)
{
}

JSONVar::~JSONVar()
{
  if (_json != NULL && _parent == NULL) {
    cJSON_Delete(_json);

    _json = NULL;
  }
}

size_t JSONVar::printTo(Print& p) const
{
  if (_json == NULL) {
    return 0;
  }

  char* s = cJSON_PrintUnformatted(_json);

  size_t writen = p.print(s);

  cJSON_free(s);

  return writen;
}

JSONVar::operator bool() const
{
  return cJSON_IsBool(_json) && cJSON_IsTrue(_json);
}

JSONVar::operator char () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator unsigned char () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator short () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator unsigned short () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator int () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator unsigned int () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator long () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator unsigned long () const
{
    return cJSON_IsNumber (_json) ? _json->valueint : 0;
}

JSONVar::operator double() const
{
  return cJSON_IsNumber(_json) ? _json->valuedouble : NAN;
}

JSONVar::operator const char*() const
{
  if (cJSON_IsString(_json)) {
    return _json->valuestring;
  }

  return NULL;
}

JSONVar::operator const String () const
{
    if (cJSON_IsString (_json)) {
        return String(_json->valuestring);
    }

    return String();
}

void JSONVar::operator=(const JSONVar& v)
{
  if (&v == &undefined) {
    if (cJSON_IsObject(_parent)) {
      cJSON_DeleteItemFromObjectCaseSensitive(_parent, _json->string);

      _json = NULL;
      _parent = NULL;
    } else {
      replaceJson(cJSON_CreateNull());
    }
  } else {
    replaceJson(cJSON_Duplicate(v._json, true));
  }
}

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
JSONVar& JSONVar::operator=(JSONVar&& v)
{
  cJSON* tmp;

  // swap _json
  tmp = _json;
  _json = v._json;
  v._json = tmp;

  // swap parent
  tmp = _parent;
  _parent = v._parent;
  v._parent = tmp;

  return *this;
}
#endif

void JSONVar::operator=(bool b)
{
  replaceJson(b ? cJSON_CreateTrue() : cJSON_CreateFalse());
}

void JSONVar::operator=(char i)
{
    replaceJson (cJSON_CreateNumber (i));
}

void JSONVar::operator=(unsigned char i)
{
    replaceJson (cJSON_CreateNumber (i));
}

void JSONVar::operator=(short i)
{
    replaceJson (cJSON_CreateNumber (i));
}

void JSONVar::operator=(unsigned short i)
{
    replaceJson (cJSON_CreateNumber (i));
}

void JSONVar::operator=(int i)
{
    replaceJson (cJSON_CreateNumber (i));
}

void JSONVar::operator=(unsigned int i)
{
    replaceJson (cJSON_CreateNumber (i));
}

void JSONVar::operator=(long l)
{
  replaceJson(cJSON_CreateNumber(l));
}

void JSONVar::operator=(unsigned long ul)
{
  replaceJson(cJSON_CreateNumber(ul));
}

void JSONVar::operator=(double d)
{
  replaceJson(cJSON_CreateNumber(d));
}

void JSONVar::operator=(const char* s)
{
  replaceJson(cJSON_CreateString(s));
}

void JSONVar::operator=(const String& s)
{
  *this = s.c_str();
}

void JSONVar::operator=(nullptr_t)
{
  replaceJson(cJSON_CreateNull());
}

bool JSONVar::operator==(const JSONVar& v) const
{
  return cJSON_Compare(_json, v._json, 1) ||
          (_json == NULL && v._json == NULL);
}

bool JSONVar::operator==(nullptr_t) const
{
  return (cJSON_IsNull(_json));
}

JSONVar JSONVar::operator[](const char* key)
{
  if (!cJSON_IsObject(_json)) {
    replaceJson(cJSON_CreateObject());
  }

  cJSON* json = cJSON_GetObjectItemCaseSensitive(_json, key);

  if (json == NULL) {
    json = cJSON_AddNullToObject(_json, key);
  }
  
  return JSONVar(json, _json);    
}

JSONVar JSONVar::operator[](const String& key)
{
  return (*this)[key.c_str()];
}

JSONVar JSONVar::operator[](int index)
{
  if (!cJSON_IsArray(_json)) {
    replaceJson(cJSON_CreateArray());
  }

  cJSON* json = cJSON_GetArrayItem(_json, index);

  if (json == NULL) {
    while (index >= cJSON_GetArraySize(_json)) {
      json = cJSON_CreateNull();

      cJSON_AddItemToArray(_json, json);
    }
  }

  return JSONVar(json, _json);
}

JSONVar JSONVar::operator[](const JSONVar& key)
{
  if (cJSON_IsArray(_json) && cJSON_IsNumber(key._json)) {
    int index = (int)key;

    return (*this)[index];
  }

  if (cJSON_IsObject(_json) && cJSON_IsString(key._json)) {
    const char* str = (const char*) key;

    return (*this)[str];
  }

  return JSONVar(NULL, NULL);
}

int JSONVar::length() const
{
  if (cJSON_IsString(_json)) {
    return strlen(_json->string);
  } else if (cJSON_IsArray(_json)) {
    return cJSON_GetArraySize(_json);
  } else {
    return -1;
  }
}

JSONVar JSONVar::keys() const
{
  if (!cJSON_IsObject(_json)) {
    return JSONVar(NULL, NULL);
  }

  int length = cJSON_GetArraySize(_json);

  const char* keys[length];
  cJSON* child = _json->child;

  for (int i = 0; i < length; i++, child = child->next) {
    keys[i] = child->string;
  }

  return JSONVar(cJSON_CreateStringArray(keys, length), NULL);
}

bool JSONVar::hasOwnProperty(const char* key) const
{
  if (!cJSON_IsObject(_json)) {
    return false;
  }

  cJSON* json = cJSON_GetObjectItemCaseSensitive(_json, key);
  return (json != NULL);
}

bool JSONVar::hasOwnProperty(const String& key) const
{
  return hasOwnProperty(key.c_str());
}

JSONVar JSONVar::parse(const char* s)
{
  cJSON* json = cJSON_Parse(s);

  return JSONVar(json, NULL);
}

JSONVar JSONVar::parse(const String& s)
{
  return parse(s.c_str());
}

String JSONVar::stringify(const JSONVar& value)
{
  if (value._json == NULL) {
    return String((const char *)NULL);
  }

  char* s = cJSON_PrintUnformatted(value._json);

  String str = s;

  cJSON_free(s);

  return str;
}

String JSONVar::typeof_(const JSONVar& value)
{
  struct cJSON* json = value._json;

  if (json == NULL ||  cJSON_IsInvalid(json)) {
    return "undefined";
  } else if (cJSON_IsBool(json)) {
    return "boolean";
  } else if (cJSON_IsNull(json)) {
    return "null"; // TODO: should this return "object" to be more JS like?
  } else if (cJSON_IsNumber(json)) {
    return "number";
  } else if (cJSON_IsString(json)) {
    return "string";
  } else if (cJSON_IsArray(json)) {
    return "array"; // TODO: should this return "object" to be more JS like?
  } else if (cJSON_IsObject(json)) {
    return "object";
  } else {
    return "unknown";
  }
}

void JSONVar::replaceJson(struct cJSON* json)
{
  cJSON* old = _json;

  _json = json;

  if (old) {
    if (_parent) {
      if (cJSON_IsObject(_parent)) {
        cJSON_ReplaceItemInObjectCaseSensitive(_parent, old->string, _json);
      } else if (cJSON_IsArray(_parent)) {
        cJSON_ReplaceItemViaPointer(_parent, old, _json);
      }
    } else {
      cJSON_Delete(old);
    }
  }
}

//---------------------------------------------------------------------

bool JSONVar::hasPropertyEqual(const char* key,  const char* value) const {
  if (!cJSON_IsObject(_json)) {
    return false;
  }

  cJSON* json = cJSON_GetObjectItemCaseSensitive(_json, key);
  return json != NULL && strcmp(value, json->valuestring) == 0;
} 

//---------------------------------------------------------------------

bool JSONVar::hasPropertyEqual(const char* key,  const JSONVar& value) const {
  return this->hasPropertyEqual(key, (const char*)value);
} 

//---------------------------------------------------------------------

bool JSONVar::hasPropertyEqual(const String& key,  const String& value) const {
  return this->hasPropertyEqual(key.c_str(), value.c_str());
}   

//---------------------------------------------------------------------

bool JSONVar::hasPropertyEqual(const String& key,  const JSONVar& value) const  {
  return this->hasPropertyEqual(key.c_str(), (const char*)value);
} 

//---------------------------------------------------------------------

JSONVar JSONVar::filter(const char* key, const char* value) const {
  cJSON* item;
  cJSON* test;
  cJSON* json = cJSON_CreateArray();

  if(cJSON_IsObject(_json)){
    test = cJSON_GetObjectItem(_json, key);
    
    if(test != NULL && strcmp(value, test->valuestring) == 0){
      return (*this);
    }
  }
  
  for (int i = 0; i < cJSON_GetArraySize(_json); i++) {
    item = cJSON_GetArrayItem(_json, i);
  
    if (item == NULL) {
      continue;
    }
    
    test = cJSON_GetObjectItem(item, key);
    
    if(test != NULL && strcmp(value, test->valuestring) == 0){
      cJSON_AddItemToArray(json, cJSON_Duplicate(item,true));
    }
  }

  if(cJSON_GetArraySize(json) == 0){
    return JSONVar(NULL, NULL);
  }
  
  if(cJSON_GetArraySize(json) == 1){
    return JSONVar(cJSON_GetArrayItem(json, 0), _json); 
  }

  return JSONVar(json, _json); 
}

JSONVar JSONVar::filter(const char* key, const JSONVar& value) const {
  return this->filter(key, (const char*)value);
}

JSONVar JSONVar::filter(const String& key, const String& value) const {
  return this->filter(key.c_str(), value.c_str());
}

JSONVar JSONVar::filter(const String& key, const JSONVar& value) const {
  return this->filter(key.c_str(), (const char*)value);
}

JSONVar undefined;
