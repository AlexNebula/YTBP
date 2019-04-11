#include "hdddata.hpp"
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosio.token/eosio.token.hpp>

using namespace eosio;

const uint32_t hours_in_one_day = 24;
const uint32_t minutes_in_one_day = hours_in_one_day * 60;
const uint32_t seconds_in_one_day = minutes_in_one_day * 60;
const uint32_t seconds_in_one_week = seconds_in_one_day * 7;
const uint32_t seconds_in_one_year = seconds_in_one_day * 365;
const name HDD_ACCOUNT = "hddofficial"_n;

// constructor
hdddata::hdddata( name s, name code, datastream<const char*> ds )
 : eosio::contract(s, code, ds),
   t_hddbalance(_self, _self.value),
   t_miningaccount(_self, _self.value),
   t_hddmarket(_self, _self.value),
   t_producer(_self, _self.value)  {
	//todo initialize produers ?
	   
	auto hddbalance_itr = t_hddbalance.find(HDD_ACCOUNT.value);
	if(hddbalance_itr == t_hddbalance.end()) {
		t_hddbalance.emplace(_self, [&](auto &row) {
			//todo check the 1st time insert
			row.last_hdd_balance=0;
			row.hdd_per_cycle_fee=0;
			row.hdd_per_cycle_profit=0;
			row.hdd_space=0;
			row.last_hdd_time = current_time();
		});
	}
	
	auto itr = t_hddmarket.find(hddcore_symbol.raw());
    if( itr == t_hddmarket.end() ) {
        //auto system_token_supply   = eosio::token(token_account).get_supply(eosio::symbol_type(system_token_symbol).name()).amount;
       //auto system_token_supply   = eosio::token::get_supply(token_account, yta_symbol.code() );
		auto system_token_supply = 0;
	   if( system_token_supply > 0 ) {
            itr = t_hddmarket.emplace( _self, [&]( auto& m ) {
               m.supply.amount = 100000000000000ll;
               m.supply.symbol = hddcore_symbol;
               //m.base.balance.amount = int64_t(_gstate.free_ram());
               m.base.balance.symbol = hdd_symbol;
               //m.quote.balance.amount = system_token_supply.amount / 1000;
               m.quote.balance.symbol = yta_symbol;
            });
         }
      } else {
         //print( "hdd market already created" );
      }
	  
	
}

 symbol hdddata::core_symbol()const {
      const static auto sym = get_core_symbol( t_hddmarket );
      return sym;
   }
   
//@abi action
void hdddata::get_hdd_balance(name owner) {
	//	当前余额=上次余额+(当前时间-上次余额时间)*（每周期收益-每周期费用）
	require_auth(owner);	
	//hddbalance_table  t_hddbalance(_self, _self.value);
	auto hddbalance_itr = t_hddbalance.find(owner.value);
	if(hddbalance_itr == t_hddbalance.end()) {
		t_hddbalance.emplace(owner, [&](auto &row) {
			//todo check the 1st time insert
			row.last_hdd_balance=0;
			row.hdd_per_cycle_fee=0;
			row.hdd_per_cycle_profit=0;
			row.hdd_space=0;
			row.last_hdd_time = current_time();
		});
	} else {
		t_hddbalance.modify(hddbalance_itr, _self, [&](auto &row) {
			//todo  check overflow and time cycle 
			row.last_hdd_balance=
				hddbalance_itr->get_last_hdd_balance() + 
				//( current_time() - (hddbalance_itr->last_hdd_time) )
				10 * ( hddbalance_itr->get_hdd_per_cycle_profit()-hddbalance_itr->get_hdd_per_cycle_fee() );
			row.last_hdd_time = current_time();
		});
	}

}

void hdddata::get_hdd_sum_balance() {
	require_auth(_self);
	
	auto hddbalance_itr = t_hddbalance.find(HDD_ACCOUNT.value);
	if(hddbalance_itr != t_hddbalance.end()) {
		//todo check the 1st time insert
		t_hddbalance.modify(hddbalance_itr, _self, [&](auto &row) {
		//todo  check overflow and time cycle 
		row.last_hdd_balance=
			hddbalance_itr->get_last_hdd_balance() + 
			// todo ((uint64_t)( now()-hddbalance_itr->last_hdd_time ))
			10 *( hddbalance_itr->get_hdd_per_cycle_profit()-hddbalance_itr->get_hdd_per_cycle_fee() );
		row.last_hdd_time = current_time();
		});
	}
}
//@abi action
void hdddata::set_hdd_per_cycle_fee(name owner, uint64_t fee) {
	require_auth(_self);
	require_auth(owner);
	//hddbalance_table  t_hddbalance(owner, owner.value);
	auto hddbalance_itr = t_hddbalance.find(owner.value);
	if(hddbalance_itr != t_hddbalance.end()) {
		//每周期费用 <= （占用存储空间*数据分片大小/1GB）*（记账周期/ 1年）
		//eos_assert( xxx, "");
		t_hddbalance.modify(hddbalance_itr, owner, [&](auto &row) {
			//todo check overflow
			row.hdd_per_cycle_fee -=fee;
		});
	}
	//每周期费用 <= （占用存储空间*数据分片大小/1GB）*（记账周期/ 1年）
}

