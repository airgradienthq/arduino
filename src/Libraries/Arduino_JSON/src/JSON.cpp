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

#include "JSON.h"

// #define JSON_HOOKS

#ifdef JSON_HOOKS

#include "cjson/cJSON.h"

static int jsonAllocations = 0;

void* JSON_malloc(size_t sz)
{
  void* ptr = malloc(sz);

  jsonAllocations++;

  Serial.print("JSON malloc: 0x");
  Serial.print((unsigned int)ptr);
  Serial.print(' ');
  Serial.print(sz);
  Serial.print(' ');
  Serial.println(jsonAllocations);

  return ptr;
}

void JSON_free(void* ptr)
{
  jsonAllocations--;

  Serial.print("JSON free: 0x");
  Serial.print((unsigned int)ptr);
  Serial.print(' ');
  Serial.println(jsonAllocations);

  free(ptr);
}
#endif

JSONClass::JSONClass()
{
#ifdef JSON_HOOKS
  struct cJSON_Hooks hooks = {
    JSON_malloc,
    JSON_free
  };

  cJSON_InitHooks(&hooks);
#endif
}

JSONClass::~JSONClass()
{
}

JSONVar JSONClass::parse(const char* s)
{
  return JSONVar::parse(s);
}

JSONVar JSONClass::parse(const String& s)
{
  return JSONVar::parse(s);
}

String JSONClass::stringify(const JSONVar& value)
{
  return JSONVar::stringify(value);
}

String JSONClass::typeof(const JSONVar& value)
{
  return JSONVar::typeof(value);
}

JSONClass JSON;
