mkfile_path := $(dir $(lastword $(MAKEFILE_LIST)))
ifeq ($(ETROBO_ENV_TARGET),physical)
APPL_CFLAGS += -DETROBO_PHYSICAL_BUILD
APPL_CXXFLAGS += -DETROBO_PHYSICAL_BUILD
endif

HUB_NAME_SOURCE := $(mkfile_path)HUB_NAME
HUB_NAME_TARGET := $(abspath $(mkfile_path)../../spike-rt/sdk/workspace/HUB_NAME)

ifneq ($(wildcard $(HUB_NAME_SOURCE)),)
$(shell mkdir -p $(dir $(HUB_NAME_TARGET)) && cp $(HUB_NAME_SOURCE) $(HUB_NAME_TARGET))
endif

# Embed the editable tracer.ini into the real-hardware firmware as a C++ string.
TRACER_CONFIG_HEADER := $(mkfile_path)app/TracerConfig.h

# This rule must not become the default goal when this file is included
# before the build system's own targets (e.g. `img` in Makefile.workspace).
TRACER_PREV_DEFAULT_GOAL := $(.DEFAULT_GOAL)

$(TRACER_CONFIG_HEADER): $(mkfile_path)tracer.ini
	@printf '%s\n' '#ifndef ETTR_APP_TRACERCONFIG_H_' '#define ETTR_APP_TRACERCONFIG_H_' '' 'static const char kTracerIni[] = R"ETROBO(' > $@
	@cat $< >> $@
	@printf '%s\n' ')ETROBO";' '' '#endif  // ETTR_APP_TRACERCONFIG_H_' >> $@

$(OBJDIR)/app.o: $(TRACER_CONFIG_HEADER)

.DEFAULT_GOAL := $(TRACER_PREV_DEFAULT_GOAL)

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
