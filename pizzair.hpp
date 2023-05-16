#pragma once

#include "common.hpp"

#include "pizzalend.hpp"

#define PSYM_LEN 3
#define LEVERAGE_DECIMALS 4

#ifdef MAINNET
  #define LPTOKEN_CONTRACT name("lptoken.air")
  #define AIR_CONTRACT name("air.pizza")
#else 
  #define LPTOKEN_CONTRACT name("lptokencode1")
  #define AIR_CONTRACT name("aircontract3")
#endif

namespace pizzair {
  double p_to_q(double p, double A, double x, double y) {
    double n = 4*(4*A - 1)*x*y;
    double m = -16*A*x*y*(x+y);
    double t = sqrt(pow(m/2, 2)+pow(n/3, 3));
    double D = cbrt(-m/2 + t) + cbrt(-m/2 - t);
    double G = 4*A*(x+y+p-D) + D;
    double c = 4*y*G - pow(D, 3)/(x+p);
    double a = 16*A;
    double b = -(4*G + 16*A*y);
    double delta = pow(b, 2) - 4*a*c;
    double q = (-b - sqrt(delta)) / (2*a);
    return q;
  };

  double cal_price(double A, double x, double y) {
    double p = std::min(x, y) * pow(10, -6);
    return p_to_q(p, A, x, y) / p;
  };

  struct market_config {
    uint32_t leverage;
    decimal fee_rate;
  };

  struct [[eosio::table]] market {
    symbol lptoken;
    std::vector<extended_symbol> syms;
    std::vector<asset> reserves;
    std::vector<double> prices;
    std::vector<uint8_t> lendables;
    uint64_t lpamount;
    market_config config;

    uint64_t primary_key() const {
      return lptoken.code().raw();
    }

    symbol_code psym() const {
      return symbol_code(lptoken.code().to_string().substr(0, PSYM_LEN));
    };

    uint64_t by_psym() const {
      return psym().raw();
    }

    asset anchor_reserve(int i) const {
      if (!lendables[i]) {
        return reserves[i];
      }

      pizzalend::pztoken pz = pizzalend::get_pztoken_byanchor(syms[i]);
      double pzprice = pz.cal_pzprice();
      return pz.cal_anchor_quantity(reserves[i], pzprice);
    };
  };

  typedef eosio::multi_index<
    name("market"), market,
    indexed_by<name("bypsym"), const_mem_fun<market, uint64_t, &market::by_psym>>
  > market_tlb;
  market_tlb markets(AIR_CONTRACT, AIR_CONTRACT.value);

  market get_market(symbol_code lpsymbol) {
    return markets.get(lpsymbol.raw(), "air market not found");
  };

  struct [[eosio::table]] market_leverage {
    symbol lptoken;
    uint32_t leverage;
    uint32_t begined_at;
    uint32_t effective_secs;

    uint64_t primary_key() const {
      return lptoken.code().raw();
    };
  };

  typedef eosio::multi_index<name("mleverage"), market_leverage> mleverage_tlb;
  mleverage_tlb mleverages(AIR_CONTRACT, AIR_CONTRACT.value);

  uint32_t _get_leverage(market m) {
    int leverage_precision = pow(10, LEVERAGE_DECIMALS);

    auto litr = mleverages.find(m.lptoken.code().raw());
    if (litr == mleverages.end()) return m.config.leverage;

    uint32_t passed_secs = now() - litr->begined_at;
    if (passed_secs >= litr->effective_secs) {
      return litr->leverage;
    }

    int32_t a1 = m.config.leverage;
    int32_t a2 = litr->leverage;
    int32_t diff = a2 - a1;
    double t = passed_secs;
    double T = litr->effective_secs;
    double rate = t/T;

    int32_t A = a1 + diff*rate;
    return A;
  }

  double _get_exact_leverage(market m) {
    uint32_t A = _get_leverage(m);
    return (double) A / pow(10, LEVERAGE_DECIMALS);
  }

  market get_market(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    if (args.size() > 0) {
      symbol_code lpsymbol = symbol_code(args[0]);
      return get_market(lpsymbol);
    }

    for (auto itr = markets.begin(); itr != markets.end(); itr++) {
      if (anchor == itr->syms[0] && coin == itr->syms[1]) {
        return *itr;
      }
      if (coin == itr->syms[0] && anchor == itr->syms[1]) {
        return *itr;
      }
    }
    check(false, "air swap market with this ticker not found");
    return market();
  }

  /**
   * parameters
   *  - lpsymbol: the lpsymbol field in the markets table
   * */
  weight_price fetch(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    market m = get_market(coin, anchor, args);
    double A = _get_exact_leverage(m);
    print_f("current A: % | ", A);
    double x, y;
    uint64_t weight;
    asset rs0 = m.anchor_reserve(0);
    asset rs1 = m.anchor_reserve(1);
    if (coin == m.syms[0] && anchor == m.syms[1]) {
      x = asset2double(rs0);
      y = asset2double(rs1);
      weight = rs1.amount;
    } else if (coin == m.syms[1] && anchor == m.syms[0]) {
      x = asset2double(rs1);
      y = asset2double(rs0);
      weight = rs0.amount;
    } else {
      check(false, "air swap market not match");
    }

    double price = cal_price(A, x, y);
    print_f("price: %", price);
    return weight_price(price, weight);
  }
}