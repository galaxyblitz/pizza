#include "common.hpp"
#include <string.h>

#ifdef MAINNET
  #define DOLPHINSWAP_CONTRACT name("dolphinsswap")
#else
  #define DOLPHINSWAP_CONTRACT name("dolphinswap1")
#endif

namespace dolphinswap {
  struct extend_symbol {
    symbol symbol;
    name contract;
  };

  struct pooltoken {
    uint32_t weight;
    extend_symbol symbol;
    asset reserve;
  };

  struct pool {
    uint64_t id;
    symbol_code code;
    uint16_t swap_fee;
    uint64_t total_lptoken;
    uint32_t create_time;
    uint32_t last_update_time;
    std::vector<pooltoken> tokens;

    uint64_t primary_key() const {
      return id;
    }
  };

  typedef eosio::multi_index<name("pools"), pool> pool_tlb;

  struct pooltokens {
    pooltoken coin_token;
    pooltoken anchor_token;
  };

  pooltokens get_tokens(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    pool_tlb pools(DOLPHINSWAP_CONTRACT, DOLPHINSWAP_CONTRACT.value);
    if (args.size() > 0) {
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find dolphin swap pool with id: %d", id);
      pool p = pools.get(id, msg);
      if (p.tokens[0].symbol.contract == anchor.get_contract() && p.tokens[0].symbol.symbol == anchor.get_symbol() && 
        p.tokens[1].symbol.contract == coin.get_contract() && p.tokens[1].symbol.symbol == coin.get_symbol()) {
        return pooltokens{p.tokens[1], p.tokens[0]};
      } else if (p.tokens[1].symbol.contract == anchor.get_contract() && p.tokens[1].symbol.symbol == anchor.get_symbol() && 
        p.tokens[0].symbol.contract == coin.get_contract() && p.tokens[0].symbol.symbol == coin.get_symbol()) {
        return pooltokens{p.tokens[0], p.tokens[1]};
      } else {
        check(false, "dolphin swap pool not match");
      }
    }

    pooltoken coin_token;
    pooltoken anchor_token;

    for (auto itr = pools.begin(); itr != pools.end(); itr++) {
      if (anchor.get_contract() == itr->tokens[1].symbol.contract && 
        anchor.get_symbol() == itr->tokens[1].symbol.symbol && 
        coin.get_contract() == itr->tokens[0].symbol.contract && 
        coin.get_symbol() == itr->tokens[0].symbol.symbol) {
        if (itr->tokens[1].reserve.amount > anchor_token.reserve.amount) {
          coin_token = itr->tokens[0];
          anchor_token = itr->tokens[1];
        }
      }

      if (anchor.get_contract() == itr->tokens[0].symbol.contract && 
        anchor.get_symbol() == itr->tokens[0].symbol.symbol && 
        coin.get_contract() == itr->tokens[1].symbol.contract && 
        coin.get_symbol() == itr->tokens[1].symbol.symbol) {
        if (itr->tokens[0].reserve.amount > anchor_token.reserve.amount) {
          coin_token = itr->tokens[1];
          anchor_token = itr->tokens[0];
        }
      }
    }
    return pooltokens{coin_token, anchor_token};
  }

  /**
   * parameters:
   *  - id: the id field in the pool table
   * */
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    pooltokens tokens = get_tokens(coin, anchor, args);
    check(anchor.get_contract() == tokens.anchor_token.symbol.contract && 
        anchor.get_symbol() == tokens.anchor_token.symbol.symbol && 
        coin.get_contract() == tokens.coin_token.symbol.contract && 
        coin.get_symbol() == tokens.coin_token.symbol.symbol, 
        "dolphin swap pool not match"
      );

    uint32_t coin_weight = tokens.coin_token.weight;
    asset coin_reserve = tokens.coin_token.reserve;
    uint32_t anchor_weight = tokens.anchor_token.weight;
    asset anchor_reserve = tokens.anchor_token.reserve;

    int8_t precDiff = (int8_t)coin_reserve.symbol.precision()-anchor_reserve.symbol.precision();
    double price = (double)anchor_reserve.amount * coin_weight * pow(10, precDiff) / (coin_reserve.amount * anchor_weight);
    uint64_t weight = anchor_reserve.amount;
    return weight_price(price, weight);
  };
}