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
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"
#include "grpcpp/security/server_credentials.h"

#include "igrf_service.grpc.pb.h"
#include "sgp_service.grpc.pb.h"
#include "sgp4/include/Tle.h"
#include "sgp4/include/SGP4.h"
#include "noise_application.h"
#include "igrf/include/geomag70.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;
using grpc::Channel;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using SGP::SGPService;
using SGP::SGPConstructRequest;
using SGP::SGPComputeRequest;
using SGP::SGPComputeResponse;
using SGP::SGPConstructResponse;
using SGP::CloseRequest;
using SGP::CloseResponse;

using IGRF::TLEComputeRequest;
using IGRF::TLEComputeResponse;
using IGRF::EndRequest;
using IGRF::EndResponse;
using IGRF::Point;
using IGRF::PointResult;
using IGRF::IGRFService;
// using proto_computation_result = IGRF::igrf_computation_result;
// using proto_computation_variance = IGRF::igrf_computation_secular_variance;

using SGP::CoordType;
// using SGP::CoordGeodetic;
using SGP::CoordECI;
// using SGP::Vector;
using std::chrono::system_clock;

float ConvertToRadians(float num) {
  return num * 3.1415926 /180;
}


class IGRFServiceImpl : public IGRFService::Service {
 public:
  explicit IGRFServiceImpl(std::shared_ptr<Channel> channel)
      : stub_(SGPService::NewStub(channel)) {
  }

  Status construct(ServerContext* context_,
        const SGPConstructRequest* request,
        SGPConstructResponse* response) {
    ClientContext context;
    Status status = stub_->SGPConstruct(&context, *request, response);
    return status;
  }


  Status computeForPoint(ServerContext* context_,
              const Point* dot,
              PointResult* dot_res) {
    for (auto& coord : dot->coord()) {
      uint64_t time = coord.encoded_time();
      // std::cout << "acc " << time;
      DateTime date(time);
      int year = 0, month = 0, day = 0;
      date.FromTicks(year, month, day);
      std::string str_date;
      str_date = std::to_string(year) + ","
          + std::to_string(month) + ","
          + std::to_string(day);
      igrf_request IGRFrequest;
      IGRFrequest.date = reinterpret_cast<char*>(malloc(str_date.length()+1));
      snprintf(IGRFrequest.date, str_date.length()+1, "%s",  str_date.c_str());
      IGRFrequest.coord_type = 'D';
      IGRFrequest.altitude_type = 'K';
      IGRFrequest.altitude = coord.alt();
      IGRFrequest.latitude = coord.lat();
      IGRFrequest.longitude = coord.lon();
      igrf_computation IGRFcomputation;
      igrf_compute(&IGRFrequest, &IGRFcomputation);
      igrf_computation_result computation_result = IGRFcomputation.result;
      igrf_computation_secular_variance variance = IGRFcomputation.variance;
      std::cout << computation_result.y << "\n";
      if (dot->add_noise_to_igrf()) {
        GaussianNoise<PseudoNoiseMixin> noise(0, 50);
        if (IGRFcomputation.result.has_x) {
          computation_result.x += noise.Apply();
        }
        computation_result.y += noise.Apply();
        computation_result.z += noise.Apply();
        if (IGRFcomputation.result.has_declination) {
          computation_result.declination += noise.Apply();
        }
        computation_result.inclination += noise.Apply();
      }

      auto tmp_result = dot_res->add_result();  // tmp_result is a pointer
      tmp_result->set_sdate(computation_result.sdate);
      if (IGRFcomputation.result.has_declination) {
        tmp_result->set_declination(computation_result.declination);
      }
      tmp_result->set_inclination(computation_result.inclination);
      if (IGRFcomputation.result.has_x) {
        tmp_result->set_x(computation_result.x);
      }
      tmp_result->set_y(computation_result.y);
      tmp_result->set_z(computation_result.z);

      tmp_result->set_horizontal_intensity(
          computation_result.horizontal_intensity);
      tmp_result->set_total_intensity(computation_result.total_intensity);

      auto tmp_variance = dot_res->add_variance();  // tmp_result is a pointer
      if (IGRFcomputation.variance.has_declination) {
        tmp_variance->set_declination_dot(variance.declination_dot);
      }
      tmp_variance->set_inclination_dot(variance.inclination_dot);
      if (IGRFcomputation.variance.has_x) {
        tmp_variance->set_x_dot(variance.x_dot);
      }
      tmp_variance->set_y_dot(variance.y_dot);
      tmp_variance->set_z_dot(variance.z_dot);

      tmp_variance->set_horizontal_intensity_dot(
          variance.horizontal_intensity_dot);
      tmp_variance->set_total_intensity_dot(variance.total_intensity_dot);

      dot_res->add_error_code(IGRFcomputation.error_code);
    }
    return Status::OK;
  }
  ///////////////