//@abi action
void hdddata::sub_hdd_balance(name owner,  uint64_t balance){
	require_auth(_self);
	require_auth(owner);
	//hddbalance_table  t_hddbalance(owner, owner.value);
	auto hddbalance_itr = t_hddbalance.find(owner.value);
	if(hddbalance_itr != t_hddbalance.end()) {
		t_hddbalance.modify(hddbalance_itr, owner, [&](auto &row) {
			//todo check overflow
			row.last_hdd_balance -=balance;
		});
	}
}

//@abi action
void hdddata::add_hdd_space(name owner, name hddaccount, uint64_t space){
	require_auth(owner);
	require_auth(hddaccount);
	//hddbalance_table  t_hddbalance(_self, _self.value);
	auto hddbalance_itr = t_hddbalance.find(hddaccount.value);
	if(hddbalance_itr != t_hddbalance.end()) {
		t_hddbalance.modify(hddbalance_itr, hddaccount, [&](auto &row) {
			//todo check overflow
			row.hdd_space +=space;
		});
	}
}

//@abi action
void hdddata::sub_hdd_space(name owner, name hddaccount, uint64_t space){
	require_auth(owner);
	require_auth(hddaccount);
	//hddbalance_table  t_hddbalance(_self, _self.value);
	auto hddbalance_itr = t_hddbalance.find(hddaccount.value);
	if(hddbalance_itr != t_hddbalance.end()) {
		t_hddbalance.modify(hddbalance_itr, hddaccount, [&](auto &row) {
			//todo check overflow
			row.hdd_space -=space;
		});
	}
}

//@abi action
void hdddata::create_mining_account(name mining_name, name owner) {
	require_auth(mining_name);
	//mining_account t_miningaccount(_self, _self.value);
	auto mining_account_itr = t_miningaccount.find(mining_name.value);
	if(mining_account_itr == t_miningaccount.end()) {
		t_miningaccount.emplace(mining_name, [&](auto &row) {
			row.ming_id = mining_name.value;
			row.owner = owner;
		 });
	} else {
		return ;
	}
	
	require_auth(owner);
	//hddbalance_table  t_hddbalance(_self, _self.value);
	auto hddbalance_itr = t_hddbalance.find(owner.value);
	if(hddbalance_itr == t_hddbalance.end()) {
	t_hddbalance.emplace(owner, [&](auto &row) {
		//todo check the 1st time insert
		row.owner = owner;
		row.hdd_space=0;
	});
	} 
}

//@abi action
void hdddata::add_mining_profit(name mining_name, uint64_t space){
	require_auth(mining_name);
	//mining_account t_miningaccount(_self, _self.value);
	auto mining_account_itr = t_miningaccount.find(mining_name.value);
	if( mining_account_itr != t_miningaccount.end()) {
	//hddbalance_table  t_hddbalance(_self, _self.value);
		auto owner_id = mining_account_itr->get_owner();
		auto hddbalance_itr = t_hddbalance.find(owner_id);
		if(hddbalance_itr == t_hddbalance.end()) {
			t_hddbalance.emplace(_self, [&](auto &row) {
			//todo check the 1st time insert
			//row.owner = owner;
			row.hdd_space=space;
			});
		} else {
		//todo 
		}
	}
	//每周期收益 += (预采购空间*数据分片大小/1GB）*（记账周期/ 1年）		
}
	
//@abi action
void hdddata::buyhdd(name buyer, name receiver, asset quant) {
	require_auth(buyer);
	eosio_assert( quant.amount > 0, "must purchase a positive amount" );

	auto fee = quant;
	fee.amount = ( fee.amount + 199 ) / 200; /// .5% fee (round up)
	// fee.amount cannot be 0 since that is only possible if quant.amount is 0 which is not allowed by the assert above.
	// If quant.amount == 1, then fee.amount == 1,
	// otherwise if quant.amount > 1, then 0 < fee.amount < quant.amount.
	auto quant_after_fee = quant;
	quant_after_fee.amount -= fee.amount;
	INLINE_ACTION_SENDER(eosio::token, transfer)( token_account, {buyer,active_permission},
		{ buyer, hdd_account, quant_after_fee, std::string("buy hdd") } );

	if( fee.amount > 0 ) {
		INLINE_ACTION_SENDER(eosio::token, transfer)( token_account, {buyer,active_permission},
											   { buyer, hddfee_account, fee, std::string("hdd fee") } );
	}

	int64_t bytes_out;

	const auto& market = t_hddmarket.get(hddcore_symbol.raw(), "hdd market does not exist");
	t_hddmarket.modify( market, same_payer, [&]( auto& es ) {
		//bytes_out = es.convert( quant_after_fee,  S(0,HDD) ).amount;
	});

	eosio_assert( bytes_out > 0, "must reserve a positive amount" );
	//todo modify the hddbalance table
}

//@abi action
void hdddata::sellhdd(name account, uint64_t quant){
	require_auth(account);
}

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if(code==receiver)
        {
            switch(action)
            {
                //EOSIO_DISPATCH_HELPER( hdddata, (get_hdd_balance)(get_hdd_sum_balance)(set_hdd_per_cycle_fee)(create_mining_account)(add_mining_profit)(sub_hdd_balance)(buyhdd)(sellhdd)(add_hdd_space)(sub_hdd_space) )
            }
        }
        else if(code==HDD_ACCOUNT.value && action=="transfer"_n.value) {
            //execute_action( name(receiver), name(code), &hdddata::deposithdd );
        }
    }
};