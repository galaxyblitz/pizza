#include "common.hpp"

#ifdef MAINNET
  #define OGX_CONTRACT name("core.ogx")
#endif

namespace ogx {
  #ifdef MAINNET
  struct current_round_rate {
    symbol sym;
    uint64_t round;
    uint64_t timestamp;
    uint128_t rate;

    uint64_t primary_key() const {
      return sym.code().raw();
    }
  };
  typedef eosio::multi_index<name("currrundrate"), current_round_rate> currrundrate_tlb;
  currrundrate_tlb currrundrates(OGX_CONTRACT, OGX_CONTRACT.value);
  #endif

  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    #ifndef MAINNET
    check(false, "ogx only support mainnet");
    return weight_price(0, 0);
    #else
    check(anchor.get_contract() == OGX_CONTRACT && 
      anchor.get_symbol() == symbol("OUSD", 8), 
      "anchor symbol of ogx source must be OUSD");
    
    check(coin.get_contract() == OGX_CONTRACT, "only supply ogx token");
    current_round_rate crr = currrundrates.get(coin.get_symbol().code().raw(), "cannot find current round rate of this coin");
    double price = (double)crr.rate / pow(10, 8);
    return weight_price(price, 0);
    #endif
  }
}