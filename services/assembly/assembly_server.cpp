/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "examples/protos/helloworld.grpc.pb.h"

#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"

#include "assembly_service.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using AssemblyService::ReadArchitecturesAndModules;
using AssemblyService::ReturnArchitecturesAndModules;
using AssemblyService::ReadConfiguration;
using AssemblyService::CheckingConfiguration;
using AssemblyService::RegistrationСonfiguration;
using AssemblyService::SearchConfiguration;

using AssemblyService::ModUnit;
using AssemblyService::Pair;
using AssemblyService::Configuration;
using AssemblyService::ConfigurationName;
using AssemblyService::ArchitecturesAndModules;

using AssemblyService::AssemblyService;


struct ModUnit {
    std::string name;
    int size;
    bool operator == (const ModUnit& other) const {
        return name == other.name;
    }
};

template<> struct std::hash<ModUnit> {
    size_t operator()(const ModUnit& modref) const {
        return std::hash<std::string>()(modref.name);
    }
};

struct Config {
    std::string name;
    std::string architecture;
    std::unordered_map<std::string, ModUnit> included_mod;
};


class AssemblyServiceServer {
public:
    //part 0 in class data
    std::unordered_set<std::string> all_architectures;
    std::unordered_set<ModUnit> all_modules;
    std::unordered_set<std::string> config_name;
    Config inp;

    //part 1 read Architactures and Modules
    std::string ArchitactureRead () {
        std::ifstream architectures_file("../../../config/architectures.txt");
        if (!architectures_file.is_open()) {
            return ("Architecture file not found!");
        }
        std::string inp_val;
        while (architectures_file >> inp_val) {
            all_architectures.insert(inp_val);
        }
        architectures_file.close();
        return ("OK");
    }

     std::string ModuleRead () {
        std::ifstream modules_file("../../../config/modules.txt");
        if (!modules_file.is_open()) {
            return ("Modules file not found");
        }
        std::string inp_val;
        ModUnit tmp;
        while (modules_file >> inp_val) {
            tmp.size = std::stoi(inp_val);
            (modules_file >> inp_val);
            tmp.name = inp_val;
            all_modules.insert(tmp);
        }
        modules_file.close();
        return ("OK");
    }

