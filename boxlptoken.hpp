#include "common.hpp"
#include <string.h>

#include "boxswap.hpp"

namespace boxlptoken {
  uint64_t get_swap_id(extended_symbol coin) {
    std::string coin_name = coin.get_symbol().code().to_string();
    check(coin_name.find("BOX") == 0, "coin is not lptoken");
    const char* cs = coin_name.substr(3).c_str();
    int l = std::strlen(cs);
    check(l > 0, "invalid lptoken coin");
    uint64_t id;
    for (int i = l-1; i >= 0; i--) {
      int s = cs[i] - 'A' + 1;
      id += s * pow(26, l-i-1);
    }
    return id;
  };

  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    uint64_t id = get_swap_id(coin);
    boxswap::pair p = boxswap::get_pair(id);

    asset reserve;
    if (p.reserve0.symbol == anchor.get_symbol()) {
      reserve = p.reserve0;
    } else if (p.reserve1.symbol == anchor.get_symbol()) {
      reserve = p.reserve1;
    } else {
      check(false, "lptoken with box swap pair not match");
    }
    
    double price = (double)reserve.amount * 2 / pow(10, reserve.symbol.precision()) / p.liquidity_token;
    return weight_price(price, 0);
  }
}