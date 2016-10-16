#pragma once

#include <stdio.h>

#include <memory>
#include <string>
#include <unordered_map>

std::unique_ptr<FILE, void(*)(FILE*)> popen_unique(const std::string& command,
    const std::string& mode);

std::string name_for_pid(pid_t pid);
pid_t pid_for_name(const std::string& name, bool search_commands = true, bool exclude_self = true);

std::unordered_map<pid_t, std::string> list_processes(bool with_commands = true);
