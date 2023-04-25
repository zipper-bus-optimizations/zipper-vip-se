#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>
#include <cstdint>
#include <list>
#include "../../interface/interface.h"

#ifndef __OPAE_CPP_HARDVIP_H__
#define __OPAE_CPP_HARDVIP_H__

using namespace opae::fpga::types;

int64_t close_accel();

int64_t setParams();

int64_t init_accel();
typedef struct Inst{
	public:
	const static uint8_t NOOP = 0;
	const static uint8_t CMOV = 0b01100000; 
	const static uint8_t ADD =  0b00100000;
	const static uint8_t SUB =  0b00100100;
	const static uint8_t MUL =  0b00101000;
	const static uint8_t NEG =  0b00110100;
	const static uint8_t LSHIFT = 0b00000100;
	const static uint8_t RSHIFT = 0b00001000;
	const static uint8_t LT  = 0b01000100;
	const static uint8_t GT  = 0b01001000;
	const static uint8_t LEQ = 0b01001100;
	const static uint8_t GEQ = 0b01010000;
	const static uint8_t EQ  = 0b01010100;
	const static uint8_t NEQ = 0b01011000;
	const static uint8_t XOR = 0b10000000;
	const static uint8_t OR  = 0b10000100;
	const static uint8_t AND = 0b10001000;
	const static uint8_t NOT = 0b10001100;
} Inst;
struct OpAddr{
	/*2 bits for mode, if mode == 2, then the least significant bit is for id*/
	/*one hot encoding for cachline*/
	/*offset inside the cacheline*/
	uint16_t addr = 0;
};
struct Result{
	bit128_t result;
};

struct Performance_array{
	uint64_t total_cycles = 0;
	uint32_t mem_req_cycles[12]= {0};
	bool valid = 0;
};


template <class T>
class Counter
{
  private:
      static uint64_t created;
	  static uint64_t living;
		static uint64_t most;
  public:
    Counter()
    {
       created++;
	   living++;
	   if (most < living) {
		most = living;
	   }
    }  
    Counter(const Counter &c)
    {
       created++;
	   living++;
	   if (most < living) {
		most = living;
	   }
    }   
    ~Counter()
    {
       living--;
    }    
    static uint64_t getCreated() {

         return created;
    }
	static uint64_t getMost() {
		return most;
	}
};

template<class T> 
uint64_t Counter<T>::living = 0; 
template<class T>
uint64_t Counter<T>::created = 0;
template<class T>
uint64_t Counter<T>::most = 0;

class enc_int: private Counter<enc_int>{
	public:
		using Counter<enc_int>::getCreated;
		using Counter<enc_int>::getMost;
		bit128_t val;
		bool valid;
		bool in_fpga;
		uint8_t location;
		bool was_computed = false;
		uint64_t global_location = 0;
		void print_val();
		enc_int& get_val();
		enc_int(const int64_t in_val);
		enc_int(const bit128_t in_val);
		enc_int();
		~enc_int();
		enc_int(const enc_int &obj);
		enc_int compute(enc_int const &obj1, enc_int const &obj2, const uint8_t inst);
		enc_int operator + (enc_int const &obj);
		enc_int operator - (enc_int const &obj);
		enc_int operator- ();
		enc_int operator / (enc_int &obj);
		enc_int operator * (enc_int const &obj);
		enc_int operator % (enc_int &obj);
		enc_int operator < (enc_int const &obj);
		enc_int operator > (enc_int const &obj);
		enc_int operator << (enc_int const &obj);
		enc_int operator >> (enc_int const &obj);
		enc_int operator <= (enc_int const &obj);
		enc_int operator >= (enc_int const &obj);
		enc_int operator == (enc_int const &obj);
		enc_int operator != (enc_int const &obj);
		enc_int operator && (enc_int const &obj);
		enc_int operator || (enc_int const &obj);
		enc_int operator & (enc_int const &obj);
		enc_int operator | (enc_int const &obj);
		enc_int operator ^ (enc_int const &obj);
		enc_int operator!();
		enc_int CMOV(enc_int const &t, enc_int const &f);

		static void get_val_at_slot(const uint8_t& pos, bool keep);
		enc_int& operator = (enc_int const &obj);
		enc_int& operator = (int64_t const& obj);
		int64_t decrypt_int();
		int64_t decrypt_pad();
		int64_t GET_DECRYPTED_VALUE();
};

enc_int operator+(enc_int& obj, const int64_t& that);
enc_int operator-(enc_int& obj, const int64_t& that);
enc_int operator*(enc_int& obj, const int64_t& that);
enc_int operator/(enc_int& obj, int64_t& that);
enc_int operator%( enc_int& obj,  int64_t& that);
enc_int operator>(enc_int& obj, const int64_t& that);
enc_int operator<(enc_int& obj, const int64_t& that);
enc_int operator==(enc_int& obj, const int64_t& that);
enc_int operator!=(enc_int& obj, const int64_t& that);
enc_int operator>=(enc_int& obj, const int64_t& that);
enc_int operator<=(enc_int& obj, const int64_t& that);
enc_int operator||(enc_int& obj, const int64_t& that);
enc_int operator &&(enc_int& obj, const int64_t& that);
enc_int operator|(enc_int& obj, const int64_t& that);
enc_int operator&(enc_int& obj, const int64_t& that);
enc_int operator^(enc_int& obj, const int64_t& that);
enc_int operator+(const int64_t& that, enc_int& obj);
enc_int operator-(const int64_t& that, enc_int& obj);
enc_int operator*(const int64_t& that, enc_int& obj);
enc_int operator/(const int64_t& that, enc_int& obj);
enc_int operator%(const int64_t& that, enc_int& obj);
enc_int operator>(const int64_t& that, enc_int& obj);
enc_int operator<(const int64_t& that, enc_int& obj);
enc_int operator>>(const int64_t& that, enc_int& obj);
enc_int operator<<(const int64_t& that, enc_int& obj);
enc_int operator==(const int64_t& that, enc_int& obj);
enc_int operator!=(const int64_t& that, enc_int& obj);
enc_int operator>=(const int64_t& that,  enc_int& obj);
enc_int operator<=(const int64_t& that, enc_int& obj);
enc_int operator&&(const int64_t& that, enc_int& obj);
enc_int operator||(const int64_t& that, enc_int& obj);
enc_int operator&(const int64_t& that, enc_int& obj);
enc_int operator|(const int64_t& that, enc_int& obj);
enc_int operator^(const int64_t& that,enc_int& obj);
enc_int CMOV(enc_int& cond, enc_int& t, enc_int& f);
enc_int CMOV(enc_int& cond, const int64_t& t,  enc_int& f);
enc_int CMOV(enc_int& cond, enc_int& t, const int64_t& f);

void poll_performance();

struct ResultSlot{
		volatile Result* ptr = nullptr;
		std::list<enc_int*> crntEncInt;
};
#endif
