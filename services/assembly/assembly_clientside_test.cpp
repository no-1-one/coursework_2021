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
#include <cassert>
#include <unistd.h>

#include "grpc/grpc.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"

#include "assembly.grpc.pb.h"

using grpc::ClientContext;
using grpc::Status;
using grpc::StatusCode;

using Assembly::ModUnit;
using Assembly::Pair;
using Assembly::Configuration;
using Assembly::ConfigurationName;
using Assembly::ArchitecturesAndModules;
using Assembly::Empty;
using Assembly::AssemblyService;



int main() {
  std::string id = "UNKNOWN";


  auto channel = grpc::CreateChannel("localhost:9090",
                                      grpc::InsecureChannelCredentials());
  std::unique_ptr<AssemblyService::Stub> stub = AssemblyService::NewStub(channel);

  //
  //rpc ReturnArchitecturesAndModules(Empty) returns (ArchitecturesAndModules) {}
  {
    ModUnit tmp;
    tmp.set_size(3);
    tmp.set_name("flywheel");

    Empty request;
    ArchitecturesAndModules response;
    ClientContext context;
    std::cout << "FIRST TEST" << std::endl;
    Status status = stub -> ReturnArchitecturesAndModules(&context, request, &response);
    std::cout << status.error_code();// << " " << response.all_architectures()[1] << std::endl;

    assert(("Configuration 1",
          (status.error_code() == StatusCode::OK)
          ));
    assert(("Configuration 2",
          (response.all_architectures().size() != 0)
          ));
    assert(("Configuration 3",
          (response.all_architectures()[1] == "u2")
          ));
    assert(("Configuration 4",
          (response.all_modules()[1].size() == tmp.size())
          ));
    assert(("Configuration 5",
          (response.all_modules()[1].name() == tmp.name())
          ));

    assert(("Search configuration works correctly.",
        (status.error_code() == StatusCode::OK) &&
        (response.all_architectures().size() != 0) &&
        (response.all_architectures()[1] == "u2") &&
        (response.all_modules()[1].size() == tmp.size()) &&
        (response.all_modules()[1].name() == tmp.name())));
  }

  //
  //rpc RegisterConfiguration(Configuration) returns (Result) {}

  //"Try to RegisterConfiguration with non-existent architecture."
  {
    Configuration request;
    request.set_name("new_test1");
    request.set_architecture("u5");
    std::unordered_map<std::string, ModUnit> mod_map;
    mod_map["3"].set_name("flywheel");
    mod_map["1"].set_name("transceiver");

    for (auto& [k,v] : mod_map) {
        Pair p;
        p.set_key(k);
        *(p.mutable_value()) = v;
        *(request.add_included_mod()) = p;
    }
    Empty response;
    ClientContext context;
    Status status = stub -> RegisterConfiguration(&context, request, &response);
    assert(("The server did not refuse to register the misconfiguration. Test 1.", status.error_code() == StatusCode::NOT_FOUND));
  }

  //"Try to RegisterConfiguration with non-existent module."
  {
    Configuration request;
    request.set_name("new_test2");
    request.set_architecture("u2");
    std::unordered_map<std::string, ModUnit> mod_map;
    mod_map["3"].set_name("abracadabra");
    mod_map["1"].set_name("transceiver");

    for (auto& [k,v] : mod_map) {
        Pair p;
        p.set_key(k);
        *(p.mutable_value()) = v;
        *(request.add_included_mod()) = p;
    }
    Empty response;
    ClientContext context;
    Status status = stub -> RegisterConfiguration(&context, request, &response);
    assert(("The server did not refuse to register the misconfiguration. Test 2.", status.error_code() == StatusCode::NOT_FOUND));
  }

  //"Try to RegisterConfiguration correct configuration."
  {
    Configuration request;
    request.set_name("new_test3");
    request.set_architecture("u2");
    std::unordered_map<std::string, ModUnit> mod_map;
    mod_map["7"].set_name("flywheel");
    mod_map["1"].set_name("transceiver");

    for (auto& [k,v] : mod_map) {
        Pair p;
        p.set_key(k);
        *(p.mutable_value()) = v;
        *(request.add_included_mod()) = p;
    }
    Empty response;
    ClientContext context;
    Status status = stub -> RegisterConfiguration(&context, request, &response);
    assert(("The server did not refuse to register the configuration. Test 3."+status.error_message(), status.error_code() == StatusCode::OK));
  }

  //"Try to RegisterConfiguration with more units used then available."
  {
    Configuration request;
    request.set_name("new_test4");
    request.set_architecture("u2");
    std::unordered_map<std::string, ModUnit> mod_map;
    mod_map["3"].set_name("flywheel");
    mod_map["1"].set_name("transceiver");
    mod_map["1"].set_name("avs");
    mod_map["1"].set_name("power_supply_system");
    mod_map["5"].set_name("camera");

    for (auto& [k,v] : mod_map) {
        Pair p;
        p.set_key(k);
        *(p.mutable_value()) = v;
        *(request.add_included_mod()) = p;
    }
    Empty response;
    ClientContext context;
    Status status = stub -> RegisterConfiguration(&context, request, &response);
    assert(("The server did not refuse to register the misconfiguration. Test 4.", status.error_code() == StatusCode::INVALID_ARGUMENT));
  }

  //"Try to RegisterConfiguration with not correct configuration."
  {
    Configuration request;
    request.set_name("new_test5");
    request.set_architecture("u2");
    std::unordered_map<std::string, ModUnit> mod_map;
    mod_map["3"].set_name("flywheel");
    mod_map["1"].set_name("transceiver");
    mod_map["1"].set_name("avs");
    mod_map["2"].set_name("onboard_computer");
    mod_map["1"].set_name("power_supply_system");
    mod_map["2"].set_name("magnetic_coils");

    for (auto& [k,v] : mod_map) {
        Pair p;
        p.set_key(k);
        *(p.mutable_value()) = v;
        *(request.add_included_mod()) = p;
    }
    Empty response;
    ClientContext context;
    Status status = stub -> RegisterConfiguration(&context, request, &response);
    assert(("Register configuration works not correctly. Test 4.", status.error_code() == StatusCode::INVALID_ARGUMENT));
  }

  //"Try to RegisterConfiguration with a duplicate name.");
  {
    Configuration request;
    request.set_name("new_test5");
    request.set_architecture("u2");
    std::unordered_map<std::string, ModUnit> mod_map;
    mod_map["1"].set_name("avs");

    for (auto& [k,v] : mod_map) {
        Pair p;
        p.set_key(k);
        *(p.mutable_value()) = v;
        *(request.add_included_mod()) = p;
    }
    Empty response;
    ClientContext context;
    Status status = stub -> RegisterConfiguration(&context, request, &response);
    assert(("The server did not refuse to register the misconfiguration. Test 5.", status.error_code() == StatusCode::ALREADY_EXISTS));
  }

  //
  //rpc SearchConfiguration(ConfigurationName) returns(Configuration) {}

  //"Try to SearchConfiguration with exist configuration."
  {
    ConfigurationName request;
    request.set_name("new_test6");
    ClientContext context;
    Configuration response;
    Status status = stub -> SearchConfiguration(&context, request, &response);

    assert(("Search 1",
          (status.error_code() == StatusCode::OK)
          ));
    assert(("Search 2",
          (response.name() == "new_test6")
          ));
    assert(("Search 3",
          (response.architecture() == "u2")
          ));

    assert(("Configuration 4", (response.included_mod().size() == 5)));

    ModUnit tmp;
    std::unordered_map<std::string, ModUnit> true_mod;

    for (auto mud: response.included_mod()) {
      true_mod[mud.key()] = mud.value();
    }

    tmp.set_size(3);
    tmp.set_name("flywheel");
    assert(("Configuration 5.1",
          (true_mod["5"].size() == tmp.size())
          ));
    assert(("Configuration 5.2",
          (true_mod["5"].name() == tmp.name())
          ));

    tmp.set_size(1);
    tmp.set_name("transceiver");
    assert(("Configuration 6.1",
          (true_mod["3"].size() == tmp.size())
          ));
    assert(("Configuration 6.2",
          (true_mod["3"].name() == tmp.name())
          ));

    tmp.set_size(1);
    tmp.set_name("sun_sensor");
    assert(("Configuration 7.1",
          (true_mod["4"].size() == tmp.size())
          ));
    assert(("Configuration 7.2",
          (true_mod["4"].name() == tmp.name())
          ));

    tmp.set_size(2);
    tmp.set_name("onboard_computer");
    assert(("Configuration 8.1",
          (true_mod["0"].size() == tmp.size())
          ));
    assert(("Configuration 8.2",
          (true_mod["0"].name() == tmp.name())));

    tmp.set_size(1);
    tmp.set_name("power_supply_system");
    assert(("Configuration 9.1",
          (true_mod["2"].size() == tmp.size())
          ));
    assert(("Configuration 9.2",
          (true_mod["2"].name() == tmp.name())
          ));


    assert(("Search configuration works not correctly.",
           (status.error_code() == StatusCode::OK) &&
           (response.name() == "new_test6") &&
           (response.architecture() == "u2")));
  }
  //"Try to SearchConfiguration with non-existent configuration."
  {
    ConfigurationName request;
    request.set_name("non-existent_conf");
    ClientContext context;
    Configuration response;
    Status status = stub -> SearchConfiguration(&context, request, &response);
    assert(("Search configuration works not correctly.", (status.error_code() == StatusCode::NOT_FOUND)));
  }

 }

/*
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

using Assembly::AssemblyService;

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

*/
