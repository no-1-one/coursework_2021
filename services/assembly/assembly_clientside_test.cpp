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

#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"

#include "assembly_service.grpc.pb.h" //!

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

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

FILE* handle = NULL;
std::string id = "UNKNOWN";
SCENARIO("Testing standard user actions on assembly service") {
    GIVEN("A Server, a basic channel, a stub and a context") {
        if (!handle) {
          handle = popen("./sgp_server 9090 --test", "w");
          if (!handle) std::cout << "ERROR";
          sleep(1);
        }
        auto channel = grpc::CreateChannel("localhost:9090",
                                           grpc::InsecureChannelCredentials());

        std::unique_ptr<AssemblyService::Stub> stub = AssemblyService::NewStub(channel);
        ClientContext context;

        //rpc ReturnArchitecturesAndModules(Empty) returns (ArchitecturesAndModules) {}
        WHEN("Try to ReturnArchitecturesAndModules") {
              Empty request;
              ArchitecturesAndModules response;
              Status status = stub -> ReturnArchitecturesAndModules(&context, request, &response);
              THEN("Return architectures and modules works correctly") {
                  REQUIRE(status.ok());
              }
        }

        //rpc RegisterConfiguration(Configuration) returns (Result) {}
        WHEN("Try to RegisterConfiguration with non-existent architecture.") {
              Configuration request;
              request.name = "new_test1";
              request.architecture = "u5";
              std::unordered_map<std::string, ModUnit> mod_map;
              mod_map["3"] = "flywheel";
              mod_map["1"] = "transceiver";
              request.included_mod = mod_map;
              Result response;
              Status status = stub -> RegisterConfiguration(&context, request, &response);
              THEN("The server refused to register. Configuration is not correct.") {
                  REQUIRE(status.error_code() == StatusCode::NOT_FOUND);
                  REQUIRE(response == 1);
              }
        }

        WHEN("Try to RegisterConfiguration with non-existent module.") {
              Configuration request;
              request.name = "new_test2";
              request.architecture = "u2";
              std::unordered_map<std::string, ModUnit> mod_map;
              mod_map["3"] = "abracadabra";
              mod_map["1"] = "transceiver";
              request.included_mod = mod_map;
              Result response;
              Status status = stub -> RegisterConfiguration(&context, request, &response);
              THEN("The server refused to register. Configuration is not correct.") {
                  REQUIRE(status.error_code() == StatusCode::NOT_FOUND);
              }
        }

        WHEN("Try to RegisterConfiguration with non-existent module.") {
              Configuration request;
              request.name = "new_test3";
              request.architecture = "u2";
              std::unordered_map<std::string, ModUnit> mod_map;
              mod_map["7"] = "flywheel";
              mod_map["1"] = "transceiver";
              request.included_mod = mod_map;
              Result response;
              Status status = stub -> RegisterConfiguration(&context, request, &response);
              THEN("The server refused to register. Configuration is not correct.") {
                  REQUIRE(status.error_code() == StatusCode::NOT_FOUND);
              }
        }

        WHEN("Try to RegisterConfiguration with more units used then available.") {
              Configuration request;
              request.name = "new_test4";
              request.architecture = "u2";
              std::unordered_map<std::string, ModUnit> mod_map;
              mod_map["3"] = "flywheel";
              mod_map["1"] = "transceiver";
              mod_map["1"] = "avs";
              mod_map["1"] = "power_supply_system";
              mod_map["5"] = "camera";
              request.included_mod = mod_map;
              Result response;
              Status status = stub -> RegisterConfiguration(&context, request, &response);
              THEN("The server refused to register. Configuration is not correct.") {
                  REQUIRE(status.error_code() == StatusCode::INVALID_ARGUMENT);
              }
        }

        WHEN("Try to RegisterConfiguration with correct configuration.") {
              Configuration request;
              request.name = "new_test5";
              request.architecture = "u2";
              std::unordered_map<std::string, ModUnit> mod_map;
              mod_map["3"] = "flywheel";
              mod_map["1"] = "transceiver";
              mod_map["1"] = "avs";
              mod_map["2"] = "onboard_computer";
              mod_map["1"] = "power_supply_system";
              mod_map["2"] = "magnetic_coils";
              request.included_mod = mod_map;
              Result response;
              Status status = stub -> RegisterConfiguration(&context, request, &response);
              THEN("Register configuration works correctly") {
                  REQUIRE(status.ok());
              }
        }

        WHEN("Try to RegisterConfiguration with a duplicate name.") {
              Configuration request;
              request.name = "new_test5";
              request.architecture = "u2";
              std::unordered_map<std::string, ModUnit> mod_map;
              mod_map["1"] = "avs";
              request.included_mod = mod_map;
              Result response;
              Status status = stub -> RegisterConfiguration(&context, request, &response);
              THEN("The server refused to register. Configuration is not correct.") {
                  REQUIRE(status.error_code() == StatusCode::ALREADY_EXISTS);
              }
        }

        //rpc SearchConfiguration(ConfigurationName) returns(Configuration) {}
        WHEN("Try to SearchConfiguration with exist configuration.") {
              ConfigurationName request;
              request.name = "new_test5";
              Configuration response;
              Status status = stub -> SearchConfiguration(&context, request, &response);
              THEN("Search configuration works correctly") {
                  REQUIRE(status.ok());
                  REQUIRE(request.name == "new_test5");
                  REQUIRE(request.architecture == "u2");
              }
        }
    }
}
