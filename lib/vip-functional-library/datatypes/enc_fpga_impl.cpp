#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>
#include <uuid/uuid.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <bitset>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <assert.h> 
#include "include/afu_json_info.h"
#include "include/enc_fpga_lib.h"
#include "include/config.h"
using namespace opae::fpga::types;
static handle::ptr_t accel = nullptr;
static uint64_t global_counter = 0;
shared_buffer::ptr_t req;
shared_buffer::ptr_t result;
uint8_t req_q_pointer = 0;
static ResultSlot result_track[NUM_FPGA_ENTRIES];

void poll_performance(){
	memset(((void*)result->c_type())+(NUM_FPGA_ENTRIES*WRITE_GRANULARITY), 0, sizeof(Performance_array));
	accel->write_csr64(24, NUM_FPGA_ENTRIES);
	volatile Performance_array* perf = (Performance_array*)(((void*)result->c_type())+(NUM_FPGA_ENTRIES*WRITE_GRANULARITY));
	while(!perf->valid){}
	std::cout <<"Total req: "<< perf->total_cycles<<std::endl;
	std::cout <<"[0, 10)"<< perf->mem_req_cycles[0] <<std::endl;
	std::cout <<"[10, 50)"<< perf->mem_req_cycles[1] <<std::endl;
	std::cout <<"[50, 100)"<< perf->mem_req_cycles[2] <<std::endl;
	std::cout <<"[100, 200)"<< perf->mem_req_cycles[3] <<std::endl;
	std::cout <<"[200, 300)"<< perf->mem_req_cycles[4] <<std::endl;
	std::cout <<"[300, 400)"<< perf->mem_req_cycles[5] <<std::endl;
	std::cout <<"[400, 500)"<< perf->mem_req_cycles[6] <<std::endl;
	std::cout <<"[500, 600)"<< perf->mem_req_cycles[7] <<std::endl;
	std::cout <<"[600, 700)"<< perf->mem_req_cycles[8] <<std::endl;
	std::cout <<"[700, 800)"<< perf->mem_req_cycles[9] <<std::endl;
	std::cout <<"[800, 900)"<< perf->mem_req_cycles[10] <<std::endl;
	std::cout <<"[900, 1000)"<< perf->mem_req_cycles[11] <<std::endl;
	std::cout <<"[1000, )"<< perf->mem_req_cycles[12] <<std::endl;
}

int close_accel(){
	accel->reset();
	req->release();
	result->release();
	accel->close();
	return 0;
}
int init_accel(){
	if(!accel){
		properties::ptr_t filter = properties::get();
		filter->guid.parse(AFU_ACCEL_UUID);
		filter->type = FPGA_ACCELERATOR;

		std::vector<token::ptr_t> tokens = token::enumerate({filter});

		// assert we have found at least one
		if (tokens.size() < 1) {
			std::cerr << "accelerator not found\n";
			return -1;
		}
		token::ptr_t tok = tokens[0];
		accel = handle::open(tok, FPGA_OPEN_SHARED);
		accel->reset();
		req = shared_buffer::allocate(accel, NUM_CACHELINE* SIZE_OF_CACHELINE);
		result = shared_buffer::allocate(accel,(NUM_FPGA_ENTRIES+1)*WRITE_GRANULARITY);
	  std::fill_n(req->c_type(), NUM_OPERAND_ENTRIES*READ_GRANULARITY, 0);
	  std::fill_n(result->c_type(), (NUM_FPGA_ENTRIES+1)*WRITE_GRANULARITY, 0);

		// open accelerator and map MMIO
		accel->reset();

		uint64_t read_setup = 0;
		read_setup += ((req->io_address())>>6);
		uint64_t write_setup = 0;
		write_setup += ((result->io_address())>>6);
		write_setup += (WRITE_GRANULARITY << 42);
	  __sync_synchronize();
		accel->write_csr64(0, read_setup);
		accel->write_csr64(8, write_setup);
	}
	return 0;
	// std::cout <<"initliazed success"<<std::endl;
}


/*
 enc_int:print_val
 print the value of the enc_int
*/
void enc_int::print_val(){
	std::cout << "enc_int: ";
	for(int i =0; i <16; i++){
		std::cout << (int)this->val.value[i] <<std::endl;
	}
	std::cout << std::endl;
}

