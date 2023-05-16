#include "common.hpp"

#ifdef MAINNET
  #define PTOSWAP_CONTRACT name("ptoswapaccts")
#else
  #define PTOSWAP_CONTRACT name("testptoswap1")
#endif

namespace ptoswap {
  struct market {
    uint64_t mid;
    name contract0;
    name contract1;
    symbol sym0;
    symbol sym1;
    asset reserve0;
    asset reserve1;
    asset balance0;
    asset balance1;
    uint64_t liquidity_token;
    double price0_last;
    double price1_last;
    uint64_t price0_cumulative_last;
    uint64_t price1_cumulative_last;
    time_point_sec last_update;
    name creator;

    uint64_t primary_key() const {
      return mid;
    }
  };

  typedef eosio::multi_index<name("markets"), market> market_tlb;

  market get_market(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    market_tlb markets(PTOSWAP_CONTRACT, PTOSWAP_CONTRACT.value);
    if (args.size() > 0) {
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find pto swap market with id: %d", id);
      return markets.get(id, msg);
    }

    for (auto itr = markets.begin(); itr != markets.end(); itr++) {
      if (anchor.get_contract() == itr->contract0 && anchor.get_symbol() == itr->sym0 && 
        coin.get_contract() == itr->contract1 && coin.get_symbol() == itr->sym1 ) {
        return *itr;
      }

      if (anchor.get_contract() == itr->contract1 && anchor.get_symbol() == itr->sym1 && 
        coin.get_contract() == itr->contract0 && coin.get_symbol() == itr->sym0 ) {
        return *itr;
      }
    }
    check(false, "pto swap market with this ticker not found");
    return market();
  }

  /**
   * parameters:
   *  - id: the id field in the markets table
   * */
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    auto m = get_market(coin, anchor, args);

    asset coin_reserve;
    asset anchor_reserve;
    if (anchor.get_contract() == m.contract0 && anchor.get_symbol() == m.sym0 && 
      coin.get_contract() == m.contract1 && coin.get_symbol() == m.sym1) {
      anchor_reserve = m.reserve0;
      coin_reserve = m.reserve1;
    } else if (anchor.get_contract() == m.contract1 && anchor.get_symbol() == m.sym1 && 
      coin.get_contract() == m.contract0 && coin.get_symbol() == m.sym0) {
      anchor_reserve = m.reserve1;
      coin_reserve = m.reserve0;
    } else {
      check(false, "pto swap market not match");
    }
    
    int8_t precDiff = (int8_t)coin_reserve.symbol.precision()-anchor_reserve.symbol.precision();
    double price = (double)anchor_reserve.amount * pow(10, precDiff) / coin_reserve.amount;
    uint64_t weight = anchor_reserve.amount;
    return weight_price(price, weight);
  }
}