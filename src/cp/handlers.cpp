#include "handlers.h"
#include "cloud_profiler.h"

#include <string>
#include <map>
#include <iostream>

std::map<std::string, handler_type> String2HandlerMap = {
		{"IDENTITY",             IDENTITY},
		{"MEDIAN",               MEDIAN},
		{"DOWNSAMPLE",           DOWNSAMPLE},
		{"XOY",                  XOY},
		{"FIRSTLAST",            FIRSTLAST},
		{"PERIODICCOUNTER",      PERIODICCOUNTER},
		{"FASTPERIODICCOUNTER",  FASTPERIODICCOUNTER},
		{"NULLH",                NULLH},
		{"NET_CONF",             NET_CONF},
		{"FOX_START",            FOX_START},
		{"FOX_END",              FOX_END},
		{"GPIORTT",              GPIORTT},
		{"BUFFER_IDENTITY",      BUFFER_IDENTITY},
    {"BUFFER_IDENTITY_COMP", BUFFER_IDENTITY_COMP},
};

std::map<handler_type, std::string> Handler2StringMap = {
		{IDENTITY,             "IDENTITY"},
		{MEDIAN,               "MEDIAN"},
		{DOWNSAMPLE,           "DOWNSAMPLE"},
		{XOY,                  "XOY"},
		{FIRSTLAST,            "FIRSTLAST"},
		{PERIODICCOUNTER,      "PERIODICCOUNTER"},
		{FASTPERIODICCOUNTER,  "FASTPERIODICCOUNTER"},
		{NULLH,                "NULLH"},
		{NET_CONF,             "NET_CONF"},
		{FOX_START,            "FOX_START"},
		{FOX_END,              "FOX_END"},
		{GPIORTT,              "GPIORTT"},
		{BUFFER_IDENTITY,      "BUFFER_IDENTITY"},
    {BUFFER_IDENTITY_COMP, "BUFFER_IDENTITY_COMP"},
};

bool str_to_handler(std::string s, handler_type * t) {
  auto it = String2HandlerMap.find(s);
  if (it == String2HandlerMap.end()) {
    return false;
  }
  *t = it->second;
  return true;
}

bool handler_to_str(handler_type t, std::string ** s) {
  auto it = Handler2StringMap.find(t);
  if (it == Handler2StringMap.end()) {
    return false;
  }
  *s = &it->second;
  return true;
}

std::map<log_format, std::string> LogFormat2StringMap = {
  {ASCII,                  "a"},
  {BINARY,                 "b"},
  {ZSTD,                   "z"},
  {LZO1X,                  "l"},
  {ASCII_TSC,              "i"},
  {BINARY_TSC,             "y"},
  {ZSTD_TSC,               "d"},
  {LZO1X_TSC,              "x"},
};

std::map<std::string, log_format> String2LogFormatMap = {
  {"a",                  ASCII},
  {"b",                  BINARY},
  {"z",                  ZSTD},
  {"l",                  LZO1X},
  {"i",                  ASCII_TSC},
  {"y",                  BINARY_TSC},
  {"d",                  ZSTD_TSC},
  {"x",                  LZO1X_TSC},
};

bool format_to_str(log_format f, std::string * s) {
  auto it = LogFormat2StringMap.find(f);

  if (it == LogFormat2StringMap.end())
    return false;

  *s = it->second;
  return true;
}

bool str_to_format(std::string s, log_format * f) {
  auto it = String2LogFormatMap.find(s);

  if (it == String2LogFormatMap.end())
    return false;

  *f = it->second;
  return true;
}
