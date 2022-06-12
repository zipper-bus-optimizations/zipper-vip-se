include ../../common/sw/common_include.mk


CPPFLAGS += $(CFLAGS) -Wno-cpp -maes
SRC_DIR =
INCLUDE_DIR =
LIB_DIR = lib
OBJ_DIR = obj
# Files and folders
SRCS_OBJS = $(OBJ_DIR)/vip-functional-library.o $(OBJ_DIR)/interface/interface.o $(OBJ_DIR)/interface/parameters.o $(OBJ_DIR)/interface/interface.o $(OBJ_DIR)/rng/rng_interface.o $(OBJ_DIR)/interface/interface.o $(OBJ_DIR)/rng/mersenne-twister-1/mersenne-twister.o $(OBJ_DIR)/interface/interface.o $(OBJ_DIR)/ciphers/aes/aes_interface.o $(OBJ_DIR)/interface/interface.o $(OBJ_DIR)/ciphers/xor/xor_interface.o
SRCS_OBJS_PATH = $(addprefix $(OBJ_DIR)/,$(notdir $(SRCS_OBJS)))

SRCS = datatypes/enc_fpga_impl.cpp

FPGA_LIB = libencintfpga
ASE_LIB = libencintase
AFU_JSON_INFO = datatypes/include/afu_json_info.h

all: $(SRCS_OBJS) all_reuse all_noreuse;

all_reuse: fpga ase ;

all_noreuse: fpga_noreuse ase_noreuse ;

fpga: $(AFU_JSON_INFO) $(LIB_DIR)/$(FPGA_LIB).a ;

ase: $(AFU_JSON_INFO) $(LIB_DIR)/$(ASE_LIB).a ;

fpga_noreuse: $(AFU_JSON_INFO) $(LIB_DIR)/$(FPGA_LIB)_noreuse.a ;

ase_noreuse: $(AFU_JSON_INFO) $(LIB_DIR)/$(ASE_LIB)_noreuse.a ;

$(LIB_DIR)/$(FPGA_LIB).a: $(OBJ_DIR)/$(FPGA_LIB).o   $(SRCS_OBJS_PATH)
	ar crf $@ $^ 

$(LIB_DIR)/$(FPGA_LIB)_noreuse.a: $(OBJ_DIR)/$(FPGA_LIB)_noreuse.o  $(SRCS_OBJS_PATH)
	ar crf $@ $^ 

$(OBJ_DIR)/$(FPGA_LIB).o:  $(SRCS) 
	$(CXX) -c  $^ -o $@ $(CPPFLAGS) -D REUSE $(LDFLAGS) $(FPGA_LIBS) -lrt

$(OBJ_DIR)/$(FPGA_LIB)_noreuse.o:  $(SRCS) 
	$(CXX) -c  $^ -o $@ $(CPPFLAGS) -D NOREUSE $(LDFLAGS) $(FPGA_LIBS) -lrt

# AFU info from JSON file, including AFU UUID
$(AFU_JSON_INFO): ../../hw/bdx-rtl.json
	afu_json_mgr json-info --afu-json=$^ --c-hdr=$@

$(LIB_DIR)/$(ASE_LIB).a: $(OBJ_DIR)/$(ASE_LIB).o  $(SRCS_OBJS_PATH)
	ar crf $@ $^ 

$(LIB_DIR)/$(ASE_LIB)_noreuse.a: $(OBJ_DIR)/$(ASE_LIB)_noreuse.o  $(SRCS_OBJS_PATH)
	ar crf $@ $^ 

$(OBJ_DIR)/$(ASE_LIB).o: $(SRCS) 
	$(CXX) -c  $^ $(CPPFLAGS) -D REUSE $(LDFLAGS) $(ASE_LIBS) -lrt -o $@

$(OBJ_DIR)/$(ASE_LIB)_noreuse.o: $(SRCS) 
	$(CXX) -c  $^ $(CPPFLAGS) -D NOREUSE $(LDFLAGS) $(ASE_LIBS) -lrt -o $@

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) -c  $^ $(CPPFLAGS) $(LDFLAGS) $(ASE_LIBS) -lrt -o $(OBJ_DIR)/$(notdir $@)

.PHONY: clean

clean:
	rm -rf $(LIB_DIR)/*.a $(OBJ_DIR)/*.o $(AFU_JSON_INFO)

objdir:
	@mkdir -p $(OBJDIR)

.PHONY: all clean
