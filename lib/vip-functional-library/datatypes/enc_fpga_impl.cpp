#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>
#include <uuid/uuid.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
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
uint64_t total_reuse = 0;
uint64_t total_call_memory = 0;
uint64_t total_call_compute = 0;
uint64_t total_effcient_compute = 0;
uint8_t req_q_pointer = 0;
bool finish = false;
static ResultSlot result_track[NUM_FPGA_ENTRIES];
uint64_t distances[20];
void poll_performance(){
	memset(((void*)result->c_type())+(NUM_FPGA_ENTRIES*WRITE_GRANULARITY), 0, sizeof(Performance_array));
	accel->write_csr64(24, NUM_FPGA_ENTRIES);
	volatile Performance_array* perf = (Performance_array*)(((void*)result->c_type())+(NUM_FPGA_ENTRIES*WRITE_GRANULARITY));
	while(!perf->valid){}
	std::cout <<"Total call memory: "<< total_call_memory<<std::endl;
	std::cout <<"Total reuse: "<< total_reuse<<std::endl;
	std::cout <<"Total call compute: "<< total_call_compute<<std::endl;
	std::cout <<"Total efficient compute: "<< total_effcient_compute<<std::endl;
	std::cout <<"Total created object: "<< Counter<enc_int>::getCreated() << std::endl;
	std::cout <<"Greatest living object: "<< Counter<enc_int>::getMost() << std::endl;
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
	for(int i = 0;i < 20; i++){
		std::cout << distances[i]/(double)total_call_memory;
		if(i<19) std::cout<<", ";
		else std::cout <<std::endl;
	}
}

int64_t close_accel(){
	finish = true;
	accel->reset();
	req->release();
	result->release();
	accel->close();
	return 0;
}

int64_t setParams(){
	setParameters();
	return 0;
}

