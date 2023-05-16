#include "common.hpp"

#ifdef MAINNET
  #define NEWDEX_CONTRACT name("newdexpublic")
#else
  #define NEWDEX_CONTRACT name("testnewdex11")
#endif

namespace newdex {
  struct ndx_symbol {
    name contract;
    symbol sym;
  };

  struct exchange_pair {
    uint64_t pair_id;
    uint8_t price_precision;
    uint8_t status;
    ndx_symbol base_symbol;
    ndx_symbol quote_symbol;
    name manager;
    time_point_sec list_time;
    std::string pair_symbol;
    double current_price;
    uint64_t base_currency_id;
    uint64_t quote_currency_id;
    uint8_t pair_fee;
    uint64_t ext1;
    uint64_t ext2;
    std::string extstr;

    uint64_t primary_key() const {
      return pair_id;
    }
  };

  typedef eosio::multi_index<name("exchangepair"), exchange_pair> pair_tlb;

  struct order {
    uint64_t order_id;
    uint64_t pair_id;
    uint8_t type;
    name owner;
    time_point_sec placed_time;
    asset remain_quantity;
    asset remain_convert;
    double price;
    name contract;
    uint8_t count;
    uint8_t crosschain;
    uint64_t ext1;
    std::string extstr;

    uint64_t primary_key() const {
      return order_id;
    }
  };

  typedef eosio::multi_index<name("buyorder"), order> buyorder_tlb;

  exchange_pair get_pair(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    pair_tlb pairs(NEWDEX_CONTRACT, NEWDEX_CONTRACT.value);
    if (args.size() > 0) {
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find newdex pair with id: %d", id);
      return pairs.get(id, msg);
    }

    for (auto itr = pairs.begin(); itr != pairs.end(); itr++) {
      if (anchor.get_contract() == itr->quote_symbol.contract && anchor.get_symbol() == itr->quote_symbol.sym && 
        coin.get_contract() == itr->base_symbol.contract && coin.get_symbol() == itr->base_symbol.sym) {
        return *itr;
      }
    }
    check(false, "newdex pair with this ticker not found");
    return exchange_pair();
  }

  /**
   * parameters:
   *  - id: the pair_id field in the exchangepair table
   * */
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    exchange_pair p = get_pair(coin, anchor, args);
    check(anchor.get_contract() == p.quote_symbol.contract && anchor.get_symbol() == p.quote_symbol.sym && 
        coin.get_contract() == p.base_symbol.contract && coin.get_symbol() == p.base_symbol.sym, "newdex pair not match");

    buyorder_tlb orders(NEWDEX_CONTRACT, p.pair_id);
    check(orders.begin() != orders.end(), "empty newdex order");
    auto by_price = [&](const order& a, const order& b) {
        return a.price < b.price;
    };
    auto max_order = std::max_element(orders.begin(), orders.end(), by_price);
    double price = max_order->price;
    return weight_price(price, 0);
  }
}