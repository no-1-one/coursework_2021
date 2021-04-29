/**
 * @file sgp_client.cpp
 * @author Baleskin Vitaly (vbaleskin@hse.ru)
 * @brief 
 * @version 0.1
 * @date 2020-12-06
 * 
 * @copyright Copyright (c) NRU HSE 2020
 * 
 */


#include <chrono>  // linter failure
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>  // same as chrono
#include <future>

#include "grpc/grpc.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"

#include "sgp_service.grpc.pb.h"  // to be ignored. this is due to the difference between structures
#include "igrf_service.grpc.pb.h"  // to be ignored. this is due to the difference between structures
#include "sgp4/include/DateTime.h"
#include "sgp4/include/Eci.h"
#include "sgp4/include/CoordGeodetic.h"

#define CATCH_CONFIG_MAIN
#include "../testing/catch.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using IGRF::IGRFService;
using IGRF::Point;
using IGRF::PointResult;
using IGRF::TLEComputeRequest;
using IGRF::TLEComputeResponse;
using SGP::SGPConstructRequest;
using SGP::SGPConstructResponse;
using SGP::CoordType;
using IGRF::EndRequest;
using IGRF::EndResponse;



FILE* handle = NULL;
std::string id = "UNKNOWN";
SCENARIO("IGRF Service's client can be instantiated and used") {
  GIVEN("A Server, a basic channel, a stub and a context") {
    if (!handle) {
      handle = popen("./igrf_server 9091 --test", "w");
      if (!handle) std::cout << "ERROR";
      sleep(1);
    }
    auto channel = grpc::CreateChannel("localhost:9091",
                                       grpc::InsecureChannelCredentials());
    std::unique_ptr<IGRFService::Stub> stub = IGRFService::NewStub(channel);
    ClientContext context;

    /* WHEN("Construction is invoked") {
      SGPConstructRequest request;
      request.set_title("ISS");
      request.set_first(
          "1 25544U 98067A   20173.69712963  "
          ".00000371  00000-0  14697-4 0  9998");
      request.set_second(
          "2 25544  51.6445 326.1422 0002733  "
          "71.2042 114.5589 15.49451886232717");
      SGPConstructResponse response;
      Status status = stub->SGPConstruct(&context, request, &response);
      THEN("An OK status and non-empty id are returned") {
        REQUIRE(status.ok());
        REQUIRE_FALSE(response.computational_id().empty());
        id = response.computational_id();
      }
    } */

    WHEN("Computation is invoked for point") {
      Point request;
      auto c = request.add_coord();
      c->set_lon(0);
      c->set_lat(0);
      c->set_alt(0);
      c->set_encoded_time(DateTime::Now().Ticks());
      request.set_add_noise_to_igrf(false);
      PointResult response;
      Status status = stub->computeForPoint(&context, request, &response);
      THEN("An OK status is received, alongside a valid result") {
        REQUIRE(status.ok());
        REQUIRE(!response.result().empty());
        REQUIRE(abs(response.result().begin()->y()) > 1e-3);
      }
    }


    WHEN("Construction is invoked") {
      SGPConstructRequest request;
      request.set_title("ISS");
      request.set_first(
          "1 25544U 98067A   21022.22515229 "
          "-.00006244  00000-0 -10577-3 0  9996");
      request.set_second(
          "2 25544  51.6469 344.6609 0002238 "
          "275.3350 157.6252 15.48872772265973");
      SGPConstructResponse response;
      Status status = stub->construct(&context, request, &response);
      THEN("An OK status and non-empty id are returned") {
        REQUIRE(status.ok());
        REQUIRE_FALSE(response.computational_id().empty());
        id = response.computational_id();
      }
    }

    WHEN("Computation is invoked for TLE") {
      TLEComputeRequest request;
      request.set_add_noise_to_igrf(false);
      request.set_add_noise_to_sgp(false);
      request.set_computational_id(id);
      request.add_encoded_time(DateTime::Now().Ticks());
      TLEComputeResponse response;
      Status status = stub->computeTLE(&context, request, &response);
      THEN("An OK status is received, alongside a valid result") {
        REQUIRE(status.ok());
        REQUIRE(!response.results().result().empty());
        REQUIRE(response.results().result().size() == 1);
        REQUIRE(
          abs(response.results().result().begin()->y())
          + abs(response.results().result().begin()->z()) > 1e-3);
        // std::cout << std::endl
        //     << abs(response.results().result().begin()->x()) << " "
        //     << abs(response.results().result().begin()->y()) << " "
        //     << abs(response.results().result().begin()->z());
      }
    }

    WHEN("Connection is, finally, closed") {
      EndRequest request;
      request.set_computational_id(id);
      EndResponse response;
      Status status = stub->endWork(&context, request, &response);
      THEN("An OK status is received") {
        // std::cout << status.error_code() << " " << status.error_message();
        REQUIRE(status.ok());
        fprintf(handle, "STOP");
        pclose(handle);
      }
    }
  }
}