int64_t init_accel(){
  	setParameters();
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
	  std::fill_n(req->c_type(), NUM_CACHELINE* SIZE_OF_CACHELINE, 0);
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

enc_int::enc_int(const enc_int &obj){
	if(this->in_fpga){
		result_track[this->location].crntEncInt.remove(this);
	}
	this->val = obj.val;
	this->valid = obj.valid;
	this->in_fpga = obj.in_fpga;
	this->location = obj.location;
	this->was_computed = obj.was_computed;
	this->global_location = obj.global_location;
	if(this->in_fpga){
		result_track[this->location].crntEncInt.push_back(this);
	}
	//std::cout <<"mark 14"<<std::endl;
}

enc_int::enc_int(const int64_t in_val){
	this->val = encrypt_64_128(in_val);
	this->valid = true; 
	this->location = 0;
	this->in_fpga = false;
};
enc_int::enc_int(const bit128_t in_val){

	this->val = in_val;
	this->valid = true; 
	this->location = 0;
	this->in_fpga = false;	
}
enc_int::enc_int(){
	uint64_t value = 0;
	this->val =  encrypt_64_128(value);
	this->valid = true; 
	this->location = 0;
	this->in_fpga = false;
};

/*
 enc_int:print_val
 print the value of the enc_int
*/
void enc_int::print_val(){
	std::cout << "enc_int: ";
	this->get_val();
	for(int64_t i =0; i <16; i++){
		std::cout <<  std::hex<< (this->val.value[i] >> 4);
		std::cout <<  std::hex<< (this->val.value[i] & ((1<<4)-1));

	}
	std::cout << std::endl;
}

void enc_int::get_val_at_slot(const uint8_t& pos, bool keep){
	// std::cout <<"in get_val_at_slot"<<std::endl;
	// std::cout <<"pos_to_get: "<<(int64_t)pos <<" ptr: "<<(int64_t)result_track[pos].ptr <<" list_size: "<< result_track[pos].crntEncInt.size()<<std::endl;
	volatile Result* ptr = result_track[pos].ptr;
	if(ptr != nullptr){
		while(!ptr->result){
			// std::cout <<"hasn't get result"<<std::endl;
			// std::cout <<"pos: "<<(int64_t)pos<<std::endl;
		}
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
	total_call_compute++;
	uint8_t flag = 0;
	OpAddr ops[3];
	enc_int ret;
	uint8_t result_q_pointer = global_counter % NUM_FPGA_ENTRIES;
	uint64_t mask = (1<<9-1)<<4;
	//std::cout << "mark 1"<<std::endl;
	/*with reuse*/
	#ifdef REUSE
	total_call_memory++;
	if(this->was_computed){
		uint64_t distance = global_counter-this->global_location;
		if(distance>=20){
			distances[19]++;
		}else if(distance >=1){
			distances[distance-1]++;
		}
	}
	if( this->in_fpga && (result_q_pointer%SIMULATED_ENTRIES)!= (this->location%SIMULATED_ENTRIES)){
	//std::cout << "mark 2"<<std::endl;
		total_reuse++;
		ops[0].addr = this->location + (1 << 14);
	}else{
	//std::cout << "mark 3"<<std::endl;
		flag = 1;
		uint16_t offset = CALC_OFFSET(req_q_pointer);
		uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
		uint8_t id = CALC_ID(req_q_pointer);
		ops[0].addr = offset + cacheline + (id << 14) + (1 << 15);
		// std::cout<<"cacheline: "<< std::bitset<16>(cacheline)<<std::endl;
		Operand op;
		this->get_val();
		memcpy(&op.val, &this->val.value,sizeof(op.val));
		op.valid_w_id = id + 2;
		mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
		// printf("cal:%d, cacheline:%d,offset:%d\n", CALC_OFFSET_IN_BYTE(req_q_pointer), CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer), CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer));
		req_q_pointer++;
	}
	if(obj1.was_computed){
		uint64_t distance = global_counter-obj1.global_location;
		if(distance>=20){
			distances[19]++;
		}else if(distance >=1){
			distances[distance-1]++;
		}
	}
	total_call_memory++;
	if( obj1.in_fpga && (result_q_pointer%SIMULATED_ENTRIES)!= (obj1.location%SIMULATED_ENTRIES) ){
	//std::cout << "mark 4"<<std::endl;
		total_reuse++;
		ops[1].addr = obj1.location + (1 << 14);
	}else{
	//std::cout << "mark 5"<<std::endl;
		flag = 1;
		uint16_t offset = CALC_OFFSET(req_q_pointer);
		uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
		uint8_t id = CALC_ID(req_q_pointer);
		ops[1].addr = offset + cacheline + (id << 14) + (1 << 15);
		// std::cout<< "req_q_pointer"<<req_q_pointer<<std::endl;
		// std::cout<< "bits for offset"<<std::bitset<64>(BITS_FOR_OFFSET)<<std::endl;
		// std::cout << "num cacheline: "<< (req_q_pointer%TOTAL_ENTRIES)/NUM_OPERAND_PER_CACHELINE<<std::endl;
		// std::cout<< "cacheline: "<< std::bitset<16>(cacheline)<<std::endl;
		Operand op;
		obj1.get_val();
		memcpy(&op.val, &obj1.val.value,sizeof(op.val));
		op.valid_w_id = id + 2;
		mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
		// printf("cal:%d, cacheline:%d,offset:%d\n", CALC_OFFSET_IN_BYTE(req_q_pointer), CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer), CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer));
		req_q_pointer++;
	}
	#endif
	/*no reuse*/
	#if defined(NOREUSE) || defined(BASELINE)
	//std::cout << "mark 6"<<std::endl;

	uint16_t offset = CALC_OFFSET(req_q_pointer);
	uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
	uint8_t id = CALC_ID(req_q_pointer);
	ops[0].addr = offset + cacheline + (id << 14) + (1 << 15);
	Operand op;
	this->get_val();
	memcpy(&op.val, &this->val.value,sizeof(op.val));
	op.valid_w_id = id + 2;
	// printf("id: %d",id);
	mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
	//std::cout << "mark 7"<<std::endl;
	req_q_pointer++;
	offset = CALC_OFFSET(req_q_pointer);
	cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
	id = CALC_ID(req_q_pointer);
	ops[1].addr = offset + cacheline + (id << 14) + (1 << 15);
	obj1.get_val();
	memcpy(&op.val, &obj1.val.value,sizeof(op.val));
	op.valid_w_id = id + 2;
	// printf("id: %d",id);
	mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
	req_q_pointer++;
	//std::cout << "mark 8"<<std::endl;

	#endif

	if(inst == Inst::CMOV){
		if(obj2.was_computed){
			uint64_t distance = global_counter-obj2.global_location;
			if(distance>=20){
				distances[19]++;
			}else if(distance >=1){
				distances[distance-1]++;
			}
		}
		/*with reuse*/
		#ifdef REUSE
		total_call_memory++;
		if(obj2.in_fpga && (result_q_pointer%SIMULATED_ENTRIES)!= (obj2.location%SIMULATED_ENTRIES) ){
			total_reuse++;
			ops[2].addr = obj2.location  + (1 << 14);
		}else{
			flag = 1;
			uint16_t offset = CALC_OFFSET(req_q_pointer);
			uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
			uint8_t id = CALC_ID(req_q_pointer);
			ops[2].addr = offset + cacheline + (id << 14) + (1 << 15);
			Operand op;
			obj2.get_val();
			memcpy(&op.val, &obj2.val.value,sizeof(op.val));
			op.valid_w_id = id + 2;
			mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);
	// printf("cal:%d, cacheline:%d,offset:%d\n", CALC_OFFSET_IN_BYTE(req_q_pointer), CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer), CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer));
			req_q_pointer++;
		}
		#endif
		/*no reuse*/
		#if defined(NOREUSE) || defined(BASELINE)
		uint16_t offset = CALC_OFFSET(req_q_pointer);
		uint16_t cacheline = CALC_CACHELINE_ONE_HOT(req_q_pointer);
		uint8_t id = CALC_ID(req_q_pointer);
		ops[2].addr = offset + cacheline + (id << 14) + (1 << 15);
		Operand op;
		obj2.get_val();
		memcpy(&op.val, &obj2.val.value,sizeof(op.val));
		op.valid_w_id = id + 2;
		mempcpy(((void*)req->c_type() + CALC_OFFSET_IN_BYTE(req_q_pointer) + CALC_CACHELINE_OFFSET_IN_BYTE(req_q_pointer)), &op, READ_GRANULARITY);	
		req_q_pointer++;
		#endif
	}else{
		ops[2].addr = 0;
	}

	#ifdef REUSE
	if (flag == 0) {total_effcient_compute++;}
	#endif

	get_val_at_slot((result_q_pointer+8-SIMULATED_ENTRIES)%8, false);

	result_track[result_q_pointer].ptr = nullptr;
	result_track[result_q_pointer].crntEncInt.clear();

	ret.valid = false;
	ret.location = result_q_pointer;
	ret.in_fpga = true;
	ret.was_computed = true;
	ret.global_location = global_counter;
	result_track[result_q_pointer].ptr = (Result*)((void*)result->c_type() +result_q_pointer*WRITE_GRANULARITY);

	uint64_t write_req = 0;

	write_req += result_q_pointer;


	mempcpy((uint8_t*)&write_req+1, ops, 3*sizeof(OpAddr));
	uint8_t insmod =  inst;
	write_req += ((uint64_t)insmod) << 56;
	//std::cout << "inst: "<<std::bitset<64>(write_req)<<std::endl;
	global_counter ++;
	memset((void*)result_track[result_q_pointer].ptr->result.value,0,sizeof(uint8_t)*16);

	result_track[result_q_pointer].crntEncInt.push_back(&ret);

  	__sync_synchronize();

	accel->write_csr64(16, write_req);
	//std::cout << "mark 10"<<std::endl;
	// std::cout <<"compute success"<<std::endl;

	#ifdef BASELINE
		ret.get_val();
	#endif
	// std::cout << global_counter<<std::endl;
	return ret;
}

enc_int::~enc_int(){
	// std::cout <<"in ~enc_int"<<std::endl;
	if(this->in_fpga && !finish){
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

enc_int enc_int::operator / (enc_int &obj){
	this->get_val();
	obj.get_val();
  uint64_t value1 = decrypt_128_64(this->val);
  uint64_t value2 = decrypt_128_64(obj.val);
	int64_t val1 =  *((int64_t*) &value1);
	int64_t val2 =  *((int64_t*) &value2);
	return enc_int(val1/val2);
}
enc_int enc_int::operator % (enc_int &obj){
	this->get_val();
	obj.get_val();
  uint64_t value1 = decrypt_128_64(this->val);
  uint64_t value2 = decrypt_128_64(obj.val);
	int64_t val1 =  *((int64_t*) &value1);
	int64_t val2 =  *((int64_t*) &value2);
	return enc_int(val1%val2);
}
enc_int enc_int::operator * (enc_int const &obj){
	return this->compute(obj, enc_int(), Inst::MUL);
}

enc_int enc_int::CMOV(enc_int const &f, enc_int const &cond){
	return this->compute(f, cond, Inst::CMOV);
}

enc_int& enc_int::operator = (enc_int const &obj){
	if(this->in_fpga){
		result_track[this->location].crntEncInt.remove(this);
	}
	this->val = obj.val;
	this->valid = obj.valid;
	this->in_fpga = obj.in_fpga;
	this->location = obj.location;
	this->was_computed = obj.was_computed;
	this->global_location = obj.global_location;
	if(this->in_fpga){
		result_track[this->location].crntEncInt.push_back(this);
	}
	//std::cout <<"mark 14"<<std::endl;
	return *this;
}

enc_int enc_int::operator < (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::LT);
	return result;
}
enc_int enc_int::operator > (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::GT);
	//std::cout <<"mark 11"<<std::endl;
	//std::cout <<"mark 12"<<std::endl;
	return result;
}

enc_int enc_int::operator << (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::LSHIFT);
	return result;
}
enc_int enc_int::operator >> (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::RSHIFT);
	return result;
}
enc_int enc_int::operator == (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::EQ);
	return result;
}
enc_int& enc_int::operator = (int64_t const &obj){
	this->val = encrypt_64_128(obj);
	this->valid = true; 
	this->location = 0;
	this->in_fpga = false;
	return *this;
}


