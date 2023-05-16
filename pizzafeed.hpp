#include "common.hpp"

#include "dfsswap.hpp"
#include "dolphinswap.hpp"
#include "hamburgerswap.hpp"
#include "newdex.hpp"
#include "onesswap.hpp"
#include "boxswap.hpp"
#include "boxlptoken.hpp"
#include "ptoswap.hpp"
#include "fixed.hpp"
#include "pizzaswap.hpp"
#include "romeswap.hpp"
#include "ecurve.hpp"
#include "airlptoken.hpp"
#include "pizzair.hpp"
#include "ogx.hpp"

#ifdef MAINNET
  #define ACT_ACCOUNT name("active.pizza")
  #define ADMIN_ACCOUNT name("admin.pizza")
#else
  #define ACT_ACCOUNT name("lendactacc11")
  #define ADMIN_ACCOUNT name("lendadmacc11")
#endif

namespace pizzafeed {
  struct source {
    uint8_t type;
    std::vector<std::string> args;
  };

  class [[eosio::contract]] pizzafeed : public contract {
  public:
    pizzafeed(name self, name first_receiver, datastream<const char*> ds): 
      contract(self, first_receiver, ds), fetchs(self, self.value), oracles(self, self.value) {}

    [[eosio::action]]
    void feed(symbol_code coin, decimal price);

    [[eosio::action]]
    void set(extended_symbol coin, std::vector<source> sources, uint32_t interval, uint32_t reliable_interval);

    [[eosio::action]]
    void refresh();

    [[eosio::action]]
    void refresh2(uint32_t offset, uint32_t limit);

    [[eosio::action]]
    void setavailable(extended_symbol coin, bool available);

    #ifndef MAINNET
    [[eosio::action]]
    void clear();
    #endif

  private:
    enum SourceType {
      Oracle = 1,
      DFSSwap = 2,
      BoxSwap = 3,
      OnesSwap = 4,
      DophinSwap = 5,
      Newdex = 6,
      HamburgerSwap = 7,
      PtoSwap = 8,
      Fixed = 9,
      BoxLPToken = 10,
      PizzaSwap = 11,
      RomeSwap = 12,
      EcurvePool = 13,
      AirLPtoken = 14,
      AirSwap = 15,
      OGX = 16,
      Multipath = 20
    };

    struct [[eosio::table]] fetch {
      uint64_t id;
      extended_symbol coin;
      std::vector<source> sources;
      uint32_t interval;
      uint32_t reliable_interval;
      bool updatable;
      bool available;
      std::vector<decimal> prices;
      decimal reliable_price;
      uint32_t fetched_at;
      uint32_t recorded_at;

      uint128_t by_coin() const {
        return raw(coin);
      };

      uint64_t primary_key() const {
        return id;
      }

      bool should_record(uint32_t current, uint32_t reliable_interval) {
        return (current - recorded_at) >= reliable_interval;
      }
    };

    typedef eosio::multi_index<
      name("fetch"), fetch,
      indexed_by<
        name("bycoin"),
        const_mem_fun<fetch, uint128_t, &fetch::by_coin>
      >
    > fetch_tlb;
    fetch_tlb fetchs;

    struct [[eosio::table]] oracle {
      symbol_code coin;
      decimal price;
      uint32_t fetched_at;

      uint64_t primary_key() const {
        return coin.raw();
      }
    };
    typedef multi_index<name("oracle"), oracle> oracle_tlb;
    oracle_tlb oracles;

    oracle get_oracle(extended_symbol coin);

    decimal get_oracle_price(extended_symbol coin);

    bool update(fetch f);

    decimal execute_fetch(fetch f);

    weight_price execute_source(extended_symbol coin, extended_symbol anchor, uint8_t type, std::vector<std::string> args);

    weight_price fetch_multipath(extended_symbol coin, extended_symbol anchor, std::vector<std::string> args);

    decimal get_oracle_price(std::vector<std::string> args);

    uint64_t upstat(std::vector<extended_symbol> upsyms);
  };
}