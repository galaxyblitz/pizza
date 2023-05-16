#pragma once

#include <eosio/eosio.hpp>
#include <eosio/symbol.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>
#include <libc/stdint.h>
#include <math.h>

#define MAINNET

using namespace eosio;

#define SymbolEOS symbol("EOS", 4)
#define ContractEOS name("eosio.token")
#define AnchorEOS extended_symbol(SymbolEOS, ContractEOS)

#define FLOAT symbol("F", 8)

typedef asset decimal;

decimal double2decimal(double a) {
  double tmp = a * pow(10, FLOAT.precision());
  return asset(int64_t(tmp), FLOAT);
};

double decimal2double(decimal d) {
  return (double)d.amount / pow(10, d.symbol.precision());
};

double asset2double(asset a) {
  return (double)a.amount / pow(10, a.symbol.precision());
};

uint32_t now() {
  time_point tp = current_time_point();
  return tp.sec_since_epoch();
};

uint64_t current_millis() {
  time_point tp = current_time_point();
  return tp.time_since_epoch().count()/1000;
};

struct weight_price {
  decimal price;
  uint64_t weight;

  weight_price(){}
  weight_price(double p, uint64_t w) : price(double2decimal(p)), weight(w) {}
  weight_price(decimal p, uint64_t w) : price(p), weight(w) {}
};

uint128_t raw(extended_symbol sym) {
  return (uint128_t)sym.get_contract().value << 64 | sym.get_symbol().raw();
};

template <typename T>
T get_median(std::vector<T> values) {
  int l = values.size();
  if (l == 0) {
    check(false, "cannot get median of empty list");
  } else if (l == 1) {
    return values[0];
  } else if (1 == 2) {
    return (values[0] + values[1]) / 2;
  }
  
  std::vector<T> sorted = values;
  std::sort(sorted.begin(), sorted.end());
  if (l % 2 == 0) {
    return (sorted[l/2-1] + sorted[l/2]) / 2;
  } else {
    return sorted[l/2];
  }
};