enc_int enc_int::operator <= (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::LEQ);
	return result;
}
enc_int enc_int::operator >= (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::GEQ);
	return result;
}
enc_int enc_int::operator != (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::NEQ);
	return result;
}

enc_int enc_int::operator && (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::AND);
	return result;
}

enc_int enc_int::operator & (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::AND);
	return result;
}

enc_int enc_int::operator || (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::OR);
	return result;
}

enc_int enc_int::operator | (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::OR);
	return result;
}

enc_int enc_int::operator ^ (enc_int const &obj){
	enc_int result = this->compute(obj, enc_int(), Inst::XOR);
	return result;
}

enc_int enc_int::operator !(){
	enc_int result = this->compute(enc_int(), enc_int(), Inst::NOT);
	return result;
}

int64_t enc_int::decrypt_int(){
	// More complex decryption function that implements pointer casts rather than static casts 
	this->get_val();
	uint64_t value = decrypt_128_64(this->val);
	return *((int64_t*) &value);
}

int64_t enc_int::decrypt_pad(){
	// More complex decryption function that implements pointer casts rather than static casts 
	this->get_val();
	uint64_t value = decrypt_128_pad(this->val);
	return *((int64_t*) &value);
}

int64_t enc_int::GET_DECRYPTED_VALUE(){
	return this->decrypt_int();
}