/* class SGPClient {
 public:
  explicit SGPClient(std::shared_ptr<Channel> channel)
      : stub_(SGPService::NewStub(channel)) {}

  bool RequestConstruction(bool supress_output) {
    ClientContext context;
    SGPConstructRequest request;
    request.set_title("ISS");
    request.set_first(
        "1 25544U 98067A   20173.69712963  "
        ".00000371  00000-0  14697-4 0  9998");
    request.set_second(
        "2 25544  51.6445 326.1422 0002733  "
        "71.2042 114.5589 15.49451886232717");

    SGPConstructResponse response;
    Status status = stub_->SGPConstruct(&context, request, &response);
    if (!status.ok()) {
      std::cout << "ERROR[" << status.error_code() << "]: "
          << status.error_message() << std::endl;
      std::cout << "DETAILS: " << status.error_details() << std::endl;
      return false;
    }
    if (!supress_output) std::cout << "COMPUTATIONAL ID: "
        << response.computational_id() << std::endl;
    id = response.computational_id();
    return true;
  }

  bool RequestComputation(bool inGeo,
                          const std::vector<DateTime>& time,
                          bool supress_output,
                          bool use_noise = false) {
    ClientContext context;
    SGPComputeRequest request;
    for (auto& dt : time) {
      request.add_encoded_time(dt.Ticks());
    }
    request.set_use_noise(use_noise);
    request.set_computational_id(id);
    request.set_coord_type(inGeo
                           ? SGP::CoordType::GEODETIC
                           : SGP::CoordType::ECI);
    SGPComputeResponse response;

    Status status = stub_->SGPCompute(&context, request, &response);
    if (!status.ok()) {
      std::cout << "ERROR[" << status.error_code()
                << "]: " << status.error_message() << std::endl;
      std::cout << "DETAILS: " << status.error_details() << std::endl;
      return false;
    }

    if (inGeo) {
      auto& proto_geos = response.geodetic();
      for (auto& proto_geo : proto_geos) {
        CoordGeodetic geo{proto_geo.lat(),
                          proto_geo.lon(),
                          proto_geo.alt(),
                          true};
        if (!supress_output) std::cout << "COORDS (GEODETIC): "
            << std::endl << geo.ToString() << std::endl;
      }
    } else {
      auto& proto_ecis = response.eci();
      for (auto& proto_eci : proto_ecis) {
        DateTime time{proto_eci.encoded_time()};
        Vector pos, vel;
        pos.x = proto_eci.position().x();
        pos.y = proto_eci.position().y();
        pos.z = proto_eci.position().z();
        vel.x = proto_eci.velocity().x();
        vel.y = proto_eci.velocity().y();
        vel.z = proto_eci.velocity().z();
        Eci eci{time, pos, vel};
        if (!supress_output) {
          std::cout << "COORDS (ECI) : " << std::endl;
          std::cout << "POSITION: " << eci.Position().ToString() << std::endl;
          std::cout << "VELOCITY: " << eci.Velocity().ToString() << std::endl;
          std::cout << "TIME: " << eci.GetDateTime().ToString() << std::endl;
        }
      }
    }
    return true;
  }

  bool ScrambleUserRequestComputation() {
    auto old = id;
    id += "]";
    auto res = RequestComputation(false, {DateTime::Now()}, false);
    id = old;
    return res;
  }

 private:
  std::unique_ptr<SGPService::Stub> stub_;
  std::string id;
};

std::vector<DateTime> getTimeSequence(size_t n) {
  std::vector<DateTime> times;
  times.reserve(n);
  auto source = DateTime::Now();
  for (size_t i = 0; i < n; ++i) times.push_back(source.AddSeconds(i));
  return times;
} */


/* int main(int argc, char** argv) {
  SGPClient consumer(
      grpc::CreateChannel("localhot:9090", grpc::InsecureChannelCredentials()));
  std::cout << "---------- RequestConstruction -----------" << std::endl;
  consumer.RequestConstruction(false);
  std::cout << "--------- RequestComputationECI  ---------" << std::endl;
  consumer.RequestComputation(false, {DateTime::Now()}, false);
  std::cout << "--------- RequestComputationGEO  ---------" << std::endl;
  auto d = DateTime::Now();
  consumer.RequestComputation(true, {d}, false);
  consumer.RequestComputation(true, {d}, false, true);
  std::cout << "---- RequestComputationScrambledUser  ----" << std::endl;
  consumer.ScrambleUserRequestComputation();
  char ans;
  std::cout << "proceed with x1M timing tests? (y/n)";
  std::cin >> ans;
  if (ans == 'y') {
    std::cout << "TIMING_TESTS:" << std::endl;
    system("sleep 10");
    std::cout <<  "CONSTRUCT TESTS" << std::endl;
    auto construct_start = std::chrono::steady_clock::now();
    for (auto i = 1; i <= 1e6; ++i) {
      consumer.RequestConstruction(true);
    }
    auto construct_end = std::chrono::steady_clock::now();
    std::cout << "COMPUTE TESTS" << std::endl;
    std::vector<double> periods;
    for (size_t n_sets=1; n_sets < 102; n_sets+=10) {
      std::cout << "\t" << n_sets << " Sets per package" << std::endl;
      auto compute_start = std::chrono::steady_clock::now();
      for (auto i = 1; i <= 1e4; ++i) {
        consumer.RequestComputation(false, getTimeSequence(n_sets), true, true);
      }
      auto compute_end = std::chrono::steady_clock::now();
      periods.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
          compute_end - compute_start).count()/1.0e4);
    }
    std::cout << "construct avg: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                  construct_end - construct_start).count()/1.0e6
              << std::endl;
    std::cout << "compute avg: " << std::endl;
    size_t j = 0;
    for (size_t i=1; i < 102; i+=10) {
      std::cout << "\tSets per package: " << i
                << ". 10KAvg(per set of computations): "
                << periods[j]/i << std::endl;
      ++j;
    }
  }
  return 0;
}
 */
