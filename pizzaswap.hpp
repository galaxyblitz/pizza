#include "common.hpp"

#ifdef MAINNET
  #define PIZZASWAP_CONTRACT name("pzaswapcntct")
#else
  #define PIZZASWAP_CONTRACT name("zheshizenmet")
#endif

namespace pizzaswap {
  struct pair {
    name pair;
    symbol minor_symbol;
    name minor_contract;
    symbol major_symbol;
    name major_contract;
    uint32_t status;
    uint32_t time;

    uint64_t primary_key() const { return pair.value; }
  };

  typedef eosio::multi_index<name("pair"), pair> pair_tlb;
  pair_tlb pairs(PIZZASWAP_CONTRACT, PIZZASWAP_CONTRACT.value);
  struct total {
    uint64_t id;
    asset total_minor;
    asset total_major;
    uint64_t total_mmf = 0;
    asset total_fee;
    asset history_mm_minor;
    asset history_mm_major;
    asset history_bank_minor;
    asset history_bank_major;
    asset history_fee;
    uint32_t time;

    uint64_t primary_key() const { return 0; }
  };

  typedef eosio::multi_index<name("total"), total> total_tlb;

  pair get_pair(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    if (args.size() > 0) {
      name pname = name(args[0]);
      pair p = pairs.get(pname.value, "cannot find pizza swap pair");
      return p;
    }

    for (auto itr = pairs.begin(); itr != pairs.end(); itr++) {
      if (anchor.get_contract() == itr->minor_contract && anchor.get_symbol() == itr->minor_symbol && 
        coin.get_contract() == itr->major_contract && coin.get_symbol() == itr->major_symbol ) {
        return *itr;
      }

      if (anchor.get_contract() == itr->major_contract && anchor.get_symbol() == itr->major_symbol && 
        coin.get_contract() == itr->minor_contract && coin.get_symbol() == itr->minor_symbol ) {
        return *itr;
      }
    }
    check(false, "pizza swap pair with this ticker not found");
    return pair();
  };

  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    pair p = get_pair(coin, anchor, args);

    total_tlb totals(PIZZASWAP_CONTRACT, p.pair.value);
    total t = totals.get(0);

    asset coin_reserve;
    asset anchor_reserve;
    if (anchor.get_contract() == p.minor_contract && anchor.get_symbol() == p.minor_symbol && 
        coin.get_contract() == p.major_contract && coin.get_symbol() == p.major_symbol) {
      anchor_reserve = t.total_minor;
      coin_reserve = t.total_major;
    } else if (anchor.get_contract() == p.major_contract && anchor.get_symbol() == p.major_symbol && 
        coin.get_contract() == p.minor_contract && coin.get_symbol() == p.minor_symbol) {
      anchor_reserve = t.total_major;
      coin_reserve = t.total_minor;
    } else {
      check(false, "pizza swap pair not match");
    }
    
    int8_t precDiff = (int8_t)coin_reserve.symbol.precision()-anchor_reserve.symbol.precision();
    double price = (double)anchor_reserve.amount * pow(10, precDiff) / coin_reserve.amount;
    uint64_t weight = anchor_reserve.amount;
    return weight_price(price, weight);
  };
}