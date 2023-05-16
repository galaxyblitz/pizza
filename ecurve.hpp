#pragma once
#include "common.hpp"

#ifdef MAINNET
  #define ECURVE_POOL name("ecurve3pool1")
#endif

namespace ecurve {
  struct priceinfo {
    uint64_t id;
    float price;
    int64_t D;

    uint64_t primary_key() const {
      return id;
    }
  };
  typedef eosio::multi_index<name("priceinfo1"), priceinfo> priceinfo_tlb;
  #ifdef MAINNET
  priceinfo_tlb priceinfos(ECURVE_POOL, ECURVE_POOL.value);
  #endif

  /**
   * parameters:
   *  - id: the id field in the priceinfo1 table
   * */
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    check(anchor.get_contract() == name("tethertether") && anchor.get_symbol() == symbol("USDT", 4), "anchor symbol of ecurve source must be USDT");

    #ifdef MAINNET
      check(args.size() == 1, "the number of ecurve source args must be 1");
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find ecurve price info with id: %d", id);

      auto info = priceinfos.get(id, msg);
      return weight_price(info.price, 0);
    #endif

    return weight_price(1, 0);
  };
}