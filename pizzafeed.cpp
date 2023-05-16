#include "pizzafeed.hpp"

namespace pizzafeed {
  void pizzafeed::feed(symbol_code coin, decimal price) {
    require_auth(permission_level{ACT_ACCOUNT, name("operator")});

    auto itr = oracles.find(coin.raw());
    if (itr == oracles.end()) {
      oracles.emplace(_self, [&](auto& row) {
        row.coin = coin;
        row.price = price;
        row.fetched_at = now();
      });
    } else {
      oracles.modify(itr, _self, [&](auto& row) {
        row.price = price;
        row.fetched_at = now();
      });
    }
  };

  void pizzafeed::set(extended_symbol coin, std::vector<source> sources, uint32_t interval, uint32_t reliable_interval) {
    require_auth(permission_level{ADMIN_ACCOUNT, name("manager")});

    check(interval == 0 || interval >= 10, "the interval must be zero or greater than 10 seconds");

    auto idx = fetchs.get_index<name("bycoin")>();
    auto itr = idx.find(raw(coin));
    
    if (itr == idx.end()) {
      auto nitr = fetchs.emplace(_self, [&](auto& row) {
        row.id = fetchs.available_primary_key();
        row.coin = coin;
        row.sources = sources;
        row.interval = interval;
        row.reliable_interval = reliable_interval;
        row.reliable_price = decimal(0, FLOAT);
        row.updatable = true;
        row.available = false;
      });
      update(*nitr);
    } else {
      idx.modify(itr, _self, [&](auto& row) {
        row.sources = sources;
        row.interval = interval;
        row.reliable_interval = reliable_interval;
      });
      if (itr->updatable) {
        update(*itr);
      }
    }
  };

  void pizzafeed::refresh() {
    require_auth(permission_level{ACT_ACCOUNT, name("operator")});

    uint32_t current = now();
    for (auto itr = fetchs.begin(); itr != fetchs.end(); itr++) {
      if (!itr->updatable) continue;
      if (itr->fetched_at - current < 0.8*itr->interval) continue;
      update(*itr);
    }
  };

  void pizzafeed::refresh2(uint32_t offset, uint32_t limit) {
    require_auth(permission_level{ACT_ACCOUNT, name("operator")});

    uint32_t current = now();
    uint32_t count = 0;
    for (auto itr = fetchs.begin(); itr != fetchs.end(); itr++) {
      count++;
      if (count <= offset) continue;
      if (count > offset + limit) break;

      if (!itr->updatable) continue;
      if (itr->fetched_at - current < 0.8*itr->interval) continue;
      update(*itr);
    }
  };

  void pizzafeed::setavailable(extended_symbol coin, bool available) {
    require_auth(permission_level{ADMIN_ACCOUNT, name("manager")});

    auto idx = fetchs.get_index<name("bycoin")>();
    auto itr = idx.find(raw(coin));
    check(itr != idx.end(), "fetch with this coin not found");
    check(itr->available != available, "no change");

    idx.modify(itr, _self, [&](auto& row) {
      row.available = available;
    });
  };

  #ifndef MAINNET
  void pizzafeed::clear() {
    require_auth(_self);

    auto oitr = oracles.begin();
    while (oitr != oracles.end()) {
      oitr = oracles.erase(oitr);
    }
    
    auto ritr = fetchs.begin();
    while (ritr != fetchs.end()) {
      ritr = fetchs.erase(ritr);
    }
  };
  #endif

  bool pizzafeed::update(fetch f) {
    auto itr = fetchs.find(f.id);
    check(itr != fetchs.end(), "fetch not found");

    decimal price = execute_fetch(f);

    decimal old_price = f.reliable_price;

    uint32_t current = now();
    uint32_t passed = current - itr->recorded_at;
    fetchs.modify(itr, _self, [&](auto& row) {
      row.fetched_at = current;
      if (f.reliable_interval == 0) {
        row.prices = {price};
        row.reliable_price = price;
        row.recorded_at = current;
      }  else if (passed > f.reliable_interval) {
        if (row.prices.size() == 0 || passed >= 2 * f.reliable_interval) {
          row.reliable_price = price;
        } else {
          row.reliable_price = get_median(row.prices);
        }
        row.recorded_at = current;
        row.prices = {price};
      } else if (passed == f.reliable_interval) {
        row.prices.push_back(price);
        row.reliable_price = get_median(row.prices);
        row.recorded_at = current;
        row.prices.clear();
      } else {
        row.prices.push_back(price);
      }
    });

    return old_price != itr->reliable_price;
  };

