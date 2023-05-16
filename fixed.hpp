#include "common.hpp"

namespace fixed {
  weight_price fetch(extended_symbol coin, std::vector<std::string> args) {
    check(args.size() > 0, "empty args");
    int64_t amount = std::atoi(args[0].c_str());
    decimal price = asset(amount, FLOAT);
    return weight_price(price, 0);
  }
}