#include "common.hpp"

#ifdef MAINNET 
  #define ONESSWAP_CONTRACT name("onesgamedefi")
#else
  #define ONESSWAP_CONTRACT name("testonesswap")
#endif
namespace onesswap {
  struct token {
    name address;
    symbol symbol;
  };

  struct liquidity {
    uint64_t liquidity_id;
    token token1;
    token token2;
    asset quantity1;
    asset quantity2;
    uint64_t liquidity_token;
    float price1;
    float price2;
    uint64_t cumulative1;
    uint64_t cumulative2;
    float swap_weight;
    float liquidity_weight;
    uint64_t timestamp;

    uint64_t primary_key() const {
      return liquidity_id;
    }
  };

  typedef eosio::multi_index<name("liquidity"), liquidity> liquidity_tlb;

  liquidity get_liquidity(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    liquidity_tlb liquidies(ONESSWAP_CONTRACT, ONESSWAP_CONTRACT.value);
    if (args.size() > 0) {
      uint64_t id = std::atoi(args[0].c_str());
      char* msg;
      sprintf(msg, "cannot find ones swap liquidity with id: %d", id);
      return liquidies.get(id, msg);
    }

    for (auto itr = liquidies.begin(); itr != liquidies.end(); itr++) {
      if (anchor.get_contract() == itr->token1.address && anchor.get_symbol() == itr->token1.symbol && 
        coin.get_contract() == itr->token2.address && coin.get_symbol() == itr->token2.symbol) {
        return *itr;
      }
      if (anchor.get_contract() == itr->token2.address && anchor.get_symbol() == itr->token2.symbol && 
        coin.get_contract() == itr->token1.address && coin.get_symbol() == itr->token1.symbol) {
        return *itr;
      }
      
    }
    check(false, "ones swap liquidity with this ticker not found");
    return liquidity();
  };

  /**
   * parameters:
   *  - id: the liquidity_id field in the liquidity table
   * */
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    auto l = get_liquidity(coin, anchor, args);
    double price;
    uint64_t weight;
    if (anchor.get_contract() == l.token1.address && anchor.get_symbol() == l.token1.symbol && 
      coin.get_contract() == l.token2.address && coin.get_symbol() == l.token2.symbol) {
      price = l.price2;
      weight = l.quantity1.amount;
    } else if (anchor.get_contract() == l.token2.address && anchor.get_symbol() == l.token2.symbol && 
      coin.get_contract() == l.token1.address && coin.get_symbol() == l.token1.symbol) {
      price = l.price1;
      weight = l.quantity2.amount;
    } else {
      check(false, "ones swap liquidity not match");
    }
    return weight_price(price, weight);
  };
}