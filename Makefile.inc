mkfile_path := $(dir $(lastword $(MAKEFILE_LIST)))
ifeq ($(ETROBO_ENV_TARGET),physical)
APPL_CFLAGS += -DETROBO_PHYSICAL_BUILD
APPL_CXXFLAGS += -DETROBO_PHYSICAL_BUILD
endif

# Shell�X�N���v�g�ŁAapp,unit�f�B���N�g�����̃\�[�X�R�[�h�����I�u�W�F�N�g�t�@�C�����ɕϊ����Ă���
APPL_CXXOBJS += $(shell ls ${mkfile_path}unit/*.cpp | sed -r 's/.*\/(.*)\.cpp/\1.o/g')
APPL_CXXOBJS += $(shell ls ${mkfile_path}app/*.cpp | sed -r 's/.*\/(.*)\.cpp/\1.o/g')

SRCLANG := c++

ifdef CONFIG_EV3RT_APPLICATION

# Include libraries
include $(EV3RT_SDK_LIB_DIR)/libcpp-spike/Makefile

endif

APPL_DIRS += $(mkfile_path)app $(mkfile_path)unit

INCLUDES += \
    -I$(mkfile_path)app \
    -I$(mkfile_path)unit