enc_int operator+(enc_int& obj, const int64_t& that){
	return obj + enc_int(that);
}
enc_int operator-(enc_int& obj, const int64_t& that){
	return obj - enc_int(that);
}
enc_int operator*(enc_int& obj, const int64_t& that){
	return obj * enc_int(that);
}
enc_int operator/(enc_int& obj, int64_t& that){
	obj.get_val();
  uint64_t value = decrypt_128_64(obj.val);
	int64_t val =  *((int64_t*) &value);
	return enc_int(val/that);
}

enc_int operator%( enc_int& obj, int64_t& that){
	obj.get_val();
  uint64_t value = decrypt_128_64(obj.val);
	int64_t val =  *((int64_t*) &value);
	return enc_int(val%that);
}

enc_int operator>(enc_int& obj, const int64_t& that){
	return obj > enc_int(that);
}
enc_int operator<(enc_int& obj, const int64_t& that){
	return obj < enc_int(that);
}
enc_int operator==(enc_int& obj, const int64_t& that){
	return obj == enc_int(that);
}
enc_int operator!=(enc_int& obj, const int64_t& that){
	return obj != enc_int(that);
}
enc_int operator>=(enc_int& obj, const int64_t& that){
	return obj >= enc_int(that);
}
enc_int operator<=(enc_int& obj, const int64_t& that){
	return obj <= enc_int(that);
}
enc_int operator||(enc_int& obj, const int64_t& that){
	return obj || enc_int(that);
}
enc_int operator &&(enc_int& obj, const int64_t& that){
	return obj && enc_int(that);
}
enc_int operator|(enc_int& obj, const int64_t& that){
	return obj | enc_int(that);
}
enc_int operator&(enc_int& obj, const int64_t& that){
	return obj & enc_int(that);
}
enc_int operator^(enc_int& obj, const int64_t& that){
	return obj ^ enc_int(that);
}

