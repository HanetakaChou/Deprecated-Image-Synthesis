# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# -----------------------------------------------------------------------------
# Macro    : my-dir
# Returns  : the directory of the current Makefile
# Usage    : $(my-dir)
# -----------------------------------------------------------------------------
my-dir = $(call parent-dir,$(lastword $(MAKEFILE_LIST)))

# ====================================================================
#
# Host system auto-detection.
#
# ====================================================================
ifeq ($(OS),Windows_NT)
	# On all modern variants of Windows (including Cygwin and Wine)
	# the OS environment variable is defined to 'Windows_NT'
	#
	# The value of PROCESSOR_ARCHITECTURE will be x86 or AMD64
	#
	HOST_OS := windows

	# Trying to detect that we're running from Cygwin is tricky
	# because we can't use $(OSTYPE): It's a Bash shell variable
	# that is not exported to sub-processes, and isn't defined by
	# other shells (for those with really weird setups).
	#
	# Instead, we assume that a program named /bin/uname.exe
	# that can be invoked and returns a valid value corresponds
	# to a Cygwin installation.
	#
	UNAME := $(shell /bin/uname.exe -s 2>NUL)
	ifneq (,$(filter CYGWIN% MINGW32% MINGW64%,$(UNAME)))
		HOST_OS := unix
		_ := $(shell rm -f NUL) # Cleaning up
	endif
else
	HOST_OS := unix
endif

# -----------------------------------------------------------------------------
# Function : parent-dir
# Arguments: 1: path
# Returns  : Parent dir or path of $1, with final separator removed.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
	# On Windows, defining parent-dir is a bit more tricky because the
	# GNU Make $(dir ...) function doesn't return an empty string when it
	# reaches the top of the directory tree, and we want to enforce this to
	# avoid infinite loops.
	#
	#   $(dir C:)     -> C:       (empty expected)
	#   $(dir C:/)    -> C:/      (empty expected)
	#   $(dir C:\)    -> C:\      (empty expected)
	#   $(dir C:/foo) -> C:/      (correct)
	#   $(dir C:\foo) -> C:\      (correct)
	#
	parent-dir = $(patsubst %/,%,$(strip \
		$(eval __dir_node := $(patsubst %/,%,$(subst \,/,$1)))\
		$(eval __dir_parent := $(dir $(__dir_node)))\
		$(filter-out $1,$(__dir_parent))\
		))
else
	parent-dir = $(patsubst %/,%,$(dir $(1:%/=%)))
endif

# -----------------------------------------------------------------------------
# Function : host-mkdir
# Arguments: 1: directory path
# Usage    : $(call host-mkdir,<path>
# Rationale: This function expands to the host-specific shell command used
#            to create a path if it doesn't exist.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-mkdir = md $(subst /,\,"$1") >NUL 2>NUL || rem
else
host-mkdir = mkdir -p $1
endif

# -----------------------------------------------------------------------------
# Function : host-rm
# Arguments: 1: list of files
# Usage    : $(call host-rm,<files>)
# Rationale: This function expands to the host-specific shell command used
#            to remove some files.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-rm = \
	$(eval __host_rm_files := $(foreach __host_rm_file,$1,$(subst /,\,$(wildcard $(__host_rm_file)))))\
	$(if $(__host_rm_files),del /f/q $(__host_rm_files) >NUL 2>NUL || rem)
else
host-rm = rm -f $1
endif

#
# Copyright (C) YuqiaoZhang(HanetakaChou)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

HIDE := @

LOCAL_PATH := $(realpath $(call my-dir))
SHADERS_DIR := $(LOCAL_PATH)/../shaders
SPIRV_DIR := $(LOCAL_PATH)/../spirv
GLSL_COMPILER_PATH := $(LOCAL_PATH)/../thirdparty/vulkansdk/bin/glslangValidator

GLSL_COMPILER_FLAGS :=
ifeq (true, $(APP_DEBUG))
	GLSL_COMPILER_FLAGS += -g -Od
else
	GLSL_COMPILER_FLAGS += -g0
endif

all : $(SPIRV_DIR)/forward_shading_vertex.inl $(SPIRV_DIR)/forward_shading_fragment.inl

$(SPIRV_DIR)/forward_shading_vertex.inl $(SPIRV_DIR)/forward_shading_vertex.d : $(SHADERS_DIR)/forward_shading_vertex.glsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" --depfile "$(SPIRV_DIR)/forward_shading_vertex.d" -V100 -x $(GLSL_COMPILER_FLAGS) -S vert -o "$(SPIRV_DIR)/forward_shading_vertex.inl" "$(SHADERS_DIR)/forward_shading_vertex.glsl"

$(SPIRV_DIR)/forward_shading_fragment.inl $(SPIRV_DIR)/forward_shading_fragment.d : $(SHADERS_DIR)/forward_shading_fragment.glsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" --depfile "$(SPIRV_DIR)/forward_shading_fragment.d" -V100 -x $(GLSL_COMPILER_FLAGS) -S frag -o "$(SPIRV_DIR)/forward_shading_fragment.inl" "$(SHADERS_DIR)/forward_shading_fragment.glsl"

$(SPIRV_DIR)/point_light_shadow_vertex.inl $(SPIRV_DIR)/point_light_shadow_vertex.d : $(SHADERS_DIR)/point_light_shadow_vertex.glsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) $(GLSL_COMPILER_PATH) --depfile "$(SPIRV_DIR)/point_light_shadow_vertex.d" -V100 -x $(GLSL_COMPILER_FLAGS) -S vert -o "$(SPIRV_DIR)/point_light_shadow_vertex.inl" "$(SHADERS_DIR)/point_light_shadow_vertex.glsl"

$(SPIRV_DIR)/point_light_shadow_fragment.inl $(SPIRV_DIR)/point_light_shadow_fragment.d : $(SHADERS_DIR)/point_light_shadow_fragment.glsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) $(GLSL_COMPILER_PATH) --depfile "$(SPIRV_DIR)/point_light_shadow_fragment.d" -V100 -x $(GLSL_COMPILER_FLAGS) -S frag -o "$(SPIRV_DIR)/point_light_shadow_fragment.inl" "$(SHADERS_DIR)/point_light_shadow_fragment.glsl"

-include $(SPIRV_DIR)/forward_shading_vertex.d $(SPIRV_DIR)/forward_shading_fragment.d $(SPIRV_DIR)/point_light_shadow_vertex.d $(SPIRV_DIR)/point_light_shadow_fragment.d

clean:
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/forward_shading_vertex.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/forward_shading_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/point_light_shadow_vertex.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/point_light_shadow_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/forward_shading_vertex.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/forward_shading_fragment.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/point_light_shadow_vertex.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/point_light_shadow_fragment.d)

.PHONY : \
	all