void enc_int::get_val_at_slot(const uint8_t& pos, bool keep){
	// std::cout <<"in get_val_at_slot"<<std::endl;
	// std::cout <<"pos_to_get: "<<(int)pos <<" ptr: "<<(int)result_track[pos].ptr <<" list_size: "<< result_track[pos].crntEncInt.size()<<std::endl;
	volatile Result* ptr = result_track[pos].ptr;
	if(ptr != nullptr){
		while(ptr->result){}
		for(auto it = result_track[pos].crntEncInt.begin(); it != result_track[pos].crntEncInt.end(); ++it){
			(*it)->in_fpga = keep;
			if(!(*it)->valid){
				(*it)->val = ptr->result;
				(*it)->valid = true;
			}
		}
	}
	// std::cout <<"get_val_at_slot success"<<std::endl;
	return;
}

enc_int& enc_int::get_val(){
	// std::cout <<"in get_val"<<std::endl;
	if(!this->valid){
		if(!this->in_fpga){
			assert(("in_fpga should be true but now false", this->in_fpga));
		}
		get_val_at_slot(this->location, true);
	}
	// std::cout <<"get_val success"<<std::endl;
	return *this;
}

enc_int enc_int::compute(enc_int const &obj1, enc_int const &obj2, const uint8_t inst){
	// std::cout <<"in compute"<<std::endl;
	OpAddr ops[3];
	enc_int ret;
	uint8_t result_q_pointer = global_counter % NUM_FPGA_ENTRIES;
	uint64_t mask = (1<<9-1)<<4;
	/*with reuse*/
	#ifdef REUSE
	if( this->in_fpga && result_q_pointer!= this->location){
		ops[0].addr = this->location + (1 << 14);
	}else{
		uint16_t offset = CALC_OFFSET(req_q_pointer);
		uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
		uint8_t id = CALC_ID(req_q_pointer);
		ops[0].addr = offset + cacheline + (id << 14) + (1 << 15);
		Operand op;
		memcpy(op.val, this->val.value,sizeof(op.val));
		op.valid_w_id = id + 2;
		mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
		req_q_pointer++;
	}

	if( obj1.in_fpga && result_q_pointer!= obj1.location ){
		ops[1].addr = obj1.location + (1 << 14);
	}else{
		uint16_t offset = CALC_OFFSET(req_q_pointer);
		uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
		uint8_t id = CALC_ID(req_q_pointer);
		ops[1].addr = offset + cacheline + (id << 14) + (1 << 15);
		Operand op;
		memcpy(op.val, obj1.val.value,sizeof(op.val));
		op.valid_w_id = id + 2;
		mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
		req_q_pointer++;
	}
	#endif
	/*no reuse*/
	#ifdef NOREUSE
	uint16_t offset = CALC_OFFSET(req_q_pointer);
	uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
	uint8_t id = CALC_ID(req_q_pointer);
	ops[0].addr = offset + cacheline + (id << 14) + (1 << 15);
	Operand op;
	memcpy(op.val, this->val.value,sizeof(op.val));
	op.valid_w_id = id + 2;
	mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
	req_q_pointer++;
	offset = CALC_OFFSET(req_q_pointer);
	cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
	id = CALC_ID(req_q_pointer);
	ops[1].addr = offset + cacheline + (id << 14) + (1 << 15);
	memcpy(op.val, obj1.val.value,sizeof(op.val));
	op.valid_w_id = id + 2;
	mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
	req_q_pointer++;
	#endif

	if(inst == Inst::CMOV){
		/*with reuse*/
		#ifdef REUSE
		if(obj2.in_fpga && result_q_pointer!= obj2.location ){
			ops[2].addr = obj2.location  + (1 << 14);
		}else{
			uint16_t offset = CALC_OFFSET(req_q_pointer);
			uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
			uint8_t id = CALC_ID(req_q_pointer);
			ops[2].addr = offset + cacheline + (id << 14) + (1 << 15);
			Operand op;
			memcpy(op.val, obj2.val.value,sizeof(op.val));
			op.valid_w_id = id + 2;
			mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
			req_q_pointer++;
		}
		#endif
		/*no reuse*/
		#ifdef NOREUSE
		uint16_t offset = CALC_OFFSET(req_q_pointer);
		uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
		uint8_t id = CALC_ID(req_q_pointer);
		ops[2].addr = offset + cacheline + (id << 14) + (1 << 15);
		Operand op;
		memcpy(op.val, obj2.val.value,sizeof(op.val));
		op.valid_w_id = id + 2;
		mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);	
		req_q_pointer++;
		#endif
	}else{
		ops[2].addr = 0;
	}

	get_val_at_slot(result_q_pointer, false);
	result_track[result_q_pointer].ptr = nullptr;
	result_track[result_q_pointer].crntEncInt.clear();

	ret.valid = false;
	ret.location = result_q_pointer;
	ret.in_fpga = true;
	
	result_track[result_q_pointer].ptr = (Result*)((void*)result->c_type() +result_q_pointer*WRITE_GRANULARITY);
	uint64_t write_req = 0;

	write_req += result_q_pointer;


	mempcpy((uint8_t*)&write_req+1, ops, 3*sizeof(OpAddr));
	uint8_t insmod =  inst;
	write_req += ((uint64_t)insmod) << 56;

	global_counter ++;
	memset((void*)result_track[result_q_pointer].ptr->result.value,0,sizeof(uint8_t)*16);

	result_track[result_q_pointer].crntEncInt.push_back(&ret);

  __sync_synchronize();

	accel->write_csr64(16, write_req);
	// std::cout <<"compute success"<<std::endl;
	return ret;
}

