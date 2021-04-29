#pragma once
#ifdef __cplusplus
extern "C"{
#endif 
struct igrf_computation_result{
    double sdate;
    double declination;
    double inclination;
    double horizontal_intensity; //corresponds to "h"
    double x;
    double y;
    double z;
    double total_intensity;  //corresponds to "f"

    char has_x;
    char has_declination;
};

struct igrf_computation_secular_variance{
    double sdate;
    double declination_dot;
    double inclination_dot;
    double horizontal_intensity_dot; //corresponds to "hdot"
    double x_dot;
    double y_dot;
    double z_dot;
    double total_intensity_dot; //corresponds to "fdot"

    char has_x;
    char has_declination;
};

typedef struct igrf_computation_result igrf_computation_result;
typedef struct igrf_computation_secular_variance igrf_computation_secular_variance;

struct igrf_computation{
    igrf_computation_result result;
    igrf_computation_secular_variance variance;
    int error_code;
};

typedef struct igrf_computation igrf_computation;

struct igrf_request{
    //const char* model_file_path;
    char* date;
    char coord_type;
    char altitude_type;
    double altitude;
    double latitude;
    double longitude;
};
typedef struct igrf_request igrf_request;

int igrf_construct(const char* model_file_path);

int igrf_compute(igrf_request* request, igrf_computation* response);
#ifdef __cplusplus
}
#endif