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
#include <string_view>
//#include "examples/protos/helloworld.grpc.pb.h"

#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"

#include "assembly.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using Assembly::ModUnit;
using Assembly::Pair;
using Assembly::Configuration;
using Assembly::ConfigurationName;
using Assembly::ArchitecturesAndModules;
using Assembly::Empty;

using Assembly::AssemblyService;


struct ModUnitStruct {
    std::string name;
    int size;
    bool operator == (const ModUnitStruct& other) const {
        return name == other.name;
    }
};

template<> struct std::hash<ModUnitStruct> {
    size_t operator()(const ModUnitStruct& modref) const {
        return std::hash<std::string>()(modref.name);
    }
};

struct Config {
    std::string name;
    std::string architecture;
    std::unordered_map<std::string, ModUnitStruct> included_mod;
};


class AssemblyServiceServer : public AssemblyService::Service{
public:
    //part 0 in class data
    std::unordered_set<std::string> all_architectures;
    std::unordered_set<ModUnitStruct> all_modules;
    std::unordered_set<std::string> configurations_names;
    

    //part 1 read Architactures and Modules
    std::string ArchitectureRead () {
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
        ModUnitStruct tmp;
        while (modules_file >> inp_val) {
            tmp.size = std::stoi(inp_val);
            (modules_file >> inp_val);
            tmp.name = inp_val;
            all_modules.insert(tmp);
        }
        modules_file.close();
        return ("OK");
    }

    std::string ReadServerData () {
        std::string architectures_read_err = ArchitectureRead();
        if (architectures_read_err == "OK") {
            architectures_read_err.clear();
        }
        std::string module_read_err = ModuleRead();
        if (module_read_err == "OK") {
            module_read_err.clear();
        }
        if (!(architectures_read_err.empty() && module_read_err.empty())) {
            return (architectures_read_err + module_read_err);
        }
        return "OK; Read " + std::to_string(all_architectures.size()) + " architectures and " + std::to_string(all_modules.size()) + " modules";

    }

  
    //part 2 return Architactures and Modules
    template<class CONT>
    void DatabaseOutput (CONT* architectures_and_modules_container) {
        std::cout << all_architectures.size();
        for (auto tmp : all_architectures) {
            architectures_and_modules_container->add_all_architectures(tmp);// container->all_architectures().Add(tmp)
            std::cout << tmp << std::endl;
        }
        for (auto tmp : all_modules) {
            ModUnit mu;
            mu.set_name(tmp.name);
            mu.set_size(tmp.size);
            std::cout << tmp.name << " " << tmp.size << std::endl;
            *(architectures_and_modules_container->add_all_modules()) = mu;
        }
    }

    Status ReturnArchitecturesAndModules(ServerContext* context,
                                         const Empty* empty,
                                         ArchitecturesAndModules* container) {
        DatabaseOutput(container);  
        return grpc::Status::OK;
    }


    //part 3 Checking Сonfiguration
    std::string is_configuration_correct (Config& inp) {
        
        if (!((inp.architecture == "u1") || (inp.architecture == "u2") || (inp.architecture == "u3"))) {
            return ("There is no such architecture");
        }
        int avaliable_capasity = std::stoi(inp.architecture.substr(1)) * 5;

        for (auto& [slot, mod] : inp.included_mod) {
            if (auto it = all_modules.find(mod); it != all_modules.end()) {
                auto actual_size = it->size;
                if ((std::stoi(slot) + actual_size) > avaliable_capasity) {
                    return("There are more units used then available");
                }
                
                for (int i = (std::stoi(slot))+1; i < std::stoi(slot) + actual_size; ++i) {
                    if (inp.included_mod.find(std::to_string(i)) != inp.included_mod.end()) {
                        return "Modules overlap";
                    }
                }
                
            } else {
                return ("There is no such module");
            }
        }
        return ("OK");
    }