enc_int::~enc_int(){
	// std::cout <<"in ~enc_int"<<std::endl;
	if(this->in_fpga){
		result_track[this->location].crntEncInt.remove(this);
	}
	// std::cout <<"~enc_int success"<<std::endl;
}


enc_int enc_int::operator + (enc_int const &obj){
	return this->compute(obj, enc_int(), Inst::ADD);
}

enc_int enc_int::operator - (enc_int const &obj){
	return this->compute(obj, enc_int(), Inst::SUB);
}

enc_int enc_int::operator- (){
	return this->compute(enc_int(), enc_int(), Inst::NEG);
}

enc_int enc_int::operator / (enc_int const &obj){
	return this->compute(obj, enc_int(), Inst::DIV);
}
enc_int enc_int::operator % (enc_int const &obj){
	return this->compute(obj, enc_int(), Inst::MOD);
}
enc_int enc_int::operator * (enc_int const &obj){
	return this->compute(obj, enc_int(), Inst::MUL);
}

enc_int enc_int::CMOV(enc_int const &t, enc_int const &f){
	return this->compute(t, f, Inst::CMOV);
}

enc_int& enc_int::operator = (enc_int const &obj){
	if(this->in_fpga){
		result_track[this->location].crntEncInt.remove(this);
	}
	this->val = obj.val;
	this->valid = obj.valid;
	this->in_fpga = obj.in_fpga;
	this->location = obj.location;
	if(this->in_fpga){
		result_track[this->location].crntEncInt.push_back(this);
	}
	return *this;
}

enc_int enc_int::operator < (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::LT);
	result.val = 0;
	return result;
}
enc_int enc_int::operator > (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::GT);
	result.val = 0;
	return result;
}

enc_int enc_int::operator << (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::LSHIFT);
	result.val = 0;
	return result;
}
enc_int enc_int::operator >> (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::RSHIFT);
	result.val = 0;
	return result;
}
enc_int enc_int::operator == (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::EQ);
	result.val = 0;
	return result;
}
enc_int& enc_int::operator = (int const &obj){
	this->val = encrypt_64_128(((uint64_t*) &obj));
	this->valid = true; 
	this->location = 0;
	this->in_fpga = false;
	return *this;
}


enc_int enc_int::operator <= (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::LEQ);
	result.val = 0;
	return result;
}
enc_int enc_int::operator >= (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::GEQ);
	result.val = 0;
	return result;
}
enc_int enc_int::operator != (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::NEQ);
	result.val = 0;
	return result;
}

enc_int enc_int::operator && (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::AND);
	result.val = 0;
	return result;
}

enc_int enc_int::operator & (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::AND);
	result.val = 0;
	return result;
}

enc_int enc_int::operator || (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::OR);
	result.val = 0;
	return result;
}

enc_int enc_int::operator | (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::OR);
	result.val = 0;
	return result;
}

