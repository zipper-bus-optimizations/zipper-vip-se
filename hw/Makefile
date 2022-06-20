RTL=./rtl
SOURCE=${RTL}/sources.txt
BUILD_DIR=build_fpga

sim: rtl/sources.txt rtl/vip_ccip.sv rtl/SE.v
	rm -rf build_sim
	afu_sim_setup --source rtl/sources.txt build_sim
	cd build_sim && make
	
run_sim:
	cd build_sim && make sim

rebuild: clean build
	cd build_fpga

build: setup
	cd build_fpga; qsub-synth

setup: ${SOURCE} ${RTL}/vip_ccip.sv ${RTL}/se.v
	afu_synth_setup -s ${SOURCE} ${BUILD_DIR}

.PHONY:clean

clean:
	rm -rf ${BUILD_DIR}
