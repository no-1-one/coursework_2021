#include "noise_application.h"

TrueNoiseMixin::TrueNoiseMixin(): device(){}
std::random_device& TrueNoiseMixin::get(){
  return device;
}

PseudoNoiseMixin::PseudoNoiseMixin(): device(std::random_device()()){}

PseudoNoiseMixin::PseudoNoiseMixin(uint32_t seed): device(seed){}

std::mt19937_64& PseudoNoiseMixin::get(){
  return device;
}