    void ReadServerData () {
        std::string architectures_read_err = ArchitactureRead();
        if (architectures_read_err == "OK") {
            architectures_read_err.clear();
        }
        std::string module_read_err = ModuleRead();
        if (module_read_err == "OK") {
            module_read_err.clear();
        }
        if ()!(architectures_read_err.empty() && module_read_err.empty())) {
            return (architectures_read_err + module_read_err);
        }
    }


    //part 2 return Architactures and Modules
    void DatabaseOutput (std::vector<std::string> &architectures_container,
                         std::vector<ModUnit> &modules_container) {
        for (auto tmp : all_architectures) {
            architectures_container.push_back(tmp);
        }
        for (auto tmp : all_modules) {
            modules_container.push_back(tmp);
        }
    }

    Status ReturnArchitecturesAndModules(ServerContext* context,
                                         Empty* empty,
                                         ArchitecturesAndModules* container) {
        DatabaseOutput(container->all_architectures, container->all_modules, all_architectures, all_modules);
        return Status::OK;
    }


    //part 3 Checking Сonfiguration
    std::string is_configuration_correct () {
        if (not((inp.architecture == "u1") || (inp.architecture == "u2") || (inp.architecture == "u3"))) {
            return ("There is no such architecture");
        }
        int avaliable_capasity = std::stoi(inp.architecture.substr(1)) * 5;

        for (auto& [slot, mod] : inp.included_mod) {
            if (all_modules.find(mod) != all_modules.end()) {
                if ((std::stoi(slot) + mod.size) > avaliable_capasity) {
                    return("There are more units used then available");
                }
                /*
                for (int i = (std::stoi(slot)); i <= std::stoi(slot) + mod.size - 1; ++i) {
                    if (inp.included_mod.find(std::to_string(i)) != inp.included_mod.end()) {
                        assert(false && "AAAA!");
                        return false;
                    }
                }
                */
            } else {
                return ("There is no such module");
            }
        }
        return ("OK");
    }

    Status CheckingConfiguration (ServerContext* context,
                                  Configuration* input,
                                  Empty* result) {
        inp.name = input -> name;
        inp.size = input -> size;
        for (auto mod : input -> included_mod) {
            inp.included_mod[mod -> key] = mod -> value;
        }
        std::string res = is_configuration_correct ();
        if (res == "OK") {
            return Status::OK;
        }
        if (res == "There is no such architecture") {
            return Status(NOT_FOUND, res);
        }
        return Status(INVALID_ARGUMENT, res);
    }


    //part 4 Registration Сonfiguration
    std::string ConfigNames() {
        std::ifstream config_file("../../../config/configurations.txt");
        if (!config_file.is_open()) {
            return "Configurations file not found";
        }

        std::string inp_val;
        while (config_file >> inp_val) {
            if (inp_val[0] == '['){
                inp_val = inp_val.substr(1, inp_val.size() - 2);
                if (!(configurations_names.contains(inp_val))) {
                    configurations_names.insert(inp_val);
                }
            }
        }
        config_file.close();
        return "OK";
    }

    Status RegisterConfiguration (ServerContext* context,
                                  Configuration* input,
                                  Empty* result) {
        inp.name = input -> name;
        inp.size = input -> size;
        for (auto mod : input -> included_mod) {
            inp.included_mod[mod -> key] = mod -> value;
        }
        std::string res = is_configuration_correct ();
        if (res == "There is no such architecture") {
            return Status(NOT_FOUND, res);
        }
        if (res == "There is no such module") {
            return Status(NOT_FOUND, res);
        }
        if (res == "There are more units used then available") {
            return Status(INVALID_ARGUMENT, res);
        }
        if (configurations_names.find(inp.name) != configurations_names.end()) {
            return Status(ALREADY_EXISTS, "Configuration with this name is already exist");
        }
        std::ofstream fi_out;
        fi_out.open("../../../config/configurations.txt", std::ios::app);
        fi_out << "\n[" << inp.name << "]" << "\n";
        fi_out << "architecture = " << inp.architecture << "\n";
        for (auto& [slot, mod] : inp.included_mod) {
            fi_out <<  slot << " = " << mod.name << " " << mod.size << "\n";
        }
        fi_out.close();
        return Status::OK;
    }


    // part 5 Config Search
    Config SearchConfiguration(ServerContext* context,
                               ConfigurationName target,
                               Configuration* result) {
        std::ifstream configuration_file("../../../config/configurations.txt");
        bool success_search = false;
        if (!configuration_file.is_open()) {
            return Status(NOT_FOUND, "Configuration file not found");
        }
        std::string inp_val;
        while (configuration_file >> inp_val) {
            if (inp_val[0] == '[' && inp_val.substr(1, inp_val.size() - 2) == target -> name) {
                success_search = true;
                result -> name = target -> name;
                getline(configuration_file, inp_val);
                getline(configuration_file, inp_val);
                result -> architecture = inp_val.substr(inp_val.find("=") + 2);

                std::string key, mod_str;
                ModUnit mod;
                while (getline(configuration_file, inp_val)) {
                    if (inp_val.empty()) {
                        configuration_file.close();
                        return Status::OK;
                    }
                    key = inp_val.substr(0, inp_val.find("=") - 1);
                    mod_str = inp_val.substr(inp_val.find("=") + 2);
                    mod.name = mod_str.substr(0, mod_str.find(" "));
                    mod.size = std::stoi(mod_str.substr(mod_str.find(" ") + 1));
                    result -> included_mod.push_bask(std::pair<key, mod>);
                }
            }
        }
        return Status(NOT_FOUND, "Configuration with this name does not exist");
    }
};


void RunServer(std::string port) {
  std::string server_address("0.0.0.0:"+port);

  AssemblyService service{};
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {

  RunServer(argv[1]);

  return 0;
}