enc_int operator+(const int64_t& that, enc_int& obj){
	return enc_int(that) + obj;
}
enc_int operator-(const int64_t& that, enc_int& obj){
	return enc_int(that) - obj;
}

enc_int operator*(const int64_t& that, enc_int& obj){
	return enc_int(that) * obj;
}

enc_int operator/(const int64_t& that, enc_int& obj){
	return enc_int(that) / obj;
}

enc_int operator%(const int64_t& that, enc_int& obj){
	return enc_int(that) % obj;
}

enc_int operator>(const int64_t& that, enc_int& obj){
	return enc_int(that) > obj;
}
enc_int operator<(const int64_t& that, enc_int& obj){
	return enc_int(that) < obj;
}

enc_int operator>>(const int64_t& that, enc_int& obj){
	return enc_int(that) >> obj;
}
enc_int operator<<(const int64_t& that, enc_int& obj){
	return enc_int(that) << obj;
}

enc_int operator==(const int64_t& that, enc_int& obj){
	return enc_int(that) == obj;
}

enc_int operator!=(const int64_t& that, enc_int& obj){
	return enc_int(that) != obj;
}

enc_int operator>=(const int64_t& that,  enc_int& obj){
	return enc_int(that) >= obj;
}

enc_int operator<=(const int64_t& that, enc_int& obj){
	return enc_int(that) <= obj;
}

enc_int operator&&(const int64_t& that, enc_int& obj){
	return enc_int(that) && obj;
}

enc_int operator||(const int64_t& that, enc_int& obj){
	return enc_int(that) || obj;
}

enc_int operator&(const int64_t& that, enc_int& obj){
	return enc_int(that) & obj;
}

enc_int operator|(const int64_t& that, enc_int& obj){
	return enc_int(that) | obj;
}

enc_int operator^(const int64_t& that,enc_int& obj){
	return enc_int(that) ^ obj;
}

enc_int CMOV(enc_int& cond, enc_int& t, enc_int& f){
	return t.CMOV(f, cond);
}

enc_int CMOV(enc_int& cond, const int64_t& t,  enc_int& f){
	enc_int enc_tv = enc_int(t);
	return CMOV(cond, enc_tv, f);
}

enc_int CMOV(enc_int& cond, enc_int& t, const int64_t& f){
	enc_int enc_f = enc_int(f);
	return CMOV(cond, t, enc_f);
}