enc_int enc_int::operator ^ (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::XOR);
	result.val = 0;
	return result;
}

enc_int enc_int::operator !(){
	enc_int result = this->compute(enc_int(), enc_int(), Inst::NOT);
	result.val = 0;
	return result;
}

int enc_int::decrypt_int(){
	// More complex decryption function that implements pointer casts rather than static casts 
	uint64_t value = decrypt_128_64(this->val);
	return *((int*) &value);
}
int enc_int::GET_DECRYPTED_VALUE(){
	return this->decrypt_int();
}

enc_int operator+(enc_int& obj, const int& that){
	return obj + enc_int(that);
}
enc_int operator-(enc_int& obj, const int& that){
	return obj - enc_int(that);
}
enc_int operator*(enc_int& obj, const int& that){
	return obj * enc_int(that);
}
enc_int operator/(enc_int& obj, const int& that){
	return obj / enc_int(that);
}
enc_int operator%( enc_int& obj, const int& that){
	return obj % enc_int(that);
}

enc_int operator>(enc_int& obj, const int& that){
	return obj > enc_int(that);
}
enc_int operator<(enc_int& obj, const int& that){
	return obj < enc_int(that);
}
enc_int operator==(enc_int& obj, const int& that){
	return obj == enc_int(that);
}
enc_int operator!=(enc_int& obj, const int& that){
	return obj != enc_int(that);
}
enc_int operator>=(enc_int& obj, const int& that){
	return obj >= enc_int(that);
}
enc_int operator<=(enc_int& obj, const int& that){
	return obj <= enc_int(that);
}
enc_int operator||(enc_int& obj, const int& that){
	return obj || enc_int(that);
}
enc_int operator &&(enc_int& obj, const int& that){
	return obj && enc_int(that);
}
enc_int operator|(enc_int& obj, const int& that){
	return obj | enc_int(that);
}
enc_int operator&(enc_int& obj, const int& that){
	return obj & enc_int(that);
}
enc_int operator^(enc_int& obj, const int& that){
	return obj ^ enc_int(that);
}

enc_int operator+(const int& that, enc_int& obj){
	return enc_int(that) + obj;
}
enc_int operator-(const int& that, enc_int& obj){
	return enc_int(that) - obj;
}

enc_int operator*(const int& that, enc_int& obj){
	return enc_int(that) * obj;
}

enc_int operator/(const int& that, enc_int& obj){
	return enc_int(that) / obj;
}

enc_int operator%(const int& that, enc_int& obj){
	return enc_int(that) % obj;
}

enc_int operator>(const int& that, enc_int& obj){
	return enc_int(that) > obj;
}
enc_int operator<(const int& that, enc_int& obj){
	return enc_int(that) < obj;
}

enc_int operator>>(const int& that, enc_int& obj){
	return enc_int(that) >> obj;
}
enc_int operator<<(const int& that, enc_int& obj){
	return enc_int(that) << obj;
}

enc_int operator==(const int& that, enc_int& obj){
	return enc_int(that) == obj;
}

enc_int operator!=(const int& that, enc_int& obj){
	return enc_int(that) != obj;
}

enc_int operator>=(const int& that,  enc_int& obj){
	return enc_int(that) >= obj;
}

enc_int operator<=(const int& that, enc_int& obj){
	return enc_int(that) <= obj;
}

enc_int operator&&(const int& that, enc_int& obj){
	return enc_int(that) && obj;
}

enc_int operator||(const int& that, enc_int& obj){
	return enc_int(that) || obj;
}

enc_int operator&(const int& that, enc_int& obj){
	return enc_int(that) & obj;
}

enc_int operator|(const int& that, enc_int& obj){
	return enc_int(that) | obj;
}

enc_int operator^(const int& that,enc_int& obj){
	return enc_int(that) ^ obj;
}

enc_int CMOV(enc_int& cond, enc_int& t, enc_int& f){
	return cond.CMOV(t, f);
}

enc_int CMOV(enc_int& cond, const int& t,  enc_int& f){
	enc_int enc_tv = enc_int(t);
	return CMOV(cond, enc_tv, f);
}

enc_int CMOV(enc_int& cond, enc_int& t, const int& f){
	enc_int enc_f = enc_int(f);
	return CMOV(cond, t, enc_f);
}
