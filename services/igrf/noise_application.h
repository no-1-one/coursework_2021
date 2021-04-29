#pragma once
#include <random>


template<class Generator>
struct NoiseMixin{
  virtual Generator& get() = 0;
};



class TrueNoiseMixin: public NoiseMixin<std::random_device>{
public:
  TrueNoiseMixin();
  virtual ~TrueNoiseMixin() = default;
  std::random_device& get() override;
  //virtual double Apply(double error_margin) const = 0;
private:
  std::random_device device;
};

class PseudoNoiseMixin: public NoiseMixin<std::mt19937_64>{
public:
  PseudoNoiseMixin();
  PseudoNoiseMixin(uint32_t seed);
  virtual ~PseudoNoiseMixin() = default;
  std::mt19937_64& get() override;
  //virtual double Apply(double error_margin) const = 0;
private:
  std::mt19937_64 device;
};

template<class TypeMixin>
struct Noise{
  template<class ...Args>
  Noise(Args ...args): mixin(args...){
    static_assert(
      std::is_base_of<NoiseMixin<std::random_device>, TypeMixin>::value || std::is_base_of<NoiseMixin<std::mt19937_64>, TypeMixin>::value, 
      "TypeMixin must derive from NoiseMixin");
  }
  virtual ~Noise() = default;
  virtual double Apply() = 0;
  TypeMixin mixin;
};

template<class TypeMixin>
class GaussianNoise: public Noise<TypeMixin>{
public:
  template<class ...Args>
  GaussianNoise(double mean, double standard_deviation, Args ...args)
  : Noise<TypeMixin>(args...), dist(mean, standard_deviation){

  }




  double Apply() override{
    
    return dist(Noise<TypeMixin>::mixin.get());
  }
private:
  std::normal_distribution<> dist;
};