  Status computeTLE(ServerContext* context_,
                    const TLEComputeRequest* TLErequest,
                    TLEComputeResponse* TLEresponse) {
    SGPComputeRequest SGPRequest;
    SGPRequest.set_computational_id(TLErequest->computational_id());
    for (auto& t : TLErequest->encoded_time()) {
        SGPRequest.add_encoded_time(t);
    }
    SGPRequest.set_coord_type(SGP::CoordType::GEODETIC);
    SGPRequest.set_use_noise(TLErequest->add_noise_to_sgp());
    ClientContext client_context;
    SGPComputeResponse SGPResponse;
    Status status = stub_->SGPCompute(&client_context,
                                      SGPRequest, &SGPResponse);
    if (!status.ok())
      std::cout << "SGPSTAT:" << status.error_code() << std::endl;
    for (auto& coord : SGPResponse.geodetic()) {
      uint64_t time = coord.encoded_time();
      // std::cout << "acc " << time;
      DateTime date(time);
      int year = 0, month = 0, day = 0;
      date.FromTicks(year, month, day);
      std::string str_date;
      str_date = std::to_string(year) + ","
         + std::to_string(month) + ","
         + std::to_string(day);

      igrf_request IGRFrequest;
      IGRFrequest.date = reinterpret_cast<char*>(malloc(str_date.length()+1));
      snprintf(IGRFrequest.date, str_date.length()+1, "%s",  str_date.c_str());
      IGRFrequest.coord_type = 'D';
      IGRFrequest.altitude_type = 'K';
      IGRFrequest.altitude = coord.alt();
      IGRFrequest.latitude = coord.lat();
      IGRFrequest.longitude = coord.lon();
      igrf_computation IGRFcomputation;
      igrf_compute(&IGRFrequest, &IGRFcomputation);
      igrf_computation_result computation_result = IGRFcomputation.result;
      igrf_computation_secular_variance variance = IGRFcomputation.variance;

      if (TLErequest->add_noise_to_igrf()) {
        GaussianNoise<PseudoNoiseMixin> noise(0, 50);
        if (IGRFcomputation.result.has_x) {
          computation_result.x += noise.Apply();
        }
        computation_result.y += noise.Apply();
        computation_result.z += noise.Apply();
        if (IGRFcomputation.result.has_declination) {
          computation_result.declination += noise.Apply();
        }
        // computation_result.sdate += noise.Apply();
        computation_result.inclination += noise.Apply();
      }

      auto point_result = TLEresponse->mutable_results();

      auto tmp_result = point_result->add_result();  // tmp_result is a pointer
      tmp_result->set_sdate(computation_result.sdate);
      if (IGRFcomputation.result.has_declination) {
        tmp_result->set_declination(computation_result.declination);
      }
      tmp_result->set_inclination(computation_result.inclination);
      if (IGRFcomputation.result.has_x) {
        tmp_result->set_x(computation_result.x);
      }
      tmp_result->set_y(computation_result.y);
      tmp_result->set_z(computation_result.z);

      tmp_result->set_horizontal_intensity(
          computation_result.horizontal_intensity);
      tmp_result->set_total_intensity(computation_result.total_intensity);

      auto tmp_variance = point_result->add_variance();
      if (IGRFcomputation.variance.has_declination) {
        tmp_variance->set_declination_dot(variance.declination_dot);
      }
      tmp_variance->set_inclination_dot(variance.inclination_dot);
      if (IGRFcomputation.variance.has_x) {
        tmp_variance->set_x_dot(variance.x_dot);
      }
      tmp_variance->set_y_dot(variance.y_dot);
      tmp_variance->set_z_dot(variance.z_dot);

      tmp_variance->set_horizontal_intensity_dot(
          variance.horizontal_intensity_dot);
      tmp_variance->set_total_intensity_dot(variance.total_intensity_dot);


      point_result->add_error_code(IGRFcomputation.error_code);
    }

    // response->set_coord_type(TLErequest->coord_type());
    // TLEresponse->set_status(SGP::Status::OK);
    // response->set_number_of_sets(request->number_of_sets());
    return Status::OK;
  }

  ///////////////
  Status endWork(ServerContext* context_,
          const EndRequest* EndReq,
          EndResponse* EndRes) {
    auto id = EndReq->computational_id();
    CloseRequest req;
    req.set_computational_id(id);
    ClientContext context;
    CloseResponse res;
    Status status = stub_->Close(&context, req, &res);
    if (!status.ok())
      std::cout << "IGRF" << status.error_code()
          << " " << status.error_message();
    return status;
  }
  ///////////////

 private:
  std::unique_ptr<SGPService::Stub> stub_;
};

void RunServer(std::string port, bool test = false) {
  std::string server_address("0.0.0.0:"+port);
  IGRFServiceImpl service{grpc::CreateChannel("0.0.0.0:9090",
                          grpc::InsecureChannelCredentials())};

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
  // Expect only arg: --db_path=path/to/route_guide_db.json.
  // std::string db = routeguide::GetDbFileContent(argc, argv);
  if (argc < 2) {
    std::cout << "Usage: ./sgp_server N [OPTIONS]\n N - port number\n"
              << "OPTIONS include --test, which implies using a pipe";
    return 1;
  }
  igrf_construct("./IGRF13.COF");
  RunServer(argv[1], argc == 3 && strcmp(argv[2], "--test") == 0
                      ? true
                      : false);

  return 0;
}
