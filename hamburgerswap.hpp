#include "common.hpp"

#ifdef MAINNET
  #define HAMBURGERSWAP_CONTRACT name("hamburgerswp")
#else
  #define HAMBURGERSWAP_CONTRACT name("hamburgerswp")
#endif

namespace hamburgerswap {
  struct pair {
    uint64_t id;
    symbol_code code;
    extended_symbol token0;
    extended_symbol token1;
    asset reserve0;
    asset reserve1;
    uint64_t total_liquidity;
    uint32_t last_update_time;
    uint32_t create_time;

    uint64_t primary_key() const {
      return id;
    }
  };

  typedef eosio::multi_index<name("pairs"), pair> pair_tlb;

  pair get_pair(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    pair_tlb pairs(HAMBURGERSWAP_CONTRACT, HAMBURGERSWAP_CONTRACT.value);
    if (args.size() > 0) {
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find hamburger swap pair with id: %d", id);
      return pairs.get(id, msg);
    }

    for (auto itr = pairs.begin(); itr != pairs.end(); itr++) {
      if (anchor.get_contract() == itr->token0.get_contract() && anchor.get_symbol() == itr->token0.get_symbol() && 
        coin.get_contract() == itr->token1.get_contract() && coin.get_symbol() == itr->token1.get_symbol()) {
        return *itr;
      }

      if (anchor.get_contract() == itr->token1.get_contract() && anchor.get_symbol() == itr->token1.get_symbol() && 
        coin.get_contract() == itr->token0.get_contract() && coin.get_symbol() == itr->token0.get_symbol()) {
        return *itr;
      }
    }
    check(false, "hamburger swap pair with this ticker not found");
    return pair();
  }

  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    auto p = get_pair(coin, anchor, args);
    asset coin_reserve;
    asset anchor_reserve;

    if (anchor.get_contract() == p.token0.get_contract() && anchor.get_symbol() == p.token0.get_symbol() && 
        coin.get_contract() == p.token1.get_contract() && coin.get_symbol() == p.token1.get_symbol()) {
      coin_reserve = p.reserve1;
      anchor_reserve = p.reserve0;
    } else if (anchor.get_contract() == p.token1.get_contract() && anchor.get_symbol() == p.token1.get_symbol() && 
        coin.get_contract() == p.token0.get_contract() && coin.get_symbol() == p.token0.get_symbol()){
      coin_reserve = p.reserve0;
      anchor_reserve = p.reserve1;
    } else {
      check(false, "hamburger swap pair not match");
    }
    
    int8_t precDiff = (int8_t)coin_reserve.symbol.precision()-anchor_reserve.symbol.precision();
    double price = (double)anchor_reserve.amount * pow(10, precDiff) / coin_reserve.amount;
    uint64_t weight = anchor_reserve.amount;
    return weight_price(price, weight);
  };
}