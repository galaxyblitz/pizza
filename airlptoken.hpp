#include "common.hpp"
#include "pizzair.hpp"

namespace airlptoken {
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    check(coin.get_contract() == LPTOKEN_CONTRACT, "only supply air lptoken");

    double total_value = 0;
    pizzair::market m = pizzair::get_market(coin.get_symbol().code());
    if (anchor == m.syms[0]) {
      total_value = asset2double(m.anchor_reserve(0)) + asset2double(m.anchor_reserve(1)) * m.prices[1]; 
    } else if (anchor == m.syms[1]) {
      total_value = asset2double(m.anchor_reserve(0)) * m.prices[0] + asset2double(m.anchor_reserve(1));
    } else {
      check(false, "anchor must be one of liqdt symbol");
    }
    
    asset lpquantity = asset(m.lpamount, m.lptoken);
    double price = lpquantity.amount > 0 ? total_value / asset2double(lpquantity) : 0;
    return weight_price(price, 0);
  }
}