  decimal pizzafeed::get_oracle_price(std::vector<std::string> args) {
    check(args.size() > 0, "at least 1 parameter is required");
    symbol_code sym = symbol_code(args[0].c_str());
    auto o = oracles.get(sym.raw(), "oracle price not found");
    return o.price;
  };

  decimal pizzafeed::execute_fetch(fetch f) {
    double total = 0;
    uint64_t total_weight;
    extended_symbol anchor = AnchorEOS;
    for (auto itr = f.sources.begin(); itr != f.sources.end(); itr++) {
      weight_price wp = execute_source(f.coin, anchor, itr->type, itr->args);
      if (wp.weight == 0) {
        return wp.price;
      }
      
      total_weight += wp.weight;
      total += (decimal2double(wp.price) * wp.weight);
    }

    check(total > 0 && total_weight > 0, "invalid source price");

    return double2decimal(total/total_weight);
  };

  weight_price pizzafeed::execute_source(extended_symbol coin, extended_symbol anchor, uint8_t type, std::vector<std::string> args) {
    weight_price wp;
    switch (type) {
      case Oracle:
        wp = weight_price(get_oracle_price(args), 0);
        break;
      case DFSSwap:
        wp = dfsswap::fetch(coin, anchor, args);
        break;
      case BoxSwap:
       wp = boxswap::fetch(coin, anchor, args);
       break;
      case OnesSwap:
        wp = onesswap::fetch(coin, anchor, args);
        break;
      case DophinSwap:
        wp = dolphinswap::fetch(coin, anchor, args);
        break;
      case Newdex:
        wp = newdex::fetch(coin, anchor, args);
        break;
      case HamburgerSwap:
        wp = hamburgerswap::fetch(coin, anchor, args);
        break;
      case PtoSwap:
       wp = ptoswap::fetch(coin, anchor, args);
       break;
      case Fixed:
        wp = fixed::fetch(coin, args);
        break;
      case BoxLPToken:
       wp = boxlptoken::fetch(coin, anchor, args);
       break;
      case PizzaSwap:
        wp = pizzaswap::fetch(coin, anchor, args);
        break;
      case RomeSwap:
        wp = romeswap::fetch(coin, anchor, args);
        break;
      case EcurvePool:
        wp = ecurve::fetch(coin, anchor, args);
        break;
      case AirLPtoken:
        wp = airlptoken::fetch(coin, anchor, args);
        break;
      case AirSwap:
        wp = pizzair::fetch(coin, anchor, args);
        break;
      case OGX:
        wp = ogx::fetch(coin, anchor, args);
        break;
      case Multipath:
        wp = fetch_multipath(coin, anchor, args);
        break;
      default:
        check(false, "unsupport source");
        break;
    }
    return wp;
  };

  // multipath price feed
  // parameters:
  //    arg0 - intermediate currency fetch id
  //    arg1 - coin to intermediate currency price feed source type
  //    [arg2...] - coin to intermediate currency price feed parameters
  weight_price pizzafeed::fetch_multipath(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args) {
    check(args.size() >= 2, "the number of multipath source args must be greater than 1");
    uint64_t proxy_id = std::atoi(args[0].c_str());
    auto proxy_fetch = fetchs.get(proxy_id, "proxy coin not found");
    decimal proxy_price = execute_fetch(proxy_fetch);
    uint8_t proxy_type = std::atoi(args[1].c_str());
    std::vector<std::string> proxy_args(args.begin() + 2, args.end());
    weight_price coin_to_proxy = execute_source(coin, proxy_fetch.coin, proxy_type, proxy_args);
    decimal price = decimal(proxy_price.amount * decimal2double(coin_to_proxy.price), FLOAT);
    uint64_t weight = (double)coin_to_proxy.weight * pow(10, (int)anchor.get_symbol().precision() - (int)proxy_fetch.coin.get_symbol().precision()) * decimal2double(proxy_price);
    weight_price wp = weight_price(price, weight);
    return wp;
  };
}