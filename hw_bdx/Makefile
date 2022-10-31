RTL=./rtl
SOURCE_COALESCE=${RTL}/sources_coalesce.txt
SOURCE_NOCOALESCE=${RTL}/sources_nocoalesce.txt
BUILD_DIR_COALESCE=build_fpga_coalesce
BUILD_DIR_NOCOALESCE=build_fpga_nocoalesce

sim_coalesce: rtl/sources_coalesce.txt rtl/vip_ccip.sv rtl/SE_coalesce.v
	rm -rf build_sim_coalesce
	afu_sim_setup --source rtl/sources_coalesce.txt build_sim_coalesce
	cd build_sim_coalesce && make

sim_nocoalesce: rtl/sources_nocoalesce.txt rtl/vip_ccip.sv rtl/SE_nocoalesce.v
	rm -rf build_sim_nocoalesce
	afu_sim_setup --source rtl/sources_nocoalesce.txt build_sim_nocoalesce
	cd build_sim_nocoalesce && make

rebuild_coalesce: clean_coalesce build_coalesce
	cd build_fpga_coalesce

rebuild_nocoalesce: clean_nocoalesce build_nocoalesce
	cd build_fpga_nocoalesce

build_coalesce: setup_coalesce
	cd build_fpga_coalesce; qsub-synth

build_nocoalesce: setup_nocoalesce
	cd build_fpga_nocoalesce; qsub-synth

setup_coalesce: ${SOURCE} ${RTL}/vip_ccip.sv ${RTL}/SE_coalesce.v
	afu_synth_setup -s ${SOURCE_COALESCE} ${BUILD_DIR_COALESCE}

setup_nocoalesce: ${SOURCE} ${RTL}/vip_ccip.sv  ${RTL}/SE_nocoalesce.v
	afu_synth_setup -s ${SOURCE_NOCOALESCE} ${BUILD_DIR_NOCOALESCE}

.PHONY:clean_coalesce clean_nocoalesce

clean_coalesce:
	rm -rf ${BUILD_DIR_COALESCE}

clean_nocoalesce:
	rm -rf ${BUILD_DIR_NOCOALESCE}
