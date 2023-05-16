#pragma once
#include "common.hpp"

#ifdef MAINNET
  #define ROMESWAP_CONTRACT name("swap.rome")
#else
  #define ROMESWAP_CONTRACT name("testromeswap")
#endif

namespace romeswap {
  struct st_coin {
    name contract;
    symbol symbol;
  };

  struct market {
    uint64_t market_id;
    st_coin coin0;
    st_coin coin1;
    asset reserve0;
    asset reserve1;
    uint64_t token;
    double price0_last;
    double price1_last;
    time_point_sec last_update;

    uint64_t primary_key() const {
      return market_id;
    }
  };
  typedef eosio::multi_index<name("markets"), market> market_tlb;
  market_tlb markets(ROMESWAP_CONTRACT, ROMESWAP_CONTRACT.value);

  market get_market(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    if (args.size() > 0) {
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find rome swap market with id: %d", id);
      return markets.get(id, msg);
    }

    for (auto itr = markets.begin(); itr != markets.end(); itr++) {
      if (anchor.get_contract() == itr->coin0.contract && anchor.get_symbol() == itr->coin0.symbol && 
        coin.get_contract() == itr->coin1.contract && coin.get_symbol() == itr->coin1.symbol) {
        return *itr;
      }
      if (anchor.get_contract() == itr->coin1.contract && anchor.get_symbol() == itr->coin1.symbol && 
        coin.get_contract() == itr->coin0.contract && coin.get_symbol() == itr->coin0.symbol) {
        return *itr;
      }
    }
    check(false, "rome swap market with this ticker not found");
    return market();
  };

  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    auto m = get_market(coin, anchor, args);
    asset coin_reserve;
    asset anchor_reserve;
    if (anchor.get_contract() == m.coin0.contract && anchor.get_symbol() == m.coin0.symbol && 
      coin.get_contract() == m.coin1.contract && coin.get_symbol() == m.coin1.symbol) {
      anchor_reserve = m.reserve0;
      coin_reserve = m.reserve1;
    } else if (anchor.get_contract() == m.coin1.contract && anchor.get_symbol() == m.coin1.symbol && 
      coin.get_contract() == m.coin0.contract && coin.get_symbol() == m.coin0.symbol) {
      anchor_reserve = m.reserve1;
      coin_reserve = m.reserve0;
    } else {
      check(false, "rome swap market not match");
    }

    int8_t precDiff = (int8_t)coin_reserve.symbol.precision()-anchor_reserve.symbol.precision();
    double price = (double)anchor_reserve.amount * pow(10, precDiff) / coin_reserve.amount;
    uint64_t weight = anchor_reserve.amount;
    return weight_price(price, weight);
  }
}