    Status CheckingConfiguration (ServerContext* context,
                                  const Configuration* input,
                                  Empty* result) {
        Config inp;
        inp.name = input->name();
        inp.architecture = input->architecture();
        for (auto mod : input->included_mod()) {
            ModUnitStruct mus;
            mus.name = mod.value().name();
            mus.size = mod.value().size();
            inp.included_mod[mod.key()] = mus;
        }
        std::string res = is_configuration_correct (inp);
        if (res == "OK") {
            return grpc::Status::OK;
        }
        if (res == "There is no such architecture" || res == "There is no such module") {
            return grpc::Status(StatusCode::NOT_FOUND, res);
        }
        return grpc::Status(StatusCode::INVALID_ARGUMENT, res);
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
                if (configurations_names.find(inp_val) == configurations_names.end()) {
                    configurations_names.insert(inp_val);
                }
            }
        }
        config_file.close();
        return "OK";
    }

    Status RegisterConfiguration (ServerContext* context,
                                  const Configuration* input,
                                  Empty* result) {
        Config inp;
        inp.name = input -> name();
        inp.architecture = input -> architecture();
        for (auto mod : input -> included_mod()) {
            ModUnitStruct mus;
            mus.name = mod.value().name();
            mus.size = mod.value().size();
            inp.included_mod[mod.key()] = mus;
        }
        std::string res = is_configuration_correct (inp);
        if (res == "There is no such architecture") {
            return grpc::Status(StatusCode::NOT_FOUND, res);
        }
        if (res == "There is no such module") {
            return grpc::Status(StatusCode::NOT_FOUND, res);
        }
        if (res == "There are more units used then available" || res == "Modules overlap") {
            return grpc::Status(StatusCode::INVALID_ARGUMENT, res);
        }
        if (configurations_names.find(inp.name) != configurations_names.end()) {
            return grpc::Status(StatusCode::ALREADY_EXISTS, "Configuration with this name is already exist");
        }
        configurations_names.insert(inp.name);
        std::ofstream fi_out;
        fi_out.open("../../../config/configurations.txt", std::ios::app);
        fi_out << "\n[" << inp.name << "]" << "\n";
        fi_out << "architecture = " << inp.architecture << "\n";
        for (auto& [slot, mod] : inp.included_mod) {
            fi_out <<  slot << " = " << mod.name << " " << all_modules.find(mod)->size << "\n";
        }
        fi_out.close();
        
        return grpc::Status::OK;
    }


    // part 5 Config Search
    Status SearchConfiguration(ServerContext* context,
                               const ConfigurationName* target,
                               Configuration* result) {
        std::ifstream configuration_file("../../../config/configurations.txt");
        bool success_search = false;
        if (!configuration_file.is_open()) {
            return grpc::Status(StatusCode::NOT_FOUND, "Configuration file not found");
        }
        std::string inp_val;
        while (configuration_file >> inp_val) {
            /* std::string_view view(inp_val);
            if (view.at(0) == '[' && view.find(target->name()) != std::string_view::npos) std::cout<<"found"<<std::endl;
            else std::cout<<"not found"<<std::endl; */


            if (inp_val[0] == '[' && inp_val.substr(1, inp_val.size() - 2) == target->name()) {
                success_search = true;
                result -> set_name(target->name());
                getline(configuration_file, inp_val);
                getline(configuration_file, inp_val);
                result -> set_architecture(inp_val.substr(inp_val.find("=") + 2));

                std::string key, mod_str;
                ModUnit mod;
                while (getline(configuration_file, inp_val)) {
                    if (inp_val.empty()) {
                        configuration_file.close();
                        return grpc::Status::OK;
                    }
                    key = inp_val.substr(0, inp_val.find("=") - 1);
                    mod_str = inp_val.substr(inp_val.find("=") + 2);

                    mod.set_name(mod_str.substr(0, mod_str.find(" ")));
                    mod.set_size(std::stoi(mod_str.substr(mod_str.find(" ") + 1)));
                    Pair p;
                    p.set_key(key);
                    *(p.mutable_value()) = mod;
                    *(result -> add_included_mod()) = p;
                }
            }
        }
        return grpc::Status(StatusCode::NOT_FOUND, "Configuration with this name does not exist");
    }
};


void RunServer(std::string port, bool test = false) {
  std::string server_address("0.0.0.0:"+port);

  AssemblyServiceServer service{};
  std::cout << service.ReadServerData() << std::endl;
  std::cout << service.ConfigNames() << std::endl;
  for (auto config : service.configurations_names) std::cout << config << std::endl;
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  if (test) {
    bool keep_going = true;
    std::string s;
    while (keep_going) {
      std::cin >> s;
      if (s == "STOP") keep_going = false;
      pthread_yield();
    }
    server->Shutdown();
  } else {
    server->Wait();
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
  std::cout << "Usage: ./assembly_server N [OPTIONS]\n N - port number\n"
            << "OPTIONS include --test, which implies using a pipe";
  return 1;
  }
  RunServer(argv[1], argc == 3 && strcmp(argv[2], "--test") == 0
                      ? true
                      : false);

  return 0;
}
