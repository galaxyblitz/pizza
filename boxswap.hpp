#pragma once
#include "common.hpp"

#ifdef MAINNET
  #define BOXSWAP_CONTRACT name("swap.defi")
#else
  #define BOXSWAP_CONTRACT name("testboxswap1")
#endif

namespace boxswap {
  struct token {
    name contract;
    symbol symbol;
  };

  struct reserves {
    asset coin_reserve;
    asset anchor_reserve;
  };

  struct pair {
    uint64_t id;
    token token0;
    token token1;
    asset reserve0;
    asset reserve1;
    uint64_t liquidity_token;
    double price0_last;
    double price1_last;
    uint64_t price0_cumulative_last;
    uint64_t price1_cumulative_last;
    time_point_sec block_time_last;

    uint64_t primary_key() const {
      return id;
    }
  };

  typedef eosio::multi_index<name("pairs"), pair> pair_tlb;

  pair get_pair(uint64_t id) {
    pair_tlb pairs(BOXSWAP_CONTRACT, BOXSWAP_CONTRACT.value);
    char* msg;
    sprintf(msg, "cannot find box swap pair with id: %d", id);
    return pairs.get(id, msg);
  };

  pair get_pair(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    pair_tlb pairs(BOXSWAP_CONTRACT, BOXSWAP_CONTRACT.value);
    if (args.size() > 0) {
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find box swap pair with id: %d", id);
      return pairs.get(id, msg);
    }

    for (auto itr = pairs.begin(); itr != pairs.end(); itr++) {
      if (anchor.get_contract() == itr->token1.contract && anchor.get_symbol() == itr->token1.symbol && 
        coin.get_contract() == itr->token0.contract && coin.get_symbol() == itr->token0.symbol ) {
        return *itr;
      }

      if (anchor.get_contract() == itr->token0.contract && anchor.get_symbol() == itr->token0.symbol && 
        coin.get_contract() == itr->token1.contract && coin.get_symbol() == itr->token1.symbol ) {
        return *itr;
      }
    }
    check(false, "box swap pair with this ticker not found");
    return pair();
  }

  /**
   * parameters:
   *  - id: the id field in the pairs table
   * */
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    pair p = get_pair(coin, anchor, args);
    asset coin_reserve;
    asset anchor_reserve;
    if (anchor.get_contract() == p.token0.contract && anchor.get_symbol() == p.token0.symbol && 
        coin.get_contract() == p.token1.contract && coin.get_symbol() == p.token1.symbol) {
      coin_reserve = p.reserve1;
      anchor_reserve = p.reserve0;
    } else if (anchor.get_contract() == p.token1.contract && anchor.get_symbol() == p.token1.symbol && 
        coin.get_contract() == p.token0.contract && coin.get_symbol() == p.token0.symbol) {
      coin_reserve = p.reserve0;
      anchor_reserve = p.reserve1;
    } else {
      check(false, "box swap pair not match");
    }
    
    int8_t precDiff = (int8_t)coin_reserve.symbol.precision()-anchor_reserve.symbol.precision();
    double price = (double)anchor_reserve.amount * pow(10, precDiff) / coin_reserve.amount;
    uint64_t weight = anchor_reserve.amount;
    return weight_price(price, weight);
  };
}