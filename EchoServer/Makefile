#------------------------------------------
# 编译程序用
# 目前的配置项能支持大部分测试程序的编译需求
# 
#							Author BAN1993
#------------------------------------------

# 生成的目标文件
TARGET = $(BUILD_DIR)/EchoServer

# 编译成功后将文件拷贝到目标目录下
# 目前必须设置,想删掉,下面的cp也需要一起删掉(虽然不影响最终编译结果)
COPY_DIR = ../bin

# 头文件
INC_DIR = -I../include

# 依赖库
LINK_LIB = -L../lib/ -lnsocket -lbase -levent -lpthread

# 编译选项
LINK_FLAG = $(INC_DIR) \
			-Wall -g -O3 -c \
			-gdwarf-2 -std=c++0x -Wno-deprecated \
			$(MY_DEF)

# 程序需要的宏定义
MY_DEF = -D_SYSCALL \
		 #-D_TEST_MEM_

# 编译缓存目录
BUILD_DIR = build

# 编译目录
SRC_DIR = .

# 编译时要忽略的目录
# 如一些在子目录下但不能或不需要编译的代码或文件
# TODO 现在写起来不方便,但是暂时没有更好的办法
IGNORE_DIR = ! -path "$(SRC_DIR)/$(BUILD_DIR)*" \
			 ! -path "$(SRC_DIR)/.svn*" \
			 ! -path "$(SRC_DIR)/Win*"

# 编译类型
CC		:= g++
FTYPE	:= cpp
			 
#------------------------------------------
# 不变项
#------------------------------------------
GUARD    = @mkdir -p $(@D)
ALL_DIR := $(shell find $(SRC_DIR) $(IGNORE_DIR) -type d)
ALL_CODE = $(foreach dir, $(ALL_DIR), $(wildcard $(dir)/*.$(FTYPE)))
ALL_OBJ  = $(addprefix $(BUILD_DIR)/,$(patsubst %.$(FTYPE),%.o,$(notdir $(ALL_CODE))))

$(info )
$(info --------------Make Infos------------------)
$(info -Target      : $(TARGET))
$(info -Copy Path   : $(COPY_DIR))
$(info -Code Path   : $(SRC_DIR))
$(info -Define      : $(MY_DEF))
$(info -All Path    : $(ALL_DIR))
$(info -Ignore Path : $(IGNORE_DIR))
$(info -Include     : $(INC_DIR))
$(info -Lib         : $(LINK_LIB))
$(info ------------------------------------------)
$(info )

$(TARGET):$(ALL_OBJ)
	$(CC) $^ $(LINK_LIB) -o $@
	@if [ ! -d $(COPY_DIR) ]; then mkdir -p $(COPY_DIR); fi;
	cp -r $(TARGET) $(COPY_DIR)

$(BUILD_DIR)/%.o:%.cpp
	$(GUARD)
	$(CC) $(LINK_FLAG) -o $@ $<

.PHONY : clean
clean :
	@rm -rf $(BUILD_DIR) $(TARGET)
	$(info Remove : $(BUILD_DIR))
	$(info Remove : $(TARGET))
