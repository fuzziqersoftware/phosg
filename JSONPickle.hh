#pragma once

#include <string>

#include "JSON.hh"


JSONObject parse_pickle(const std::string& pickle_data);
JSONObject parse_pickle(const void* pickle_data);
JSONObject parse_pickle(const void* data, size_t size);

std::string serialize_pickle(const JSONObject& o);
